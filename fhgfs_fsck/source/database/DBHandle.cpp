#include "DBHandle.h"

#include <common/storage/PathVec.h>
#include <common/toolkit/StorageTk.h>
#include <database/FsckDBException.h>
#include <toolkit/DatabaseTk.h>

static void sqlite_expectedchunkpath(sqlite3_context *context, int argc, sqlite3_value **argv)
{
   if (argc != 4)
   {
      sqlite3_result_null(context);
      return;
   }
   else
   {
      std::string entryID = (char*)sqlite3_value_text(argv[0]);

      unsigned origParentUID = (unsigned)sqlite3_value_int64(argv[1]);
      std::string origParentEntryID = (char*)sqlite3_value_text(argv[2]);
      int pathInfoFlags = (int)sqlite3_value_int(argv[3]);

      std::string resStr = DatabaseTk::calculateExpectedChunkPath(entryID, origParentUID,
         origParentEntryID, pathInfoFlags);

      sqlite3_result_text(context, resStr.c_str(), -1, SQLITE_TRANSIENT);
      return;
   }
}

/*
** This function is an unlock-notify callback registered with SQLite.
*/
static void unlock_notify_cb(void **apArg, int nArg)
{
   for ( int i = 0; i < nArg; i++ ) // normally only one Unlock notification should come in
   {
      SqliteUnlockNotification *unlockNotification = (SqliteUnlockNotification *) apArg[i];
      SafeMutexLock mutexLock(&unlockNotification->mutex);
      unlockNotification->fired = 1;
      unlockNotification->cond.signal();
      mutexLock.unlock();
   }
}

DBHandle::DBHandle(std::string& databasePath)
{
   log.setContext("DBHandle");

   this->databasePath = databasePath;

   int dbFlags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX;

   int dbOpenRes = sqlite3_open_v2(
      std::string(databasePath + "/" + DATABASE_MAINSCHEMANAME + ".db").c_str(), &(this->handle),
      dbFlags, NULL);

   if ( dbOpenRes != SQLITE_OK )
      throw FsckDBException(
         "Could not open database file: " + databasePath + "/" + DATABASE_MAINSCHEMANAME + ".db");

   // attach databases, which are individual files
   size_t schemaCount = sizeof(dbSchemas) / sizeof(dbSchemas[0]);

   for ( size_t i = 0; i < schemaCount; i++ )
   {
      std::string schemaName = dbSchemas[i];
      std::string filepath = databasePath + "/" + schemaName + ".db";

      int attachRes = sqlite3_exec(this->handle,
         std::string("ATTACH '" + filepath + "' AS " + schemaName).c_str(), NULL, NULL, NULL);
      if ( attachRes != SQLITE_OK )
         throw FsckDBException("Could not open database file: " + filepath);
   }

   // no sync after each statement
   sqlite3_exec(this->handle, "PRAGMA synchronous = OFF", NULL, NULL, NULL);

   // set database page size to 1K
   sqlite3_exec(this->handle, "PRAGMA page_size = 1024", NULL, NULL, NULL);

   // allow sqlite to use 250MB as cache for each handle (default is 2MB); set as number of pages
   sqlite3_exec(this->handle, "PRAGMA cache_size=256000", NULL, NULL, NULL);

   // temporary data and tables are held in memory, not on disk!
   sqlite3_exec(this->handle, "PRAGMA temp_store = 2", NULL, NULL, NULL);

   sqlite3_exec(this->handle, "PRAGMA journal_mode = OFF", NULL, NULL, NULL);

   sqlite3_exec(this->handle, "PRAGMA read_uncommitted = True", NULL, NULL, NULL);

   sqlite3_busy_timeout(this->handle, SQLITE_BUSY_TIMEOUT_MS);

   // register custom functions
   sqlite3_create_function(this->handle, "expectedchunkpath", 4, SQLITE_UTF8, NULL,
      &sqlite_expectedchunkpath, NULL, NULL);

   this->available = true;
}

DBHandle::~DBHandle()
{
   sqlite3_close(this->handle);
}

/*
 * This function is a wrapper around the SQLite function sqlite3_exec(). It functions in the same
 * way as exec(), except that if a required shared-cache lock cannot be obtained, this function
 * may block waiting for the lock to become available.
 *
 * Note: does not return result rows and is therefore only good for modifying queries
 *
 * @param queryStr the query to be executed
 * @param outSqlErrorStr a pointer to a string which will hold the sqlite error string (if any);
 * can be NULL, if caller is not interested
 * @param logErrors boolean indication whether or not a SQL error shall be logged (sometimes it
 * makes sense to only log in the calling function)
 *
 * @return an SQLite result code as it is returned by the sqlite functions
 */
int DBHandle::sqliteBlockingExec(const char* queryStr, std::string* outSqlErrorStr, bool logErrors)
{
   char* sqlErrorMsg;
   int res;

   while ( (res =
      sqlite3_exec(this->handle, queryStr, NULL, NULL, &sqlErrorMsg)) == SQLITE_LOCKED )
   {
      res = waitForUnlockNotify();
      if (res != SQLITE_OK)
         break;
   }

   if (res != SQLITE_OK )
   {
      // error string has to be freed with sqlite3_free (http://www.sqlite.org/c3ref/exec.html)
      if (outSqlErrorStr)
         *outSqlErrorStr = std::string(sqlErrorMsg);

      if (logErrors)
      {
         log.log(4,"SQL Error: " + std::string(sqlErrorMsg));
         log.log(4,"SQL Error Code: " + StringTk::intToStr(res));
         log.log(4,"SQL Query was: " + std::string(queryStr));
      }

      sqlite3_free(sqlErrorMsg);
   }

   return res;
}

int DBHandle::sqliteBlockingExec(std::string& queryStr, std::string* outSqlErrorStr,
   bool logErrors)
{
   return sqliteBlockingExec(queryStr.c_str(), outSqlErrorStr, logErrors);
}

/*
 ** This function is a wrapper around the SQLite function sqlite3_step(). It functions in the same
 ** way as step(), except that if a required shared-cache lock cannot be obtained, this function
 ** may block waiting for the lock to become available.
 */
int DBHandle::sqliteBlockingStep(sqlite3_stmt *stmt)
{
   int res;
   while ( (res = sqlite3_step(stmt)) == SQLITE_LOCKED )
   {
      res = waitForUnlockNotify();
      if (res != SQLITE_OK)
         break;
   }

   return res;
}

/*
 ** This function is a wrapper around the SQLite function sqlite3_prepare_v2(). It functions in the
 ** same way as prepare_v2(), except that if a required shared-cache lock cannot be obtained, this
 ** function may block waiting for the lock to become available.
 **
 * @param query UTF-8 encoded SQL query
 * @param queryLen length of query string in bytes
 * @param outStmt A pointer to the prepared statement
 * @param sqlTail The uncompiled part of query (should always be empty, if everything went fine);
 * can be NULL
 */
int DBHandle::sqliteBlockingPrepare(const char *query, int queryLen, sqlite3_stmt **outStmt,
   const char **sqlTail)
{
   int res;
   while ( (res = sqlite3_prepare_v2(this->handle, query, queryLen, outStmt, sqlTail))
      == SQLITE_LOCKED )
   {
      res = waitForUnlockNotify();
      if ( res != SQLITE_OK )
         break;
   }
   return res;
}

/*
 * To be called if a sqlite call fails with SQLITE_LOCKED. This function registers the current
 * handle for an unlock-notify callback with an instance of SqliteUnlockNotification
 */
int DBHandle::waitForUnlockNotify()
{
   int res;
   SqliteUnlockNotification unlockNotification;

   // initialize the UnlockNotification structure
   unlockNotification.fired = 0;

   // register for an unlock-notify callback
   res = sqlite3_unlock_notify(this->handle, unlock_notify_cb, (void *) &unlockNotification);

   /*
    * The call to sqlite3_unlock_notify() always returns either SQLITE_LOCKED or SQLITE_OK.
    *
    * If SQLITE_LOCKED was returned, then the system is deadlocked. In this case this function
    * needs to return SQLITE_LOCKED to the caller. Otherwise, block until the unlock-notify
    * callback is invoked, then return SQLITE_OK.
    */
   if ( res == SQLITE_OK )
   {
      SafeMutexLock mutexLock(&unlockNotification.mutex);
      if ( !unlockNotification.fired )
      {
         unlockNotification.cond.wait(&unlockNotification.mutex);
      }
      mutexLock.unlock();
   }

   return res;
}


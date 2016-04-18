#include "FsckDB.h"

#include <common/toolkit/StorageTk.h>
#include <common/toolkit/StringTk.h>
#include <common/threading/SafeRWLock.h>
#include <database/FsckDBException.h>
#include <toolkit/FsckTkEx.h>

#include <cstdio>

FsckDB::FsckDB(std::string dbFilename)
{
   this->log.setContext("FsckDB");

   this->dbFilename = dbFilename;

   int sqlRes = sqlite3_enable_shared_cache(true);

   if (sqlRes != SQLITE_OK)
      log.log(Log_WARNING, "Unable to enable shared cache mode for SQLite.");

   this->dbHandlePool = new DBHandlePool(dbFilename);
}

void FsckDB::init(bool clearDB)
{
   if ( clearDB )
   {
      // delete the database file if it exists
      if ( StorageTk::pathExists(dbFilename) )
      {
         if ( remove(dbFilename.c_str()) != 0 )
         {
            throw FsckDBException("Could not delete existing database file.");
         }
      }
   }
   else
   {
      // if we keep an old database, we need to clear the stored errors in there
      size_t arraySize = sizeof(__FsckErrorCodes) / sizeof(__FsckErrorCodes[0]);
      for(size_t i=0; i < arraySize; i++)
      {
         clearErrorTable(__FsckErrorCodes[i].errorCode);
      }
   }

   this->createTables();
}

FsckDB::~FsckDB()
{
   SAFE_DELETE(this->dbHandlePool);
}

/*
 * execute a given SQL query string;
 * does not return result rows and is therefore only good for modifying queries
 *
 * NOTE : all modifications should be done with this function, because when writing to a SQLite DB,
 * sqlite locks the database. If another thread is trying to write in the meanwhile it returns
 * SQLIT_BUSY. Inside here, we detect the SQLITE_BUSY, and retry in case DB was locked.
 * we simply loop until result is not SQLITE_BUSY; inside DBHandle a timeout period is set, so if
 * DB is locked, it will wait this period of time until SQLITE_BUSY is returned.
 *
 *
 * @param dbHandle
 * @param queryStr the query to be executed
 * @param sqlErrorStr a pointer to a string which will hold the sqlite error string (if any)
 * @param logErrors boolean indication whether or not a SQL error shall be logged (sometimes it
 * makes sense to only log in the calling function)
 *
 * @return an SQLite result code as it is returned by the sqlite functions
 */
int FsckDB::executeQuery(DBHandle *dbHandle, std::string queryStr, std::string* sqlErrorStr,
   bool logErrors)
{
   char* sqlErrorMsg;

   int execResult = SQLITE_BUSY;

   // nevertheless, we need to handle the unlikely case that a busy occurs, because the DB is
   // locked from somewhere else
   while (execResult == SQLITE_BUSY)
   {
      execResult = sqlite3_exec(dbHandle->getHandle(), queryStr.c_str(), NULL, NULL, &sqlErrorMsg);
   }

   if (execResult != SQLITE_OK )
   {
      // error string has to be freed with sqlite3_free (http://www.sqlite.org/c3ref/exec.html)
      if (sqlErrorStr)
         *sqlErrorStr = std::string(sqlErrorMsg);

      if (logErrors)
      {
         log.log(4,"SQL Error: " + std::string(sqlErrorMsg));
         log.log(4,"SQL Query was: " + queryStr);
      }

      sqlite3_free(sqlErrorMsg);
   }

   return execResult;
}

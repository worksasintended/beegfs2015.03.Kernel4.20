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

DBHandle::DBHandle(std::string dbFilename)
{
   log.setContext("DBHandle");

   this->dbFilename = dbFilename;

   int dbFlags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;

   int dbOpenRes = sqlite3_open_v2(this->dbFilename.c_str(), &(this->handle), dbFlags, NULL);

   if (dbOpenRes != SQLITE_OK)
      throw FsckDBException("Could not open database file");

   // turn on foreign key support for this handle
   // must be done for each connection (see http://sqlite.org/foreignkeys.html#fk_enable)
   sqlite3_exec(this->handle, "PRAGMA foreign_keys = ON", NULL, NULL, NULL);

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


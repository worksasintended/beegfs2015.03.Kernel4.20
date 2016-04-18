#include "FsckDB.h"

#include <common/toolkit/StorageTk.h>
#include <common/toolkit/StringTk.h>
#include <database/FsckDBException.h>
#include <toolkit/FsckTkEx.h>

#include <cstdio>

FsckDB::FsckDB(const std::string& databasePath)
{
   this->log.setContext("FsckDB");

   // add an additional subdir beegfs-fsck to database path
   this->databasePath = databasePath;

   int sqlRes = sqlite3_enable_shared_cache(true);

   if (sqlRes != SQLITE_OK)
      log.log(Log_WARNING, "Unable to enable shared cache mode for SQLite.");

   this->dbHandlePool = new DBHandlePool(databasePath);
}

void FsckDB::init(bool clearDB)
{
   if ( clearDB )
   {
      // delete the database files if they exist
      bool rmRes = DatabaseTk::removeDatabaseFiles(databasePath);

      if ( !rmRes )
         throw FsckDBException("Could not delete existing database files.");
   }
   else
   {
      // if we keep an old database, we need to clear the stored errors in there
      size_t arraySize = sizeof(__FsckErrorCodes) / sizeof(__FsckErrorCodes[0]);
      for ( size_t i = 0; i < arraySize; i++ )
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

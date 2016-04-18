#include "FsckDB.h"

#include <toolkit/FsckTkEx.h>

#include <program/Program.h>

/*
 * looks for dir entries, for which no inode was found (dangling dentries) and directly inserts
 * them into the database
 *
 * @return boolean value indicating if an error occured
 */
bool FsckDB::checkForAndInsertDanglingDirEntries()
{
   bool retVal = checkForAndInsertDirEntriesWithoutFileInode();
   retVal = retVal && checkForAndInsertDirEntriesWithoutDirInode();

   return retVal;
}

/*
 * looks for dir entries, for which no file inode was found (according to the ID) and directly
 * inserts them into the database
 *
 * @return boolean value indicating if an error occured
 */
bool FsckDB::checkForAndInsertDirEntriesWithoutFileInode()
{
   bool retVal = true;

   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_DANGLINGDENTRY);

   const char* tableNameDentries = TABLE_NAME_DIR_ENTRIES;
   const char* tableNameInodes = TABLE_NAME_FILE_INODES;

   char* selectStmtStr = NULL;
   size_t selectStmtStrLen = asprintf(&selectStmtStr,
      "SELECT %s,%s,%s FROM %s WHERE %s<>%i AND ~%s&%i EXCEPT SELECT "
         "%s.%s,%s.%s,%s.%s FROM %s,%s WHERE %s.%s=%s.%s", "name", "parentDirID", "saveNodeID",
      tableNameDentries, "entryType", FsckDirEntryType_DIRECTORY, IGNORE_ERRORS_FIELD,
      (int) FsckErrorCode_DANGLINGDENTRY, tableNameDentries, "name", tableNameDentries,
      "parentDirID", tableNameDentries, "saveNodeID", tableNameDentries, tableNameInodes,
      tableNameDentries, "id", tableNameInodes, "id");

   if ( !selectStmtStrLen )
      return false;

   char* queryStr = NULL;
   size_t queryStrLen = asprintf(&queryStr, "BEGIN TRANSACTION; INSERT INTO %s (%s,%s,%s) %s; "
      "COMMIT;", tableName.c_str(), "name", "parentDirID", "saveNodeID", selectStmtStr);

   if ( !queryStrLen )
   {
      SAFE_FREE(selectStmtStr);
      return false;
   }

   /*
    * take all into one transaction (SQLite would treat every INSERT as transaction otherwise);
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   DBHandle *dbHandle = this->dbHandlePool->acquireHandle();

   if ( dbHandle->sqliteBlockingExec(queryStr) != SQLITE_OK )
   {
      retVal = false;
      // explicitely end transaction if something went wrong
      dbHandle->sqliteBlockingExec("COMMIT");
   }

   SAFE_FREE(selectStmtStr);
   SAFE_FREE(queryStr);

   this->dbHandlePool->releaseHandle(dbHandle);

   return retVal;
}

/*
 * looks for dir entries, for which no dir inode was found (according to the ID) and
 * directly inserts them into the database
 *
 * @return boolean value indicating if an error occured
 */
bool FsckDB::checkForAndInsertDirEntriesWithoutDirInode()
{
   bool retVal = true;

   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_DANGLINGDENTRY);

   const char* tableNameDentries = TABLE_NAME_DIR_ENTRIES;
   const char* tableNameInodes = TABLE_NAME_DIR_INODES;

   char* selectStmtStr = NULL;
   size_t selectStmtStrLen = asprintf(&selectStmtStr,
      "SELECT %s,%s,%s FROM %s WHERE %s=%i AND ~%s&%i EXCEPT SELECT "
         "%s.%s,%s.%s,%s.%s FROM %s,%s WHERE %s.%s=%s.%s", "name", "parentDirID", "saveNodeID",
      tableNameDentries, "entryType", FsckDirEntryType_DIRECTORY, IGNORE_ERRORS_FIELD,
      FsckErrorCode_DANGLINGDENTRY, tableNameDentries, "name", tableNameDentries, "parentDirID",
      tableNameDentries, "saveNodeID", tableNameDentries, tableNameInodes, tableNameDentries, "id",
      tableNameInodes, "id");

   if ( !selectStmtStrLen )
      return false;

   char* queryStr = NULL;
   size_t queryStrLen = asprintf(&queryStr, "BEGIN TRANSACTION; INSERT INTO %s (%s,%s,%s) %s; "
      "COMMIT;", tableName.c_str(), "name", "parentDirID", "saveNodeID", selectStmtStr);

   if ( !queryStrLen )
   {
      SAFE_FREE(selectStmtStr);
      return false;
   }

   /*
    * take all into one transaction (SQLite would treat every INSERT as transaction otherwise);
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   DBHandle *dbHandle = this->dbHandlePool->acquireHandle();

   if ( dbHandle->sqliteBlockingExec(queryStr) != SQLITE_OK )
   {
      retVal = false;
      // explicitely end transaction if something went wrong
      dbHandle->sqliteBlockingExec("COMMIT");
   }

   SAFE_FREE(selectStmtStr);
   SAFE_FREE(queryStr);

   this->dbHandlePool->releaseHandle(dbHandle);

   return retVal;
}

/*
 * looks for dir entries, for which the inode was found on a different host as expected (according
 * to inodeOwner info in dentry) and directly inserts them into the database
 *
 * @return boolean value indicating if an error occured
 */
bool FsckDB::checkForAndInsertDirEntriesWithWrongOwner()
{
   bool retVal = checkForAndInsertDirEntriesWithWrongFileOwner();
   retVal = retVal && checkForAndInsertDirEntriesWithWrongDirOwner();

   return retVal;
}

/*
 * looks for dir entries, for which the inode was found on a different host as expected (according
 * to inodeOwner info in dentry) and directly inserts them into the database (but only for file
 * type entries)
 *
 * @return boolean value indicating if an error occured
 */
bool FsckDB::checkForAndInsertDirEntriesWithWrongFileOwner()
{
   bool retVal = true;

   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_WRONGOWNERINDENTRY);

   const char* tableNameDentries = TABLE_NAME_DIR_ENTRIES;
   const char* tableNameInodes = TABLE_NAME_FILE_INODES;

   char* selectStmtStr = NULL;
   size_t selectStmtStrLen = asprintf(&selectStmtStr, "SELECT %s.%s,%s.%s,%s.%s FROM %s,%s WHERE "
      "%s.%s<>%i AND %s.%s=%s.%s AND %s.%s<>%s.%s AND ~%s.%s&%i", tableNameDentries, "name",
      tableNameDentries, "parentDirID", tableNameDentries, "saveNodeID", tableNameDentries,
      tableNameInodes, tableNameDentries, "entryType", FsckDirEntryType_DIRECTORY,
      tableNameDentries, "id", tableNameInodes, "id", tableNameDentries, "inodeOwnerNodeID",
      tableNameInodes, "saveNodeID", tableNameDentries, IGNORE_ERRORS_FIELD,
      (int) FsckErrorCode_WRONGOWNERINDENTRY);

   if ( !selectStmtStrLen )
      return false;

   char* queryStr = NULL;
   size_t queryStrLen = asprintf(&queryStr, "BEGIN TRANSACTION; INSERT INTO %s (%s,%s,%s) %s; "
      "COMMIT;", tableName.c_str(), "name", "parentDirID", "saveNodeID", selectStmtStr);

   if ( !queryStrLen )
   {
      SAFE_FREE(selectStmtStr);
      return false;
   }

   /*
    * take all into one transaction (SQLite would treat every INSERT as transaction otherwise);
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   DBHandle *dbHandle = this->dbHandlePool->acquireHandle();

   if ( dbHandle->sqliteBlockingExec(queryStr) != SQLITE_OK )
   {
      retVal = false;
      // explicitely end transaction if something went wrong
      dbHandle->sqliteBlockingExec("COMMIT");
   }

   SAFE_FREE(selectStmtStr);
   SAFE_FREE(queryStr);

   this->dbHandlePool->releaseHandle(dbHandle);

   return retVal;
}

/*
 * looks for dir entries, for which the inode was found on a different host as expected (according
 * to inodeOwner info in dentry) and directly inserts them into the database (but only for dir
 * type entries)
 *
 * @return boolean value indicating if an error occured
 */
bool FsckDB::checkForAndInsertDirEntriesWithWrongDirOwner()
{
   bool retVal = true;

   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_WRONGOWNERINDENTRY);

   const char* tableNameDentries = TABLE_NAME_DIR_ENTRIES;
   const char* tableNameInodes = TABLE_NAME_DIR_INODES;

   // first check files
   char* selectStmtStr = NULL;
   size_t selectStmtStrLen = asprintf(&selectStmtStr, "SELECT %s.%s,%s.%s,%s.%s FROM %s,%s WHERE "
      "%s.%s=%i AND %s.%s=%s.%s AND %s.%s<>%s.%s AND ~%s.%s&%i", tableNameDentries, "name",
      tableNameDentries, "parentDirID", tableNameDentries, "saveNodeID", tableNameDentries,
      tableNameInodes, tableNameDentries, "entryType", FsckDirEntryType_DIRECTORY,
      tableNameDentries, "id", tableNameInodes, "id", tableNameDentries, "inodeOwnerNodeID",
      tableNameInodes, "saveNodeID", tableNameDentries, IGNORE_ERRORS_FIELD,
      (int) FsckErrorCode_WRONGOWNERINDENTRY);

   if ( !selectStmtStrLen )
      return false;

   char* queryStr = NULL;
   size_t queryStrLen = asprintf(&queryStr, "BEGIN TRANSACTION; INSERT INTO %s (%s,%s,%s) %s; "
      "COMMIT;", tableName.c_str(), "name", "parentDirID", "saveNodeID", selectStmtStr);

   if ( !queryStrLen )
   {
      SAFE_FREE(selectStmtStr);
      return false;
   }

   /*
    * take all into one transaction (SQLite would treat every INSERT as transaction otherwise);
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   DBHandle *dbHandle = this->dbHandlePool->acquireHandle();

   if ( dbHandle->sqliteBlockingExec(queryStr) != SQLITE_OK )
   {
      retVal = false;
      // explicitely end transaction if something went wrong
      dbHandle->sqliteBlockingExec("COMMIT");
   }

   SAFE_FREE(selectStmtStr);
   SAFE_FREE(queryStr);

   this->dbHandlePool->releaseHandle(dbHandle);

   return retVal;
}

/*
 * looks for dir inodes, for which the owner node information is not correct and directly inserts
 * them into the database
 *
 * @return boolean value indicating if an error occured
 */
bool FsckDB::checkForAndInsertInodesWithWrongOwner()
{
   bool retVal = true;

   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_WRONGINODEOWNER);

   const char* tableNameInodes = TABLE_NAME_DIR_INODES;

   // first check files
   char* selectStmtStr = NULL;
   size_t selectStmtStrLen = asprintf(&selectStmtStr,
      "SELECT %s,%s FROM %s WHERE %s<>%s AND %s<>'%s' AND ~%s&%i", "id", "saveNodeID",
      tableNameInodes, "ownerNodeID", "saveNodeID", "id", META_DISPOSALDIR_ID_STR,
      IGNORE_ERRORS_FIELD, (int) FsckErrorCode_WRONGINODEOWNER); // Note: disposal is ignored!

   if ( !selectStmtStrLen )
      return false;

   char* queryStr = NULL;
   size_t queryStrLen = asprintf(&queryStr, "BEGIN TRANSACTION; INSERT INTO %s (%s,%s) %s; COMMIT;",
      tableName.c_str(), "id", "saveNodeID", selectStmtStr);

   if ( !queryStrLen )
   {
      SAFE_FREE(selectStmtStr);
      return false;
   }

   /*
    * take all into one transaction (SQLite would treat every INSERT as transaction otherwise);
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   DBHandle *dbHandle = this->dbHandlePool->acquireHandle();

   if ( dbHandle->sqliteBlockingExec(queryStr) != SQLITE_OK )
   {
      retVal = false;
      // explicitely end transaction if something went wrong
      dbHandle->sqliteBlockingExec("COMMIT");
   }

   SAFE_FREE(selectStmtStr);
   SAFE_FREE(queryStr);

   this->dbHandlePool->releaseHandle(dbHandle);

   return retVal;
}

/*
 * looks for dir entries, for which no corresponding dentry-by-id file was found in #fsids# and
 * directly inserts them into the database (only dir entries with inlined inodes are relevant)
 *
 * @return boolean value indicating if an error occured
 */
bool FsckDB::checkForAndInsertDirEntriesWithBrokenByIDFile()
{
   bool retVal = true;

   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_BROKENFSID);

   const char* tableNameDentries = TABLE_NAME_DIR_ENTRIES;
   const char* tableNameFsIDs = TABLE_NAME_FSIDS;

   // NOTE: Only do this check for dentries, which have inlined inodes
   char* selectStmtStr = NULL;


   size_t selectStmtStrLen = asprintf(&selectStmtStr,
      "SELECT %s,%s,%s FROM %s WHERE %s AND %s!='%s' AND ~%s&%i EXCEPT "
         "SELECT %s.%s,%s.%s,%s.%s FROM %s,%s WHERE %s.%s=%s.%s "
         "AND %s.%s=%s.%s AND %s.%s=%s.%s AND %s.%s=%s.%s AND %s.%s=%s.%s", "name", "parentDirID",
      "saveNodeID", tableNameDentries, "hasInlinedInode", "parentDirID", META_DISPOSALDIR_ID_STR,
      IGNORE_ERRORS_FIELD, FsckErrorCode_BROKENFSID, tableNameDentries, "name", tableNameDentries,
      "parentDirID", tableNameDentries, "saveNodeID", tableNameDentries, tableNameFsIDs,
      tableNameFsIDs, "id", tableNameDentries, "id", tableNameFsIDs, "parentDirID",
      tableNameDentries, "parentDirID", tableNameFsIDs, "saveNodeID", tableNameDentries,
      "saveNodeID", tableNameFsIDs, "saveDevice", tableNameDentries, "saveDevice", tableNameFsIDs,
      "saveInode", tableNameDentries, "saveInode");

   if ( !selectStmtStrLen )
      return false;

   char* queryStr = NULL;
   size_t queryStrLen = asprintf(&queryStr, "BEGIN TRANSACTION; INSERT INTO %s (%s,%s,%s) %s; "
      "COMMIT;", tableName.c_str(), "name", "parentDirID", "saveNodeID", selectStmtStr);

   if ( !queryStrLen )
   {
      SAFE_FREE(selectStmtStr);
      return false;
   }

   /*
    * take all into one transaction (SQLite would treat every INSERT as transaction otherwise);
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   DBHandle *dbHandle = this->dbHandlePool->acquireHandle();

   if ( dbHandle->sqliteBlockingExec(queryStr) != SQLITE_OK )
   {
      retVal = false;
      // explicitely end transaction if something went wrong
      dbHandle->sqliteBlockingExec("COMMIT");
   }

   SAFE_FREE(selectStmtStr);
   SAFE_FREE(queryStr);

   this->dbHandlePool->releaseHandle(dbHandle);

   return retVal;
}

/*
 * looks for dentry-by-ID files in #fsids#, for which no corresponding dentry file was found and
 * directly inserts them into the database
 *
 * @return boolean value indicating if an error occured
 */
bool FsckDB::checkForAndInsertOrphanedDentryByIDFiles()
{
   bool retVal = true;

   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDFSID);

   const char* tableNameDentries = TABLE_NAME_DIR_ENTRIES;
   const char* tableNameFsIDs = TABLE_NAME_FSIDS;

   char* selectStmtStr = NULL;

   size_t selectStmtStrLen = asprintf(&selectStmtStr,
      "SELECT %s,%s,%s FROM %s WHERE ~%s&%i EXCEPT SELECT %s.%s,%s.%s,%s.%s FROM %s,%s "
         "WHERE %s.%s=%s.%s AND %s.%s=%s.%s AND %s.%s=%s.%s", "id", "parentDirID", "saveNodeID",
      tableNameFsIDs, IGNORE_ERRORS_FIELD, FsckErrorCode_ORPHANEDFSID, tableNameFsIDs, "id",
      tableNameFsIDs, "parentDirID", tableNameFsIDs, "saveNodeID", tableNameDentries,
      tableNameFsIDs, tableNameFsIDs, "id", tableNameDentries, "id", tableNameFsIDs, "parentDirID",
      tableNameDentries, "parentDirID", tableNameFsIDs, "saveNodeID", tableNameDentries,
      "saveNodeID");

   if ( !selectStmtStrLen )
      return false;

   char* queryStr = NULL;
   size_t queryStrLen = asprintf(&queryStr, "BEGIN TRANSACTION; INSERT INTO %s (%s,%s,%s) %s; "
      "COMMIT;", tableName.c_str(), "id", "parentDirID", "saveNodeID", selectStmtStr);

   if ( !queryStrLen )
   {
      SAFE_FREE(selectStmtStr);
      return false;
   }

   /*
    * take all into one transaction (SQLite would treat every INSERT as transaction otherwise);
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   DBHandle *dbHandle = this->dbHandlePool->acquireHandle();


   if ( dbHandle->sqliteBlockingExec(queryStr) != SQLITE_OK )
   {
      retVal = false;
      // explicitely end transaction if something went wrong
      dbHandle->sqliteBlockingExec("COMMIT");
   }

   SAFE_FREE(selectStmtStr);
   SAFE_FREE(queryStr);

   this->dbHandlePool->releaseHandle(dbHandle);

   return retVal;
}

/*
 * looks for dir inodes, for which no dir entry exists
 *
 * @return boolean value indicating if an error occured
 */
bool FsckDB::checkForAndInsertOrphanedDirInodes()
{
   bool retVal = true;

   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDDIRINODE);

   const char* tableNameDentries = TABLE_NAME_DIR_ENTRIES;
   const char* tableNameInodes = TABLE_NAME_DIR_INODES;

   // first check files
   // NOTE : META_DISPOSALDIR_ID_STR and META_ROOTDIR_ID_STR are ignored, as it is normal
   // for them to not have a dentry
   char* selectStmtStr = NULL;
   size_t selectStmtStrLen = asprintf(&selectStmtStr, "SELECT %s,%s FROM %s WHERE %s<>'%s' AND "
      "%s<>'%s' AND ~%s&%i EXCEPT SELECT %s.%s,%s.%s FROM %s,%s WHERE %s.%s=%s.%s", "id",
      "saveNodeID", tableNameInodes, "id", META_ROOTDIR_ID_STR, "id", META_DISPOSALDIR_ID_STR,
      IGNORE_ERRORS_FIELD, (int) FsckErrorCode_ORPHANEDDIRINODE, tableNameInodes, "id",
      tableNameInodes, "saveNodeID", tableNameInodes, tableNameDentries, tableNameInodes, "id",
      tableNameDentries, "id");

   if ( !selectStmtStrLen )
      return false;

   char* queryStr = NULL;
   size_t queryStrLen = asprintf(&queryStr, "BEGIN TRANSACTION; INSERT INTO %s (%s,%s) %s; COMMIT;",
      tableName.c_str(), "id", "saveNodeID", selectStmtStr);

   if ( !queryStrLen )
   {
      SAFE_FREE(selectStmtStr);
      return false;
   }

   /*
    * take all into one transaction (SQLite would treat every INSERT as transaction otherwise);
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   DBHandle *dbHandle = this->dbHandlePool->acquireHandle();

   if ( dbHandle->sqliteBlockingExec(queryStr) != SQLITE_OK )
   {
      retVal = false;
      // explicitely end transaction if something went wrong
      dbHandle->sqliteBlockingExec("COMMIT");
   }

   SAFE_FREE(selectStmtStr);
   SAFE_FREE(queryStr);

   this->dbHandlePool->releaseHandle(dbHandle);

   return retVal;
}

/*
 * looks for file inodes, for which no dir entry exists
 *
 * @return boolean value indicating if an error occured
 */
bool FsckDB::checkForAndInsertOrphanedFileInodes()
{
   bool retVal = true;

   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDFILEINODE);

   const char* tableNameDentries = TABLE_NAME_DIR_ENTRIES;
   const char* tableNameInodes = TABLE_NAME_FILE_INODES;

   // first check files
   char* selectStmtStr = NULL;
   /* size_t
    selectStmtStrLen = asprintf(&selectStmtStr, "SELECT %s,%s FROM %s WHERE ~%s&%i EXCEPT SELECT "
    "%s.%s,%s.%s FROM %s,%s WHERE %s.%s=%s.%s", "id", "saveNodeID", tableNameInodes,
    IGNORE_ERRORS_FIELD, (int)FsckErrorCode_ORPHANEDFILEINODE, tableNameInodes, "id",
    tableNameInodes, "saveNodeID", tableNameInodes, tableNameDentries, tableNameInodes, "id",
    tableNameDentries, "id"); */

   size_t selectStmtStrLen = asprintf(&selectStmtStr, "SELECT %s FROM %s WHERE ~%s&%i EXCEPT "
      "SELECT %s.%s FROM %s,%s WHERE %s.%s=%s.%s", "internalID", tableNameInodes,
      IGNORE_ERRORS_FIELD, (int) FsckErrorCode_ORPHANEDFILEINODE, tableNameInodes, "internalID",
      tableNameInodes, tableNameDentries, tableNameInodes, "id", tableNameDentries, "id");

   if ( !selectStmtStrLen )
      return false;

   char* queryStr = NULL;
   /* size_t queryStrLen = asprintf(&queryStr, "BEGIN TRANSACTION; INSERT INTO %s (%s,%s) %s; COMMIT;",
    tableName.c_str(), "id", "saveNodeID", selectStmtStr); */

   size_t queryStrLen = asprintf(&queryStr, "BEGIN TRANSACTION; INSERT INTO %s (%s) %s; COMMIT;",
      tableName.c_str(), "internalID", selectStmtStr);

   if ( !queryStrLen )
   {
      SAFE_FREE(selectStmtStr);
      return false;
   }

   /*
    * take all into one transaction (SQLite would treat every INSERT as transaction otherwise);
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   DBHandle *dbHandle = this->dbHandlePool->acquireHandle();

   if ( dbHandle->sqliteBlockingExec(queryStr) != SQLITE_OK )
   {
      retVal = false;
      // explicitely end transaction if something went wrong
      dbHandle->sqliteBlockingExec("COMMIT");
   }

   SAFE_FREE(selectStmtStr);
   SAFE_FREE(queryStr);

   this->dbHandlePool->releaseHandle(dbHandle);

   return retVal;
}

/*
 * looks for chunks, for which no inode exists
 *
 * @return boolean value indicating if an error occured
 */
bool FsckDB::checkForAndInsertOrphanedChunks()
{
   bool retVal = true;

   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDCHUNK);

   const char* tableNameChunks = TABLE_NAME_CHUNKS;
   const char* tableNameInodes = TABLE_NAME_FILE_INODES;

   // first check files
   char* selectStmtStr = NULL;
   size_t selectStmtStrLen = asprintf(&selectStmtStr,
      "SELECT %s,%s,%s FROM %s WHERE ~%s&%i EXCEPT SELECT "
         "%s.%s,%s.%s,%s.%s FROM %s,%s WHERE %s.%s=%s.%s", "id", "targetID",
      "buddyGroupID", tableNameChunks, IGNORE_ERRORS_FIELD, (int) FsckErrorCode_ORPHANEDCHUNK,
      tableNameChunks, "id", tableNameChunks, "targetID", tableNameChunks, "buddyGroupID",
      tableNameChunks, tableNameInodes, tableNameChunks, "id", tableNameInodes, "id");

   if ( !selectStmtStrLen )
      return false;

   char* queryStr = NULL;
   size_t queryStrLen = asprintf(&queryStr, "BEGIN TRANSACTION; INSERT INTO %s (%s,%s,%s) %s; "
      "COMMIT;", tableName.c_str(), "id", "targetID", "buddyGroupID", selectStmtStr);

   if ( !queryStrLen )
   {
      SAFE_FREE(selectStmtStr);
      return false;
   }

   /*
    * take all into one transaction (SQLite would treat every INSERT as transaction otherwise);
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   DBHandle *dbHandle = this->dbHandlePool->acquireHandle();

   if ( dbHandle->sqliteBlockingExec(queryStr) != SQLITE_OK )
   {
      retVal = false;
      // explicitely end transaction if something went wrong
      dbHandle->sqliteBlockingExec("COMMIT");
   }

   SAFE_FREE(selectStmtStr);
   SAFE_FREE(queryStr);

   this->dbHandlePool->releaseHandle(dbHandle);

   return retVal;
}

/*
 * looks for dir inodes, for which no .cont directory exists
 *
 * @return boolean value indicating if an error occured
 */
bool FsckDB::checkForAndInsertInodesWithoutContDir()
{
   bool retVal = true;

   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_MISSINGCONTDIR);

   const char* tableNameContDirs = TABLE_NAME_CONT_DIRS;
   const char* tableNameInodes = TABLE_NAME_DIR_INODES;

   // first check files
   char* selectStmtStr = NULL;
   size_t selectStmtStrLen = asprintf(&selectStmtStr,
      "SELECT %s,%s FROM %s WHERE ~%s&%i EXCEPT SELECT "
         "%s.%s,%s.%s FROM %s,%s WHERE %s.%s=%s.%s", "id", "saveNodeID", tableNameInodes,
      IGNORE_ERRORS_FIELD, (int) FsckErrorCode_MISSINGCONTDIR, tableNameInodes, "id",
      tableNameInodes, "saveNodeID", tableNameInodes, tableNameContDirs, tableNameInodes, "id",
      tableNameContDirs, "id");

   if ( !selectStmtStrLen )
      return false;

   char* queryStr = NULL;
   size_t queryStrLen = asprintf(&queryStr, "BEGIN TRANSACTION; INSERT INTO %s (%s,%s) %s; COMMIT;",
      tableName.c_str(), "id", "saveNodeID", selectStmtStr);

   if ( !queryStrLen )
   {
      SAFE_FREE(selectStmtStr);
      return false;
   }


   /*
    * take all into one transaction (SQLite would treat every INSERT as transaction otherwise);
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   DBHandle *dbHandle = this->dbHandlePool->acquireHandle();

   if ( dbHandle->sqliteBlockingExec(queryStr) != SQLITE_OK )
   {
      retVal = false;
      // explicitely end transaction if something went wrong
      dbHandle->sqliteBlockingExec("COMMIT");
   }

   SAFE_FREE(selectStmtStr);
   SAFE_FREE(queryStr);

   this->dbHandlePool->releaseHandle(dbHandle);

   return retVal;
}

/*
 * looks for content directories, for which no inode exists
 *
 * @return boolean value indicating if an error occured
 */
bool FsckDB::checkForAndInsertOrphanedContDirs()
{
   bool retVal = true;

   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDCONTDIR);

   const char* tableNameInodes = TABLE_NAME_DIR_INODES;
   const char* tableNameContDirs = TABLE_NAME_CONT_DIRS;

   // first check files
   char* selectStmtStr = NULL;
   size_t selectStmtStrLen = asprintf(&selectStmtStr,
      "SELECT %s,%s FROM %s WHERE ~%s&%i EXCEPT SELECT "
         "%s.%s,%s.%s FROM %s,%s WHERE %s.%s=%s.%s", "id", "saveNodeID", tableNameContDirs,
      IGNORE_ERRORS_FIELD, (int) FsckErrorCode_ORPHANEDCONTDIR, tableNameContDirs, "id",
      tableNameContDirs, "saveNodeID", tableNameContDirs, tableNameInodes, tableNameContDirs, "id",
      tableNameInodes, "id");

   if ( !selectStmtStrLen )
      return false;

   char* queryStr = NULL;
   size_t queryStrLen = asprintf(&queryStr, "BEGIN TRANSACTION; INSERT INTO %s (%s,%s) %s; COMMIT;",
      tableName.c_str(), "id", "saveNodeID", selectStmtStr);

   if ( !queryStrLen )
   {
      SAFE_FREE(selectStmtStr);
      return false;
   }

   /*
    * take all into one transaction (SQLite would treat every INSERT as transaction otherwise);
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   DBHandle *dbHandle = this->dbHandlePool->acquireHandle();

   if ( dbHandle->sqliteBlockingExec(queryStr) != SQLITE_OK )
   {
      retVal = false;
      // explicitely end transaction if something went wrong
      dbHandle->sqliteBlockingExec("COMMIT");
   }

   SAFE_FREE(selectStmtStr);
   SAFE_FREE(queryStr);

   this->dbHandlePool->releaseHandle(dbHandle);

   return retVal;
}

/*
 * looks for file inodes, for which the saved attribs (e.g. filesize) are not
 * equivalent to those of the primary chunks
 *
 * @return boolean value indicating if an error occured
 */
bool FsckDB::checkForAndInsertWrongInodeFileAttribs()
{
   bool retVal = true;

   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_WRONGFILEATTRIBS);

   const char* tableNameInodes = TABLE_NAME_FILE_INODES;
   const char* tableNameDentries = TABLE_NAME_DIR_ENTRIES;
   const char* tableNameChunks = TABLE_NAME_CHUNKS;

   const char* idField = "id";
   const char* parentDirIDField = "parentDirID";
   const char* fileSizeField = "fileSize";
   const char* numLinksField = "numHardLinks";

   {
      char* selectStmtStr = NULL;

      // check number of hardlinks and file size, but do not directly insert wrong sets to DB
      // Note: ignore dentries in disposal as they are not counted in numHardlinks in the inodes
      // Note: this query will return a complete fileInode per dataset + some additional fields;
      // the additional fields are only used internally and are at the end of the dataset, so we
      // can convert each dataset to a fileInode later (additional fields will be simply stripped)
      size_t selectStmtStrLen = asprintf(&selectStmtStr,
         "SELECT * FROM (SELECT subQuery.*, COUNT() as numDentries FROM %s, "
            "(SELECT %s.*, SUM(%s.%s) AS size_chunks FROM %s,%s WHERE %s.%s=%s.%s "
            "AND ~%s.%s&%i GROUP BY %s.%s) AS subQuery WHERE %s.%s=subQuery.%s AND %s.%s<>'%s' "
            "GROUP BY %s.%s HAVING subQuery.%s<>numDentries OR "
            "subQuery.%s<>subQuery.size_chunks)", tableNameDentries,
         tableNameInodes, tableNameChunks, fileSizeField, tableNameInodes, tableNameChunks,
         tableNameInodes, idField, tableNameChunks, idField,
         tableNameInodes, IGNORE_ERRORS_FIELD, (int) FsckErrorCode_WRONGFILEATTRIBS,
         tableNameInodes, idField, tableNameDentries, idField, idField, tableNameDentries,
         parentDirIDField, META_DISPOSALDIR_ID_STR, tableNameDentries, idField, numLinksField,
         fileSizeField);

      if ( !selectStmtStrLen )
         return false;

      DBCursor<FsckFileInode>* cursor = new DBCursor<FsckFileInode>(this->dbHandlePool,
         selectStmtStr);

      // now for each of these entries check if they are really wrong or if they are just a
      // sparse file (i.e. filesize calculation has to be performed)

      FsckFileInode* currentInode = cursor->open();
      while ( currentInode )
      {
         bool isError = false;

         // first check if hard link count is OK
         FsckDirEntryList dentries;
         this->getDirEntries(currentInode->getID(), &dentries);

         if ( currentInode->getNumHardLinks() == (unsigned) dentries.size() )
         {
            // perform size check
            ChunkFileInfoVec chunkFileInfoVec;
            UInt16Vector* targetIDs = currentInode->getStripeTargets();

            for ( UInt16VectorIter targetIDIter = targetIDs->begin();
               targetIDIter != targetIDs->end(); targetIDIter++ )
            {
               FsckChunk* chunk = this->getChunk(currentInode->getID(), *targetIDIter);
               if ( chunk )
               {
                  DynamicFileAttribs fileAttribs(1, chunk->getFileSize(), chunk->getUsedBlocks(),
                     0, 0);
                  ChunkFileInfo stripeNodeFileInfo(currentInode->getChunkSize(),
                     MathTk::log2Int32(currentInode->getChunkSize()), fileAttribs);
                  chunkFileInfoVec.push_back(stripeNodeFileInfo);

                  SAFE_DELETE(chunk);
               }
               else
               {
                  DynamicFileAttribs fileAttribs(1, 0, 0, 0, 0);
                  ChunkFileInfo stripeNodeFileInfo(currentInode->getChunkSize(),
                     MathTk::log2Int32(currentInode->getChunkSize()), fileAttribs);
                  chunkFileInfoVec.push_back(stripeNodeFileInfo);
               }
            }

            // now perform the actual file size check; this check is still pretty expensive and
            // should be optimized

            // this might be a buddy mirror pattern, but we are not interested in mirror targets
            // here, so we just create it as RAID0 pattern
            StripePattern* stripePattern = FsckTk::FsckStripePatternToStripePattern(
               FsckStripePatternType_RAID0, currentInode->getChunkSize(), targetIDs);
            StatData statData;

            statData.updateDynamicFileAttribs(chunkFileInfoVec, stripePattern);

            SAFE_DELETE(stripePattern);

            if ( currentInode->getFileSize() != statData.getFileSize() )
            {
               // seems to be no sparse file; filesize really seems to be wrong
               // => add it as wrong inode
               isError = true;

            }
         }
         else
            isError = true;

         if ( isError )
         {
            std::string queryStr = "BEGIN TRANSACTION; INSERT INTO " + tableName
               + " (internalID) VALUES ('" + StringTk::uint64ToStr(currentInode->getInternalID())
               + "'); COMMIT;";

            if ( cursor->getHandle()->sqliteBlockingExec(queryStr) != SQLITE_OK )
            {
               retVal = false;
               // explicitely end transaction if something went wrong
               cursor->getHandle()->sqliteBlockingExec("COMMIT");
            }
         }

         currentInode = cursor->step();
      }

      cursor->close();

      SAFE_DELETE(cursor);
      SAFE_FREE(selectStmtStr);
   }
   return retVal;
}

/*
 * looks for dir inodes, for which the saved attribs (e.g. size) are not correct
 *
 * @return boolean value indicating if an error occured
 */
bool FsckDB::checkForAndInsertWrongInodeDirAttribs()
{
   bool retVal = true;

   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_WRONGDIRATTRIBS);

   const char* tableNameInodes = TABLE_NAME_DIR_INODES;
   const char* tableNameDentries = TABLE_NAME_DIR_ENTRIES;

   char* queryStr = NULL;
   char* selectStmtStr = NULL;
   char* subQueryExpectedSizeStr = NULL;
   char* subQueryNumSubdirsStr = NULL;

   {
      // subQueryExpectedSize
      size_t subQueryExpectedSizeStrLen = asprintf(&subQueryExpectedSizeStr,
         "SELECT %s FROM %s WHERE %s=%s.%s", "COUNT()", tableNameDentries, "parentDirID",
         tableNameInodes, "id");

      if ( !subQueryExpectedSizeStrLen )
      {
         retVal = false;
         goto cleanup;
      }
   }
   {
      // subQueryNumSubdirs
      size_t subQueryNumSubdirsStrLen = asprintf(&subQueryNumSubdirsStr,
         "SELECT %s FROM %s WHERE %s=%s.%s AND %s=%i", "COUNT()", tableNameDentries, "parentDirID",
         tableNameInodes, "id", "entryType", (int) FsckDirEntryType_DIRECTORY);

      if ( !subQueryNumSubdirsStrLen )
      {
         retVal = false;
         goto cleanup;
      }
   }

   {
      size_t selectStmtStrLen = asprintf(&selectStmtStr,
         "SELECT %s,%s,%s,(%s) AS %s, %s, (%s) AS %s FROM %s WHERE (%s!=%s OR %s != %s+2) AND "
            "%s<>'%s' AND ~%s&%i", "id", "saveNodeID", "size", subQueryExpectedSizeStr,
         "expectedSize", "numHardLinks", subQueryNumSubdirsStr, "numSubDirs", tableNameInodes,
         "size", "expectedSize", "numHardLinks", "numSubdirs", "id", META_DISPOSALDIR_ID_STR,
         IGNORE_ERRORS_FIELD, (int) FsckErrorCode_WRONGDIRATTRIBS);

      if ( !selectStmtStrLen )
      {
         retVal = false;
         goto cleanup;
      }
   }

   {
      size_t queryStrLen = asprintf(&queryStr,
         "BEGIN TRANSACTION; INSERT INTO %s (%s,%s) SELECT %s,%s FROM (%s); COMMIT;",
         tableName.c_str(), "id", "saveNodeID", "id", "saveNodeID", selectStmtStr);

      if ( !queryStrLen )
      {
         retVal = false;
         goto cleanup;
      }
   }

   { // perform the query
      /*
       * take all into one transaction (SQLite would treat every INSERT as transaction otherwise);
       * by opening the handle here (and not inside executeQuery), we make sure that we have one and
       * the same handle for the complete transaction
       */
      DBHandle *dbHandle = this->dbHandlePool->acquireHandle();

      if ( dbHandle->sqliteBlockingExec(queryStr) != SQLITE_OK )
      {
         retVal = false;
         // explicitely end transaction if something went wrong
         dbHandle->sqliteBlockingExec("COMMIT");
      }

      this->dbHandlePool->releaseHandle(dbHandle);
   }

   cleanup:
   SAFE_FREE(queryStr);
   SAFE_FREE(selectStmtStr);
   SAFE_FREE(subQueryExpectedSizeStr);
   SAFE_FREE(subQueryNumSubdirsStr);

   return retVal;
}

/*
 * looks for target IDs, which are used in stripe patterns, but do not exist
 *
 * @param targetMapper
 * @return boolean value indicating if an error occured
 */
bool FsckDB::checkForAndInsertMissingStripeTargets(TargetMapper* targetMapper,
   MirrorBuddyGroupMapper* buddyGroupMapper)
{
   bool retVal = true;

   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_MISSINGTARGET);
   std::string tableNameUsedTargets = TABLE_NAME_USED_TARGETS;

   // get all existing target IDs from mapper
   UInt16List targets;
   UInt16List nodeIDs;
   targetMapper->getMappingAsLists(targets, nodeIDs);

   // get all existing buddy group IDs from mapper
   UInt16List buddyGroupIDs;
   MirrorBuddyGroupList buddyGroups;
   buddyGroupMapper->getMappingAsLists(buddyGroupIDs, buddyGroups);

   // with the existing targets create a WHERE clause for the in-DB-check
   std::string whereClauseTargets = "targetIDType="
      + StringTk::intToStr((int)FsckTargetIDType_TARGET);
   for ( UInt16ListIter iter = targets.begin(); iter != targets.end(); iter++ )
      whereClauseTargets += " AND id!='" + StringTk::uintToStr(*iter) + "'";

   std::string whereClauseBuddyGroups = "targetIDType="
      + StringTk::intToStr((int)FsckTargetIDType_BUDDYGROUP);
   for ( UInt16ListIter iter = buddyGroupIDs.begin(); iter != buddyGroupIDs.end(); iter++ )
      whereClauseBuddyGroups += " AND id!='" + StringTk::uintToStr(*iter) + "'";

   std::string whereClause = "((" + whereClauseTargets + ") OR (" + whereClauseBuddyGroups
      + ")) AND ~" + std::string(IGNORE_ERRORS_FIELD) + "&"
      + StringTk::uintToStr((int) FsckErrorCode_MISSINGTARGET);

   // now select the IDs from DB, which where not in the existing targets list and add them to
   // the error table

   std::string selectStmt = "SELECT id, targetIDType FROM " + tableNameUsedTargets + " WHERE "
      + whereClause;
   std::string queryStr = "BEGIN TRANSACTION; INSERT INTO " + tableName + "(id,targetIDType) "
      + selectStmt + "; COMMIT;";

   /*
    * take all into one transaction (SQLite would treat every INSERT as transaction otherwise);
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   DBHandle *dbHandle = this->dbHandlePool->acquireHandle();

   if ( dbHandle->sqliteBlockingExec(queryStr) != SQLITE_OK )
   {
      // we must call COMMIT here, because if query did not succeed transaction was most likely
      // not closed
      retVal = false;
      dbHandle->sqliteBlockingExec("COMMIT");
   }

   this->dbHandlePool->releaseHandle(dbHandle);

   return retVal;
}

/*
 * looks for file inodes, which have a stripe target set, which does not exist
 *
 * @param targetMapper
 * @return boolean value indicating if an error occured
 */
bool FsckDB::checkForAndInsertFilesWithMissingStripeTargets()
{
   bool retVal = true;

   // we reuse the missing target error table here, so this check must have been done before!
   std::string tableNameMissingTarget = TABLE_NAME_ERROR(FsckErrorCode_MISSINGTARGET);
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_FILEWITHMISSINGTARGET);

   std::string tableNameInodes = TABLE_NAME_FILE_INODES;
   std::string tableNameDentries = TABLE_NAME_DIR_ENTRIES;

   // get all targets, which were detected as missing
   FsckTargetIDList missingTargets;
   this->getMissingStripeTargets(&missingTargets);

   if ( missingTargets.empty() ) // we know that no file has missing targets in pattern
      return true;

   // iterate over the missing targets and generate a where clause to find files using them
   std::string inodeWhereClause = "";
   for ( FsckTargetIDListIter iter = missingTargets.begin(); iter != missingTargets.end(); iter++ )
   {
      uint16_t id = iter->getID();

      inodeWhereClause += "',' || stripeTargets || ',' LIKE '%," + StringTk::uintToStr(id)
         + ",%' OR ";
   }

   inodeWhereClause.resize(inodeWhereClause.size() - 4); // strip the last " OR "

   char* selectStmtStr = NULL;
   size_t selectStmtStrLen = asprintf(&selectStmtStr,
      "SELECT %s.%s,%s.%s,%s.%s FROM %s,%s WHERE (%s) AND %s.id=%s.id", tableNameDentries.c_str(),
      "name", tableNameDentries.c_str(), "parentDirID", tableNameDentries.c_str(), "saveNodeID",
      tableNameDentries.c_str(), tableNameInodes.c_str(), inodeWhereClause.c_str(),
      tableNameDentries.c_str(), tableNameInodes.c_str());

   if ( !selectStmtStrLen )
   {
      return false;
   }

   std::string queryStr = "BEGIN TRANSACTION; INSERT INTO " + tableName
      + "(name,parentDirID,saveNodeID) " + selectStmtStr + ";COMMIT;";

   /*
    * take all into one transaction (SQLite would treat every INSERT as transaction otherwise);
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   DBHandle *dbHandle = this->dbHandlePool->acquireHandle();

   if ( dbHandle->sqliteBlockingExec(queryStr) != SQLITE_OK )
   {
      // we must call COMMIT here, because if query did not succeed transaction was most likely
      // not closed
      retVal = false;
      dbHandle->sqliteBlockingExec("COMMIT");
   }

   this->dbHandlePool->releaseHandle(dbHandle);

   SAFE_FREE(selectStmtStr);

   return retVal;
}

/*
 * looks for chunks, which have wrong owner/group
 *
 * important for quotas
 *
 * @return boolean value indicating if an error occured
 */
bool FsckDB::checkForAndInsertChunksWithWrongPermissions()
{
   bool retVal = true;

   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_CHUNKWITHWRONGPERM);

   const char* tableNameChunks = TABLE_NAME_CHUNKS;
   const char* tableNameInodes = TABLE_NAME_FILE_INODES;

   // first check files
   char* selectStmtStr;
   size_t selectStmtStrLen = asprintf(&selectStmtStr,
      "SELECT %s.%s,%s.%s,%s.%s FROM %s,%s WHERE %s.%s=%s.%s AND (%s.%s<>%s.%s OR %s.%s<>%s.%s) "
         "AND ~%s.%s&%i", tableNameChunks, "id", tableNameChunks, "targetID", tableNameChunks,
      "buddyGroupID", tableNameChunks, tableNameInodes, tableNameChunks, "id", tableNameInodes,
      "id", tableNameChunks, "userID", tableNameInodes, "userID", tableNameChunks, "groupID",
      tableNameInodes, "groupID", tableNameChunks, IGNORE_ERRORS_FIELD,
      (int) FsckErrorCode_CHUNKWITHWRONGPERM);

   if ( !selectStmtStrLen )
      return false;

   char* queryStr;
   size_t queryStrLen = asprintf(&queryStr, "BEGIN TRANSACTION; INSERT INTO %s (%s,%s,%s) %s; "
      "COMMIT;", tableName.c_str(), "id", "targetID", "buddyGroupID", selectStmtStr);

   if ( !queryStrLen )
   {
      SAFE_FREE(selectStmtStr);
      return false;
   }

   /*
    * take all into one transaction (SQLite would treat every INSERT as transaction otherwise);
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   DBHandle *dbHandle = this->dbHandlePool->acquireHandle();

   if ( dbHandle->sqliteBlockingExec(queryStr) != SQLITE_OK )
   {
      retVal = false;
      // explicitely end transaction if something went wrong
      dbHandle->sqliteBlockingExec("COMMIT");
   }

   SAFE_FREE(selectStmtStr);
   SAFE_FREE(queryStr);

   this->dbHandlePool->releaseHandle(dbHandle);

   return retVal;
}

/*
 * looks for chunks, which are saved in the wrong place
 * *
 * @return boolean value indicating if an error occured
 */
bool FsckDB::checkForAndInsertChunksInWrongPath()
{
   bool retVal = true;

   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_CHUNKINWRONGPATH);

   const char* tableNameChunks = TABLE_NAME_CHUNKS;
   const char* tableNameInodes = TABLE_NAME_FILE_INODES;

   // first check files
   char* selectStmtStr;
   size_t selectStmtStrLen = asprintf(&selectStmtStr,
      "SELECT %s.%s,%s.%s,%s.%s FROM %s,%s WHERE %s.%s=%s.%s AND %s.%s<>expectedchunkpath(%s.%s,"
         "%s.%s,%s.%s,%s.%s ) AND ~%s.%s&%i", tableNameChunks, "id", tableNameChunks,
      "targetID", tableNameChunks, "buddyGroupID", tableNameChunks, tableNameInodes, tableNameChunks,
      "id", tableNameInodes, "id", tableNameChunks, "savedPath", tableNameChunks, "id",
      tableNameInodes, "origParentUID", tableNameInodes, "origParentEntryID", tableNameInodes,
      "pathInfoFlags", tableNameChunks, IGNORE_ERRORS_FIELD, (int) FsckErrorCode_CHUNKINWRONGPATH);

   if ( !selectStmtStrLen )
      return false;

   char* queryStr;
   size_t queryStrLen = asprintf(&queryStr, "BEGIN TRANSACTION; INSERT INTO %s (%s,%s,%s) %s; "
      "COMMIT;", tableName.c_str(), "id", "targetID", "buddyGroupID", selectStmtStr);

   if ( !queryStrLen )
   {
      SAFE_FREE(selectStmtStr);
      return false;
   }

   /*
    * take all into one transaction (SQLite would treat every INSERT as transaction otherwise);
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   DBHandle *dbHandle = this->dbHandlePool->acquireHandle();

   std::string errorStr;
   if ( dbHandle->sqliteBlockingExec(queryStr, &errorStr) != SQLITE_OK )
   {
      retVal = false;
      // explicitely end transaction if something went wrong
      dbHandle->sqliteBlockingExec("COMMIT");
   }

   SAFE_FREE(selectStmtStr);
   SAFE_FREE(queryStr);

   this->dbHandlePool->releaseHandle(dbHandle);

   return retVal;
}


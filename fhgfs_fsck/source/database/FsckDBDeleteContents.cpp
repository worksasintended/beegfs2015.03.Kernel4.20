#include "FsckDB.h"

#include <database/FsckDBException.h>
#include <toolkit/FsckTkEx.h>

/*
 * delete a set of dentries from the database
 *
 * @param dentries the list of dentries to delete
 * @param outFailedDelete a reference to the first dentry which failed to be deleted
 * @param outErrorCode a reference to a SQLErrorCode (according to outfailedDelete)
 * @param dbHandle the DBHandle to use; can be NULL, in which case a new one will be aquired
  *
 * @return true if all deletes succeeded, false otherwise
 */
bool FsckDB::deleteDirEntries(FsckDirEntryList& dentries, FsckDirEntry& outFailedDelete,
   int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_DIR_ENTRIES;

   return deleteDirEntries(tableName, dentries, outFailedDelete, outErrorCode,
      dbHandle);
}

/*
 * delete a set of dir inodes from the database
 *
 * @param dirInodes the list of dir inodes to delete
 * @param outFailedDelete a reference to the first dir inode which failed to be deleted
 * @param outErrorCode a reference to a SQLErrorCode (according to outfailedDelete)
 * @param dbHandle the DBHandle to use; can be NULL, in which case a new one will be aquired
 *
 * @return true if all deletes succeeded, false otherwise
 */
bool FsckDB::deleteDirInodes(FsckDirInodeList& dirInodes, FsckDirInode& outFailedDelete,
   int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_DIR_INODES;

   return deleteDirInodesByIDAndSaveNode(tableName, dirInodes, outFailedDelete, outErrorCode,
      dbHandle);
}

/*
 * delete a set of file inodes from the database
 *
 * @param fileInodes the list of file inodes to delete
 * @param outFailedDelete a reference to the first file inode which failed to be deleted
 * @param outErrorCode a reference to a SQLErrorCode (according to outfailedDelete)
 * @param dbHandle the DBHandle to use; can be NULL, in which case a new one will be aquired
 *
 * @return true if all deletes succeeded, false otherwise
 */
bool FsckDB::deleteFileInodes(FsckFileInodeList& fileInodes, FsckFileInode& outFailedDelete,
   int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_FILE_INODES;

   return deleteFileInodesByIDAndSaveNode(tableName, fileInodes, outFailedDelete, outErrorCode,
      dbHandle);
}

/*
 * delete a set of chunks from the database
 *
 * @param chunks the list of chunks to delete
 * @param outFailedDelete a reference to the first item which failed to be deleted
 * @param outErrorCode a reference to a SQLErrorCode (according to outfailedDelete)
 * @param dbHandle the DBHandle to use; can be NULL, in which case a new one will be aquired
 *
 * @return true if all deletes succeeded, false otherwise
 */
bool FsckDB::deleteChunks(FsckChunkList& inodes, FsckChunk& outFailedDelete,
   int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_CHUNKS;

   return deleteChunksByIDAndTargetID(tableName, inodes, outFailedDelete, outErrorCode,
      dbHandle);
}

/*
 * delete a set of dentry-by-id files from the database
 *
 * @param fsIDs the list to delete
 * @param outFailedDelete a reference to the first item which failed to be deleted
 * @param outErrorCode a reference to a SQLErrorCode (according to outfailedDelete)
 * @param dbHandle the DBHandle to use; can be NULL, in which case a new one will be aquired
 *
 * @return true if all deletes succeeded, false otherwise
 */
bool FsckDB::deleteFsIDs(FsckFsIDList& fsIDs, FsckFsID& outFailedDelete, int& outErrorCode,
   DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_FSIDS;

   return deleteFsIDs(tableName, fsIDs, outFailedDelete, outErrorCode, dbHandle);
}

/*
 * delete a set of dangling dentries from the database
 *
 * @param dentries the list of dentries to delete
 * @param outFailedDelete a reference to the first item which failed to be deleted
 * @param outErrorCode a reference to a SQLErrorCode (according to outfailedDelete)
 * @param dbHandle the DBHandle to use; can be NULL, in which case a new one will be aquired
 *
 * @return true if all deletes succeeded, false otherwise
 */
bool FsckDB::deleteDanglingDirEntries(FsckDirEntryList& dentries,
   FsckDirEntry& outFailedDelete, int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_DANGLINGDENTRY);

   return deleteDirEntries(tableName, dentries, outFailedDelete, outErrorCode, dbHandle);
}

/*
 * delete a set of inodes with wrong owner from the database
 *
 * @param inodes the list of inodes to delete
 * @param outFailedDelete a reference to the first item which failed to be deleted
 * @param outErrorCode a reference to a SQLErrorCode (according to outfailedDelete)
 * @param dbHandle the DBHandle to use; can be NULL, in which case a new one will be aquired
 *
 * @return true if all deletes succeeded, false otherwise
 */
bool FsckDB::deleteInodesWithWrongOwner(FsckDirInodeList& inodes,
   FsckDirInode& outFailedDelete, int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_WRONGINODEOWNER);

   return deleteDirInodesByIDAndSaveNode(tableName, inodes, outFailedDelete, outErrorCode,
      dbHandle);
}

/*
 * delete a set of dir entries, which point to a wrong inode owner from the database
 *
 * @param dentries the list of dentries to delete
 * @param outFailedDelete a reference to the first item which failed to be deleted
 * @param outErrorCode a reference to a SQLErrorCode (according to outfailedDelete)
 * @param dbHandle the DBHandle to use; can be NULL, in which case a new one will be aquired
 *
 * @return true if all deletes succeeded, false otherwise
 */
bool FsckDB::deleteDirEntriesWithWrongOwner(FsckDirEntryList& dentries,
   FsckDirEntry& outFailedDelete, int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_WRONGOWNERINDENTRY);

   return deleteDirEntries(tableName, dentries, outFailedDelete, outErrorCode, dbHandle);
}

/*
 * delete a set of orphaned dir inodes from the database
 *
 * @param inodes the list of inodes to delete
 * @param outFailedDelete a reference to the first item which failed to be deleted
 * @param outErrorCode a reference to a SQLErrorCode (according to outfailedDelete)
 * @param dbHandle the DBHandle to use; can be NULL, in which case a new one will be aquired
 *
 * @return true if all deletes succeeded, false otherwise
 */
bool FsckDB::deleteOrphanedInodes(FsckDirInodeList& inodes,
   FsckDirInode& outFailedDelete, int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDDIRINODE);

   return deleteDirInodesByIDAndSaveNode(tableName, inodes, outFailedDelete, outErrorCode,
      dbHandle);
}

/*
 * delete a set of orphaned file inodes from the database
 *
 * @param inodes the list of inodes to delete
 * @param outFailedDelete a reference to the first item which failed to be deleted
 * @param outErrorCode a reference to a SQLErrorCode (according to outfailedDelete)
 * @param dbHandle the DBHandle to use; can be NULL, in which case a new one will be aquired
 *
 * @return true if all deletes succeeded, false otherwise
 */
bool FsckDB::deleteOrphanedInodes(FsckFileInodeList& inodes,
   FsckFileInode& outFailedDelete, int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDFILEINODE);

   return deleteFileInodesByInternalID(tableName, inodes, outFailedDelete, outErrorCode,
      dbHandle);
}

/*
 * delete a set of orphaned chunks from the database
 *
 * @param chunks the list of chunks to delete
 * @param outFailedDelete a reference to the first item which failed to be deleted
 * @param outErrorCode a reference to a SQLErrorCode (according to outfailedDelete)
 * @param dbHandle the DBHandle to use; can be NULL, in which case a new one will be aquired
 *
 * @return true if all deletes succeeded, false otherwise
 */
bool FsckDB::deleteOrphanedChunks(FsckChunkList& inodes, FsckChunk& outFailedDelete,
   int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDCHUNK);

   return deleteChunksByIDAndTargetID(tableName, inodes, outFailedDelete, outErrorCode,
      dbHandle);
}

/*
 * delete a set of dir inodes with missing cont dir from the database
 *
 * @param inodes the list of inodes to delete
 * @param outFailedDelete a reference to the first item which failed to be deleted
 * @param outErrorCode a reference to a SQLErrorCode (according to outfailedDelete)
 * @param dbHandle the DBHandle to use; can be NULL, in which case a new one will be aquired
 *
 * @return true if all deletes succeeded, false otherwise
 */
bool FsckDB::deleteMissingContDirs(FsckDirInodeList& inodes,
   FsckDirInode& outFailedDelete, int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_MISSINGCONTDIR);

   return deleteDirInodesByIDAndSaveNode(tableName, inodes, outFailedDelete, outErrorCode,
      dbHandle);
}

/*
 * delete a set of orphaned cont dirs from the database
 *
 * @param contDirs the list of cont dirs to delete
 * @param outFailedDelete a reference to the first item which failed to be deleted
 * @param outErrorCode a reference to a SQLErrorCode (according to outfailedDelete)
 * @param dbHandle the DBHandle to use; can be NULL, in which case a new one will be aquired
 *
 * @return true if all deletes succeeded, false otherwise
 */
bool FsckDB::deleteOrphanedContDirs(FsckContDirList& contDirs,
   FsckContDir& outFailedDelete, int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDCONTDIR);

   return deleteContDirsByIDAndSaveNode(tableName, contDirs, outFailedDelete, outErrorCode,
      dbHandle);
}

/*
 * delete a set of file inodes with wrong attributes from the database
 *
 * @param inodes the list of inodes to delete
 * @param outFailedDelete a reference to the first item which failed to be deleted
 * @param outErrorCode a reference to a SQLErrorCode (according to outfailedDelete)
 * @param dbHandle the DBHandle to use; can be NULL, in which case a new one will be aquired
 *
 * @return true if all deletes succeeded, false otherwise
 */
bool FsckDB::deleteInodesWithWrongAttribs(FsckFileInodeList& inodes,
   FsckFileInode& outFailedDelete, int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_WRONGFILEATTRIBS);

   return deleteFileInodesByInternalID(tableName, inodes, outFailedDelete, outErrorCode,
      dbHandle);
}

/*
 * delete a set of dir inodes with wrong attributes from the database
 *
 * @param inodes the list of inodes to delete
 * @param outFailedDelete a reference to the first item which failed to be deleted
 * @param outErrorCode a reference to a SQLErrorCode (according to outfailedDelete)
 * @param dbHandle the DBHandle to use; can be NULL, in which case a new one will be aquired
 *
 * @return true if all deletes succeeded, false otherwise
 */
bool FsckDB::deleteInodesWithWrongAttribs(FsckDirInodeList& inodes,
   FsckDirInode& outFailedDelete, int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_WRONGDIRATTRIBS);

   return deleteDirInodesByIDAndSaveNode(tableName, inodes, outFailedDelete, outErrorCode,
      dbHandle);
}

/*
 * delete a set of missing stripe targets from DB
 *
 * @param targetIDs the list of targetIDs to delete from the missing target ID table
 * @param outFailedDelete a reference to the first item which failed to be deleted
 * @param outErrorCode a reference to a SQLErrorCode (according to outfailedDelete)
 * @param dbHandle the DBHandle to use; can be NULL, in which case a new one will be aquired
 *
 * @return true if all deletes succeeded, false otherwise
 */
bool FsckDB::deleteMissingStripeTargets(FsckTargetIDList& targetIDs, FsckTargetID& outFailedDelete,
   int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_MISSINGTARGET);
   bool releaseHandle = false;
   bool retVal = true;

   sqlite3_stmt *stmt;
   const char *sqlTail; // uncompiled part of statement, should never have anything inside
   int sqlRes;
   /*
    * When writing to the DB in parallel, the DB locks itself. SQLITE_BUSY is returned, and we have
    * to retry until it works. We want to avoid SQLITE_BUSY results, because we experienced
    * degraded performance with the "retry-methods". Therefore we use a RWLock ourselves to
    * coordinate access to the DB
    */
   SafeRWLock safeLock(&this->rwlock, SafeRWLock_WRITE);

   /*
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   if ( !dbHandle )
   {
      dbHandle = this->dbHandlePool->acquireHandle();
      releaseHandle = true;
   }

   // prepare statements
   std::string queryStr = "DELETE FROM " + tableName + " WHERE id=? AND targetIDType=?";
   sqlRes = sqlite3_prepare_v2(dbHandle->getHandle(), queryStr.c_str(), strlen(queryStr.c_str()),
      &stmt, &sqlTail);
   if ( sqlRes != SQLITE_OK )
   {
      log.log(3,
         "SQL Error; Unable to prepare SQL statement for deleting missing stripe targets; Aborting "
         "execution; SQLite Error Code: " + StringTk::intToStr(sqlRes));
      throw FsckDBException("Unable to prepare SQL statement for stripe targets");
   }

   FsckTargetIDListIter iter = targetIDs.begin();
   for ( ; iter != targetIDs.end(); iter++ )
   {
      short index = 1;

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, (int)iter->getTargetIDType());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_step(stmt);
      if ( sqlRes != SQLITE_DONE )
      {
         retVal = false;
         break;
      }

      int sqlRes = sqlite3_reset(stmt);
      if ( sqlRes != SQLITE_OK )
      {
         log.log(3,
            "SQL Error; Unable to reset SQL statement for missing stripe target; Aborting execution; "
               "target ID: " + StringTk::uintToStr(iter->getID()) + "; SQLite Error Code: "
               + StringTk::intToStr(sqlRes));
         throw FsckDBException("Unable to reset SQL statement for missing stripe target");
      }
   }

   sqlite3_finalize(stmt);

   if ( releaseHandle )
      dbHandlePool->releaseHandle(dbHandle);

   safeLock.unlock();

   if (!retVal)
   {
      outFailedDelete = *iter;
      outErrorCode = sqlRes;
      log.log(3,
         "SQL Error; Unable to delete missing stripe target; target ID: "
            + StringTk::uintToStr(iter->getID()) + "; SQLite Error Code: "
            + StringTk::intToStr(sqlRes));
   }

   return retVal;
}

/*
 * delete a set of dentries, representing files with non-existant stripe targets from the database
 *
 * @param dentries the list of dentries to delete
 * @param outFailedDelete a reference to the first item which failed to be deleted
 * @param outErrorCode a reference to a SQLErrorCode (according to outfailedDelete)
 * @param dbHandle the DBHandle to use; can be NULL, in which case a new one will be aquired
 *
 * @return true if all deletes succeeded, false otherwise
 */
bool FsckDB::deleteFilesWithMissingStripeTargets(FsckDirEntryList& dentries,
   FsckDirEntry& outFailedDelete, int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_FILEWITHMISSINGTARGET);

   return deleteDirEntries(tableName, dentries, outFailedDelete, outErrorCode, dbHandle);
}

/*
 * delete a set of dentries with a a broken dentry-by-id file
 *
 * @param dentries the list of dentries to delete
 * @param outFailedDelete a reference to the first item which failed to be deleted
 * @param outErrorCode a reference to a SQLErrorCode (according to outfailedDelete)
 * @param dbHandle the DBHandle to use; can be NULL, in which case a new one will be aquired
 *
 * @return true if all deletes succeeded, false otherwise
 */
bool FsckDB::deleteDirEntriesWithBrokenByIDFile(FsckDirEntryList& dentries,
   FsckDirEntry& outFailedDelete, int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_BROKENFSID);

   return deleteDirEntries(tableName, dentries, outFailedDelete, outErrorCode, dbHandle);
}

bool FsckDB::deleteOrphanedDentryByIDFiles(FsckFsIDList& fsIDs, FsckFsID& outFailedDelete,
   int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDFSID);

   return deleteFsIDs(tableName, fsIDs, outFailedDelete, outErrorCode, dbHandle);
}

/*
 * delete a set of chunks with wrong uid/gid from DB
 *
 * @param chunks the list of chunks to delete
 * @param outFailedDelete a reference to the first item which failed to be deleted
 * @param outErrorCode a reference to a SQLErrorCode (according to outfailedDelete)
 * @param dbHandle the DBHandle to use; can be NULL, in which case a new one will be aquired
 *
 * @return true if all deletes succeeded, false otherwise
 */
bool FsckDB::deleteChunksWithWrongPermissions(FsckChunkList& chunks,
   FsckChunk& outFailedDelete, int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_CHUNKWITHWRONGPERM);

   return deleteChunksByIDAndTargetID(tableName, chunks, outFailedDelete, outErrorCode,
      dbHandle);
}

bool FsckDB::deleteChunksInWrongPath(FsckChunkList& chunks,
   FsckChunk& outFailedDelete, int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_CHUNKINWRONGPATH);

   return deleteChunksByIDAndTargetID(tableName, chunks, outFailedDelete, outErrorCode,
      dbHandle);
}

bool FsckDB::deleteDirEntries(std::string tableName, FsckDirEntryList& dentries,
   FsckDirEntry& outFailedDelete, int& outErrorCode, DBHandle* dbHandle)
{
   bool releaseHandle = false;
   bool retVal = true;

   sqlite3_stmt *stmt;
   const char *sqlTail; // uncompiled part of statement, should never have anything inside
   int sqlRes;

   /*
    * When writing to the DB in parallel, the DB locks itself. SQLITE_BUSY is returned, and we have
    * to retry until it works. We want to avoid SQLITE_BUSY results, because we experienced
    * degraded performance with the "retry-methods". Therefore we use a RWLock ourselves to
    * coordinate access to the DB
    */
   SafeRWLock safeLock(&this->rwlock, SafeRWLock_WRITE);

   /*
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   if ( !dbHandle )
   {
      dbHandle = this->dbHandlePool->acquireHandle();
      releaseHandle = true;
   }

   // prepare statement
   std::string queryStr = "DELETE FROM " + tableName + " WHERE name=? AND parentDirID=? AND "
      "saveNodeID=?;";
   sqlRes = sqlite3_prepare_v2(dbHandle->getHandle(), queryStr.c_str(),
      strlen(queryStr.c_str()), &stmt, &sqlTail);
   if ( sqlRes != SQLITE_OK )
   {
      log.log(3,
         "SQL Error; Unable to prepare SQL statement for deleting dir entries; Aborting execution;"
            " SQLite Error Code: " + StringTk::intToStr(sqlRes));
      throw FsckDBException("Unable to prepare SQL statement for dir entry");
   }

   // NOTE : Keep this in sync with struct FsckDirEntries and table structure
   FsckDirEntryListIter iter = dentries.begin();
   for ( ; iter != dentries.end(); iter++ )
   {
      short index = 1;

      sqlRes = sqlite3_bind_text(stmt, index++, iter->getName().c_str(), -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_text(stmt, index++, iter->getParentDirID().c_str(), -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getSaveNodeID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_step(stmt);
      if ( sqlRes != SQLITE_DONE )
      {
         retVal = false;
         break;
      }

      int sqlRes = sqlite3_reset(stmt);
      if ( sqlRes != SQLITE_OK )
      {
         log.log(3,
            "SQL Error; Unable to reset SQL statement for dir entry; Aborting execution; entryID: "
               + iter->getID() + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
         throw FsckDBException("Unable to reset SQL statement for dir entry");
      }
   }

   sqlite3_finalize(stmt);

   if ( releaseHandle )
      dbHandlePool->releaseHandle(dbHandle);

   safeLock.unlock();

   if (!retVal)
   {
      outFailedDelete = *iter;
      outErrorCode = sqlRes;
      log.log(3,
         "SQL Error; Unable to delete dir entry; entryID: " + iter->getID()
         + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
   }

   return retVal;
}

bool FsckDB::deleteFsIDs(std::string tableName, FsckFsIDList& fsIDs, FsckFsID& outFailedDelete,
   int& outErrorCode, DBHandle* dbHandle)
{
   bool releaseHandle = false;
   bool retVal = true;

   sqlite3_stmt *stmt;
   const char *sqlTail; // uncompiled part of statement, should never have anything inside
   int sqlRes;

   /*
    * When writing to the DB in parallel, the DB locks itself. SQLITE_BUSY is returned, and we have
    * to retry until it works. We want to avoid SQLITE_BUSY results, because we experienced
    * degraded performance with the "retry-methods". Therefore we use a RWLock ourselves to
    * coordinate access to the DB
    */
   SafeRWLock safeLock(&this->rwlock, SafeRWLock_WRITE);

   /*
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   if ( !dbHandle )
   {
      dbHandle = this->dbHandlePool->acquireHandle();
      releaseHandle = true;
   }

   // prepare statement
   std::string queryStr = "DELETE FROM " + tableName + " WHERE id=? AND parentDirID=? AND "
      "saveNodeID=?";
   sqlRes = sqlite3_prepare_v2(dbHandle->getHandle(), queryStr.c_str(),
      strlen(queryStr.c_str()), &stmt, &sqlTail);
   if ( sqlRes != SQLITE_OK )
   {
      log.log(3,
         "SQL Error; Unable to prepare SQL statement for deleting fs-id files; Aborting execution;"
            " SQLite Error Code: " + StringTk::intToStr(sqlRes));
      throw FsckDBException("Unable to prepare SQL statement for fs-id files");
   }

   FsckFsIDListIter iter = fsIDs.begin();
   for ( ; iter != fsIDs.end(); iter++ )
   {
      short index = 1;

      sqlRes = sqlite3_bind_text(stmt, index++, iter->getID().c_str(), -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_text(stmt, index++, iter->getParentDirID().c_str(), -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getSaveNodeID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_step(stmt);
      if ( sqlRes != SQLITE_DONE )
      {
         retVal = false;
         break;
      }

      int sqlRes = sqlite3_reset(stmt);
      if ( sqlRes != SQLITE_OK )
      {
         log.log(3,
            "SQL Error; Unable to reset SQL statement for fs-id file; Aborting execution; entryID: "
               + iter->getID() + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
         throw FsckDBException("Unable to reset SQL statement for dir entry");
      }
   }

   sqlite3_finalize(stmt);

   if ( releaseHandle )
      dbHandlePool->releaseHandle(dbHandle);

   safeLock.unlock();

   if (!retVal)
   {
      outFailedDelete = *iter;
      outErrorCode = sqlRes;
      log.log(3,
         "SQL Error; Unable to delete fs-id file; entryID: " + iter->getID()
         + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
   }

   return retVal;
}

bool FsckDB::deleteFileInodesByIDAndSaveNode(std::string tableName, FsckFileInodeList& inodes,
   FsckFileInode& outFailedDelete, int& outErrorCode, DBHandle* dbHandle)
{
   bool releaseHandle = false;
   bool retVal = true;

   sqlite3_stmt *stmt;
   const char *sqlTail; // uncompiled part of statement, should never have anything inside
   int sqlRes;

   /*
    * When writing to the DB in parallel, the DB locks itself. SQLITE_BUSY is returned, and we have
    * to retry until it works. We want to avoid SQLITE_BUSY results, because we experienced
    * degraded performance with the "retry-methods". Therefore we use a RWLock ourselves to
    * coordinate access to the DB
    */
   SafeRWLock safeLock(&this->rwlock, SafeRWLock_WRITE);

   /*
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   if ( !dbHandle )
   {
      dbHandle = this->dbHandlePool->acquireHandle();
      releaseHandle = true;
   }

   // prepare statement
   std::string queryStr =
      "DELETE FROM " + tableName + " wHERE id=? AND saveNodeID=?";

   sqlRes = sqlite3_prepare_v2(dbHandle->getHandle(), queryStr.c_str(),
      strlen(queryStr.c_str()), &stmt, &sqlTail);
   if ( sqlRes != SQLITE_OK )
   {
      log.log(3,
         "SQL Error; Unable to prepare SQL statement for deleting file inodes; Aborting execution;"
            " SQLite Error Code: " + StringTk::intToStr(sqlRes));
      throw FsckDBException("Unable to prepare SQL statement for file inodes");
   }

   // NOTE : Keep this in sync with struct FsckFileInodes and table structure
   FsckFileInodeListIter iter = inodes.begin();
   for ( ; iter != inodes.end(); iter++ )
   {
      short index = 1;

      sqlRes = sqlite3_bind_text(stmt, index++, iter->getID().c_str(), -1 , 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getSaveNodeID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_step(stmt);
      if (sqlRes != SQLITE_DONE)
      {
         retVal = false;
         break;
      }

      int resetRes = sqlite3_reset(stmt);
      if (resetRes != SQLITE_OK)
      {
         log.log(3,
            "SQL Error; Unable to reset SQL statement for file inode; Aborting execution; entryID: "
               + iter->getID() + "; SQLite Error Code: " + StringTk::intToStr(resetRes));
         throw FsckDBException("Unable to reset SQL statement for file inode");
      }
   }

   sqlite3_finalize(stmt);

   if ( releaseHandle )
      dbHandlePool->releaseHandle(dbHandle);

   safeLock.unlock();

   if (!retVal)
   {
      outFailedDelete = *iter;
      outErrorCode = sqlRes;
      log.log(3,
         "SQL Error; Unable to delete file inode; entryID: " + iter->getID()
         + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
   }

   return retVal;
}

bool FsckDB::deleteFileInodesByInternalID(std::string tableName, FsckFileInodeList& inodes,
   FsckFileInode& outFailedDelete, int& outErrorCode, DBHandle* dbHandle)
{
   bool releaseHandle = false;
   bool retVal = true;

   sqlite3_stmt *stmt;
   const char *sqlTail; // uncompiled part of statement, should never have anything inside
   int sqlRes;

   /*
    * When writing to the DB in parallel, the DB locks itself. SQLITE_BUSY is returned, and we have
    * to retry until it works. We want to avoid SQLITE_BUSY results, because we experienced
    * degraded performance with the "retry-methods". Therefore we use a RWLock ourselves to
    * coordinate access to the DB
    */
   SafeRWLock safeLock(&this->rwlock, SafeRWLock_WRITE);

   /*
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   if ( !dbHandle )
   {
      dbHandle = this->dbHandlePool->acquireHandle();
      releaseHandle = true;
   }

   // prepare statement
   std::string queryStr =
      "DELETE FROM " + tableName + " WHERE internalID=?";

   sqlRes = sqlite3_prepare_v2(dbHandle->getHandle(), queryStr.c_str(),
      strlen(queryStr.c_str()), &stmt, &sqlTail);
   if ( sqlRes != SQLITE_OK )
   {
      log.log(3,
         "SQL Error; Unable to prepare SQL statement for deleting file inodes; Aborting execution;"
            " SQLite Error Code: " + StringTk::intToStr(sqlRes));
      throw FsckDBException("Unable to prepare SQL statement for file inodes");
   }

   // NOTE : Keep this in sync with struct FsckFileInodes and table structure
   FsckFileInodeListIter iter = inodes.begin();
   for ( ; iter != inodes.end(); iter++ )
   {
      short index = 1;

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getInternalID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_step(stmt);
      if (sqlRes != SQLITE_DONE)
      {
         retVal = false;
         break;
      }

      int resetRes = sqlite3_reset(stmt);
      if (resetRes != SQLITE_OK)
      {
         log.log(3,
            "SQL Error; Unable to reset SQL statement for file inode; Aborting execution; entryID: "
               + iter->getID() + "; SQLite Error Code: " + StringTk::intToStr(resetRes));
         throw FsckDBException("Unable to reset SQL statement for file inode");
      }
   }

   sqlite3_finalize(stmt);

   if ( releaseHandle )
      dbHandlePool->releaseHandle(dbHandle);

   safeLock.unlock();

   if (!retVal)
   {
      outFailedDelete = *iter;
      outErrorCode = sqlRes;
      log.log(3,
         "SQL Error; Unable to delete file inode; entryID: " + iter->getID()
         + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
   }

   return retVal;
}

bool FsckDB::deleteDirInodesByIDAndSaveNode(std::string tableName, FsckDirInodeList& inodes,
   FsckDirInode& outFailedDelete, int& outErrorCode, DBHandle* dbHandle)
{
   bool releaseHandle = false;
   bool retVal = true;

   sqlite3_stmt *stmt;
   const char *sqlTail; // uncompiled part of statement, should never have anything inside
   int sqlRes;

   /*
    * When writing to the DB in parallel, the DB locks itself. SQLITE_BUSY is returned, and we have
    * to retry until it works. We want to avoid SQLITE_BUSY results, because we experienced
    * degraded performance with the "retry-methods". Therefore we use a RWLock ourselves to
    * coordinate access to the DB
    */
   SafeRWLock safeLock(&this->rwlock, SafeRWLock_WRITE);

   /*
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   if ( !dbHandle )
   {
      dbHandle = this->dbHandlePool->acquireHandle();
      releaseHandle = true;
   }

   // prepare statement
   std::string queryStr = "DELETE FROM " + tableName + " WHERE id=? AND saveNodeID=?";

   sqlRes = sqlite3_prepare_v2(dbHandle->getHandle(), queryStr.c_str(),
      strlen(queryStr.c_str()), &stmt, &sqlTail);
   if ( sqlRes != SQLITE_OK )
   {
      log.log(3,
         "SQL Error; Unable to prepare SQL statement for deleting dir inodes; Aborting execution;"
            " SQLite Error Code: " + StringTk::intToStr(sqlRes));
      throw FsckDBException("Unable to prepare SQL statement for dir inodes");
   }

   // NOTE : Keep this in sync with struct FsckDirInodes and table structure
   FsckDirInodeListIter iter = inodes.begin();
   for ( ; iter != inodes.end(); iter++ )
   {
      short index = 1;

      sqlRes = sqlite3_bind_text(stmt, index++, iter->getID().c_str(), -1 , 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getSaveNodeID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_step(stmt);
      if (sqlRes != SQLITE_DONE)
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_reset(stmt);
      if (sqlRes != SQLITE_OK)
      {
         log.log(3,
            "SQL Error; Unable to reset SQL statement for dir inode; Aborting execution; entryID: "
               + iter->getID() + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
         throw FsckDBException("Unable to reset SQL statement for dir inode");
      }
   }

   sqlite3_finalize(stmt);

   if ( releaseHandle )
      dbHandlePool->releaseHandle(dbHandle);

   safeLock.unlock();

   if (!retVal)
   {
      outFailedDelete = *iter;
      outErrorCode = sqlRes;
      log.log(3,
         "SQL Error; Unable to delete dir inode; entryID: " + iter->getID()
         + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
   }

   return retVal;
}

bool FsckDB::deleteChunksByIDAndTargetID(std::string tableName, FsckChunkList& chunks,
   FsckChunk& outFailedDelete, int& outErrorCode, DBHandle* dbHandle)
{
   bool releaseHandle = false;
   bool retVal = true;

   sqlite3_stmt *stmt;
   const char *sqlTail; // uncompiled part of statement, should never have anything inside
   int sqlRes;

   /*
    * When writing to the DB in parallel, the DB locks itself. SQLITE_BUSY is returned, and we have
    * to retry until it works. We want to avoid SQLITE_BUSY results, because we experienced
    * degraded performance with the "retry-methods". Therefore we use a RWLock ourselves to
    * coordinate access to the DB
    */
   SafeRWLock safeLock(&this->rwlock, SafeRWLock_WRITE);

   if ( !dbHandle )
   {
      releaseHandle = true;
      dbHandle = this->dbHandlePool->acquireHandle();
   }

   // prepare statement
   std::string queryStr = "DELETE FROM " + tableName + " WHERE id=? AND targetID=? AND "
      "buddyGroupID=?";

   sqlRes = sqlite3_prepare_v2(dbHandle->getHandle(), queryStr.c_str(),
      strlen(queryStr.c_str()), &stmt, &sqlTail);
   if ( sqlRes != SQLITE_OK )
   {
      log.log(3,
         "SQL Error; Unable to prepare SQL statement for deleting chunks; Aborting execution;"
            " SQLite Error Code: " + StringTk::intToStr(sqlRes));
      throw FsckDBException("Unable to prepare SQL statement for chunks");
   }
   // NOTE : Keep this in sync with struct FsckChunk and table structure
   FsckChunkListIter iter = chunks.begin();
   for ( ; iter != chunks.end(); iter++ )
   {
      short index = 1;

      sqlRes = sqlite3_bind_text(stmt, index++, iter->getID().c_str(), -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getTargetID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getBuddyGroupID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_step(stmt);
      if ( (sqlRes != SQLITE_DONE) )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_reset(stmt);
      if ( sqlRes != SQLITE_OK )
      {
         log.log(3,
            "SQL Error; Unable to reset SQL statement for chunk; Aborting execution; entryID: "
               + iter->getID() + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
         throw FsckDBException("Unable to reset SQL statement for chunk");
      }
   }

   sqlite3_finalize(stmt);

   if ( releaseHandle )
      dbHandlePool->releaseHandle(dbHandle);

   safeLock.unlock();

   if ( !retVal )
   {
      outFailedDelete = *iter;
      outErrorCode = sqlRes;
      log.log(3,
         "SQL Error; Unable to delete chunk; entryID: " + iter->getID() + "; targetID: "
            + StringTk::uintToStr(iter->getTargetID()) + "; buddyGroupID: "
            + StringTk::uintToStr(iter->getBuddyGroupID()) + "; SQLite Error Code: "
            + StringTk::intToStr(sqlRes));
   }

   return retVal;
}

bool FsckDB::deleteContDirsByIDAndSaveNode(std::string tableName, FsckContDirList& contDirs,
   FsckContDir& outFailedDelete, int& outErrorCode, DBHandle* dbHandle)
{
   bool releaseHandle = false;
   bool retVal = true;

   sqlite3_stmt *stmt;
   const char *sqlTail; // uncompiled part of statement, should never have anything inside
   int sqlRes;

   /*
    * When writing to the DB in parallel, the DB locks itself. SQLITE_BUSY is returned, and we have
    * to retry until it works. We want to avoid SQLITE_BUSY results, because we experienced
    * degraded performance with the "retry-methods". Therefore we use a RWLock ourselves to
    * coordinate access to the DB
    */
   SafeRWLock safeLock(&this->rwlock, SafeRWLock_WRITE);

   if ( !dbHandle )
   {
      releaseHandle = true;
      dbHandle = this->dbHandlePool->acquireHandle();
   }

   // prepare statement
   std::string queryStr = "DELETE FROM " + tableName + " WHERE id=? AND saveNodeID=?";

   sqlRes = sqlite3_prepare_v2(dbHandle->getHandle(), queryStr.c_str(),
      strlen(queryStr.c_str()), &stmt, &sqlTail);
   if ( sqlRes != SQLITE_OK )
   {
      log.log(3,
         "SQL Error; Unable to prepare SQL statement for deleting content directories; "
         "Aborting execution; SQLite Error Code: " + StringTk::intToStr(sqlRes));
      throw FsckDBException("Unable to prepare SQL statement for content directories");
   }

   // NOTE : Keep this in sync with struct FsckContDir and table structure
   FsckContDirListIter iter = contDirs.begin();
   for ( ; iter != contDirs.end(); iter++ )
   {
      short index = 1;

      sqlRes = sqlite3_bind_text(stmt, index++, iter->getID().c_str(), -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getSaveNodeID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_step(stmt);
      if ( (sqlRes != SQLITE_DONE) )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_reset(stmt);
      if ( sqlRes != SQLITE_OK )
      {
         log.log(3,
            "SQL Error; Unable to reset SQL statement for content directory; Aborting execution; "
               "directory ID: " + iter->getID() + "; SQLite Error Code: "
               + StringTk::intToStr(sqlRes));
         throw FsckDBException("Unable to reset SQL statement for content directory");
      }
   }

   sqlite3_finalize(stmt);

   if ( releaseHandle )
      dbHandlePool->releaseHandle(dbHandle);

   safeLock.unlock();

   if ( !retVal )
   {
      outFailedDelete = *iter;
      outErrorCode = sqlRes;
      log.log(3,
         "SQL Error; Unable to delete content directory; directory ID: " + iter->getID()
            + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
   }

   return retVal;
}

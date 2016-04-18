#include "FsckDB.h"

#include <database/FsckDBException.h>
#include <toolkit/FsckTkEx.h>

/*
 * inserts a list of dentries into the database; will abort on the first insert failure
 *
 * @param dentries the list of dentries to inserts
 * @param outFailedInsert a reference to a dentry which failed to be inserted
 * @param outErrorCode a reference to an int (SQLErrorCode, if insert failed)
 * @param dbHandle a pointer to the dbHandle to use; can be NULL, in which case a new handle is acquired
 *
 * @return true if insert was completed successfully, false if failure occured
 */
bool FsckDB::insertDirEntries(FsckDirEntryList& dentries, FsckDirEntry& outFailedInsert,
   int& outErrorCode, DBHandle* dbHandle)
{
   bool releaseHandle = false;
   std::string tableName = TABLE_NAME_DIR_ENTRIES;
   bool retVal = true;

   sqlite3_stmt *stmt;
   const char *sqlTail; // uncompiled part of statement, should never have anything inside
   int sqlRes;

   if ( !dbHandle )
   {
      releaseHandle = true;
      dbHandle = this->dbHandlePool->acquireHandle();
   }

   // prepare statement
   std::string queryStr = "INSERT INTO " + tableName
      + " (id, name, parentDirID, entryOwnerNodeID, inodeOwnerNodeID, "
         "entryType, hasInlinedInode, saveNodeID, saveDevice, saveInode) VALUES "
         "(?,?,?,?,?,?,?,?,?,?);";

   dbHandle->sqliteBlockingExec("BEGIN TRANSACTION");

   sqlRes = dbHandle->sqliteBlockingPrepare(queryStr.c_str(), strlen(queryStr.c_str()), &stmt,
      &sqlTail);

   if ( sqlRes != SQLITE_OK )
   {
      log.log(3,
         "SQL Error; Unable to prepare SQL statement for inserting dir entries; Aborting execution;"
            " SQLite Error Code: " + StringTk::intToStr(sqlRes));
      throw FsckDBException("Unable to prepare SQL statement for dir entry");
   }

   // NOTE : Keep this in sync with struct DB_Dentries and table structure
   FsckDirEntryListIter iter = dentries.begin();
   for ( ; iter != dentries.end(); iter++ )
   {
      std::string saveInodeString;

      short index = 1;

      sqlRes = sqlite3_bind_text(stmt, index++, iter->getID().c_str(), -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_text(stmt, index++, iter->getName().c_str(), -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_text(stmt, index++, iter->getParentDirID().c_str(),
         strlen(iter->getParentDirID().c_str()), 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getEntryOwnerNodeID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getInodeOwnerNodeID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, (int) iter->getEntryType());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, (int) iter->getHasInlinedInode());
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

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getSaveDevice());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      // saveInode is uint64_t, but sqlite int type can't handle uint64; therefore we save this
      // as text
      saveInodeString = StringTk::uint64ToStr(iter->getSaveInode());

      sqlRes = sqlite3_bind_text(stmt, index++, saveInodeString.c_str(), -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = dbHandle->sqliteBlockingStep(stmt);
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

   dbHandle->sqliteBlockingExec("COMMIT");

   if ( releaseHandle )
      dbHandlePool->releaseHandle(dbHandle);

   if (!retVal)
   {
      outFailedInsert = *iter;
      outErrorCode = sqlRes;
      log.log(3,
         "SQL Error; Unable to insert dir entry; entryID: " + iter->getID()
         + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
   }

   return retVal;
}

/*
 * inserts a list of file inodes into the database; will abort on the first insert failure
 *
 * @param dentries the list of dentries to inserts
 * @param outFailedInsert a reference to a file inode which failed to be inserted
 * @param outErrorCode a reference to an int (SQLErrorCode, if insert failed)
 * @param dbHandle a pointer to the dbHandle to use; can be NULL, in which case a new handle is acquired
 *
 * @return true if insert was completed successfully, false if failure occured
 */
bool FsckDB::insertFileInodes(FsckFileInodeList& fileInodes,
   FsckFileInode& outFailedInsert, int& outErrorCode, DBHandle* dbHandle)
{
   bool releaseHandle = false;
   bool retVal = true;
   std::string tableName = TABLE_NAME_FILE_INODES;

   sqlite3_stmt *stmt;
   const char *sqlTail; // uncompiled part of statement, should never have anything inside
   int sqlRes;
   std::string stripeTargetsStr;

   if ( !dbHandle )
   {
      releaseHandle = true;
      dbHandle = this->dbHandlePool->acquireHandle();
   }

   // prepare statement
   std::string queryStr =
      "INSERT INTO " + tableName
         + " (id, parentDirID, parentNodeID, origParentUID, origParentEntryID, pathInfoFlags, "
            "mode, userID, groupID, fileSize, usedBlocks, creationTime, modificationTime, "
            "lastAccessTime, numHardLinks, stripePatternType, stripeTargets, chunkSize, saveNodeID, "
            "isInlined, readable) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";

   dbHandle->sqliteBlockingExec("BEGIN TRANSACTION");

   sqlRes = dbHandle->sqliteBlockingPrepare(queryStr.c_str(),
      strlen(queryStr.c_str()), &stmt, &sqlTail);
   if ( sqlRes != SQLITE_OK )
   {
      log.log(3,
         "SQL Error; Unable to prepare SQL statement for inserting file inodes; Aborting execution;"
            " SQLite Error Code: " + StringTk::intToStr(sqlRes));
      throw FsckDBException("Unable to prepare SQL statement for file inodes");
   }

   // NOTE : Keep this in sync with struct DB_FileInodes and table structure
   FsckFileInodeListIter iter = fileInodes.begin();
   for ( ; iter != fileInodes.end(); iter++ )
   {
      std::string usedBlocksString;
      short index = 1;

      sqlRes = sqlite3_bind_text(stmt, index++, iter->getID().c_str(), -1 , 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_text(stmt, index++, iter->getParentDirID().c_str(), -1 , 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getParentNodeID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      PathInfo* pathInfo = iter->getPathInfo();
      sqlRes = sqlite3_bind_int64(stmt, index++, pathInfo->getOrigUID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_text(stmt, index++, pathInfo->getOrigParentEntryID().c_str(), -1 , 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, pathInfo->getFlags());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getMode());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getUserID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getGroupID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getFileSize());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      // usedBlocks is uint64_t, but sqlite int type can't handle uint64; therefore we save this
      // as text
      usedBlocksString = StringTk::uint64ToStr(iter->getUsedBlocks());

      sqlRes = sqlite3_bind_text(stmt, index++, usedBlocksString.c_str(), -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getCreationTime());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getModificationTime());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getLastAccessTime());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getNumHardLinks());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, (int) iter->getStripePatternType());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      stripeTargetsStr = StringTk::uint16VecToStr(iter->getStripeTargets());
      sqlRes = sqlite3_bind_text(stmt, index++, stripeTargetsStr.c_str(), -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getChunkSize());
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

      sqlRes = sqlite3_bind_int(stmt, index++, (int) iter->getIsInlined());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, (int) iter->getReadable());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = dbHandle->sqliteBlockingStep(stmt);
      // ignore constraint errors here, these can occur with hardlinks and inlined file inodes
      if ( ( sqlRes != SQLITE_DONE )  && ( sqlRes != SQLITE_CONSTRAINT) )
      {
         retVal = false;
         break;
      }

      int sqlRes = sqlite3_reset(stmt);
      if ( ( sqlRes != SQLITE_OK )  && ( sqlRes != SQLITE_CONSTRAINT) )
      {
         log.log(3,
            "SQL Error; Unable to reset SQL statement for file inode; Aborting execution; entryID: "
               + iter->getID() + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
         throw FsckDBException("Unable to reset SQL statement for file inode");
      }
   }

   sqlite3_finalize(stmt);

   dbHandle->sqliteBlockingExec("COMMIT");

   if ( releaseHandle )
      dbHandlePool->releaseHandle(dbHandle);

   if (!retVal)
   {
      outFailedInsert = *iter;
      outErrorCode = sqlRes;
      log.log(3,
         "SQL Error; Unable to insert file inode; entryID: " + iter->getID()
         + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
   }

   return retVal;
}

/*
 * inserts a list of dir inodes into the database; will abort on the first insert failure
 *
 * @param dentries the list of dentries to inserts
 * @param outFailedInsert a reference to a dir inode which failed to be inserted
 * @param outErrorCode a reference to an int (SQLErrorCode, if insert failed)
 * @param dbHandle a pointer to the dbHandle to use; can be NULL, in which case a new handle is acquired
 *
 * @return true if insert was completed successfully, false if failure occured
 */
bool FsckDB::insertDirInodes(FsckDirInodeList& dirInodes, FsckDirInode& outFailedInsert,
   int& outErrorCode, DBHandle* dbHandle)
{
   bool releaseHandle = false;
   bool retVal = true;
   std::string tableName = TABLE_NAME_DIR_INODES;

   sqlite3_stmt *stmt;
   const char *sqlTail; // uncompiled part of statement, should never have anything inside
   int sqlRes;
   std::string stripeTargetsStr;

   if ( !dbHandle )
   {
      releaseHandle = true;
      dbHandle = this->dbHandlePool->acquireHandle();
   }

   dbHandle->sqliteBlockingExec("BEGIN TRANSACTION");

   // prepare statement
   std::string queryStr = "INSERT INTO " + tableName
      + " (id, ownerNodeID, parentDirID, parentNodeID, size, "
         "numHardLinks, stripePatternType, stripeTargets, saveNodeID, readable) VALUES "
         "(?,?,?,?,?,?,?,?,?,?);";

   sqlRes = dbHandle->sqliteBlockingPrepare(queryStr.c_str(),
      strlen(queryStr.c_str()), &stmt, &sqlTail);
   if ( sqlRes != SQLITE_OK )
   {
      log.log(3,
         "SQL Error; Unable to prepare SQL statement for inserting dir inodes; Aborting execution;"
            " SQLite Error Code: " + StringTk::intToStr(sqlRes));
      throw FsckDBException("Unable to prepare SQL statement for dir inodes");
   }

   // NOTE : Keep this in sync with struct DB_DirInodes and table structure
   FsckDirInodeListIter iter = dirInodes.begin();
   for ( ; iter != dirInodes.end(); iter++ )
   {
      short index = 1;

      sqlRes = sqlite3_bind_text(stmt, index++, iter->getID().c_str(), -1 , 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getOwnerNodeID());
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

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getParentNodeID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getSize());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getNumHardLinks());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, (int) iter->getStripePatternType());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      stripeTargetsStr = StringTk::uint16VecToStr(iter->getStripeTargets());
      sqlRes = sqlite3_bind_text(stmt, index++, stripeTargetsStr.c_str(), -1, 0);
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

      sqlRes = sqlite3_bind_int(stmt, index++, (int) iter->getReadable());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = dbHandle->sqliteBlockingStep(stmt);
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

   dbHandle->sqliteBlockingExec("COMMIT");

   if ( releaseHandle )
      dbHandlePool->releaseHandle(dbHandle);

   if (!retVal)
   {
      outFailedInsert = *iter;
      outErrorCode = sqlRes;
      log.log(3,
         "SQL Error; Unable to insert dir inode; entryID: " + iter->getID()
         + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
   }

   return retVal;
}

/*
 * inserts a list of chunks into the database
 *
 * @param chunks the list of chunks to insert
 * @param outFailedInsert reference to a chunk, which will be set if a insert fails
 * @param outErrorCode reference to an int, which will be set to sqlite error code according to outFailedInsert
 * @param dbHandle a pointer to the dbHandle to use; can be NULL, in which case a new handle is acquired
 *
 * @return true if no failure occured, false otherwise
 */
bool FsckDB::insertChunks(FsckChunkList& chunks, FsckChunk& outFailedInsert,
   int& outErrorCode, DBHandle* dbHandle)
{
   bool releaseHandle = false;
   bool retVal = true;
   std::string tableName = TABLE_NAME_CHUNKS;

   sqlite3_stmt *stmt;
   const char *sqlTail; // uncompiled part of statement, should never have anything inside
   int sqlRes;

   if ( !dbHandle )
   {
      releaseHandle = true;
      dbHandle = this->dbHandlePool->acquireHandle();
   }

   // prepare statement
   std::string queryStr = "INSERT INTO " + tableName
      + " (id, targetID, savedPath, fileSize, usedBlocks, creationTime, modificationTime, "
         "lastAccessTime, userID, groupID, buddyGroupID) VALUES (?,?,?,?,?,?,?,?,?,?,?);";

   dbHandle->sqliteBlockingExec("BEGIN TRANSACTION");

   sqlRes = dbHandle->sqliteBlockingPrepare(queryStr.c_str(),
      strlen(queryStr.c_str()), &stmt, &sqlTail);
   if ( sqlRes != SQLITE_OK )
   {
      log.log(3,
         "SQL Error; Unable to prepare SQL statement for inserting chunks; Aborting execution;"
            " SQLite Error Code: " + StringTk::intToStr(sqlRes));
      throw FsckDBException("Unable to prepare SQL statement for chunks");
   }

   // NOTE : Keep this in sync with struct FsckChunk and table structure
   FsckChunkListIter iter = chunks.begin();
   for ( ; iter != chunks.end(); iter++ )
   {
      std::string usedBlocksString;
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

      sqlRes = sqlite3_bind_text(stmt, index++, iter->getSavedPath()->getPathAsStr().c_str(),
         -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getFileSize());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      // usedBlocks is uint64_t, but sqlite int type can't handle uint64; therefore we save this
      // as text
      usedBlocksString = StringTk::uint64ToStr(iter->getUsedBlocks());

      sqlRes = sqlite3_bind_text(stmt, index++, usedBlocksString.c_str(), -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getCreationTime());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getModificationTime());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getLastAccessTime());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getUserID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getGroupID());
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

      sqlRes = dbHandle->sqliteBlockingStep(stmt);
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

   dbHandle->sqliteBlockingExec("COMMIT");

   if ( releaseHandle )
      dbHandlePool->releaseHandle(dbHandle);

   if (!retVal)
   {
      outFailedInsert = *iter;
      outErrorCode = sqlRes;
      log.log(3,
         "SQL Error; Unable to insert chunk; entryID: " + iter->getID()
         + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
   }

   return retVal;
}

/*
 * inserts a list of content directories into the database
 *
 * @param chunks the list of chunks to insert
 * @param outFailedInsert reference to a cont dir, which will be set if a insert fails
 * @param outErrorCode reference to an int, which will be set to sqlite error code according to outFailedInsert
 * @param dbHandle a pointer to the dbHandle to use; can be NULL, in which case a new handle is acquired
 *
 * @return true if no failure occured, false otherwise
 */
bool FsckDB::insertContDirs(FsckContDirList& contDirs, FsckContDir& outFailedInsert,
   int& outErrorCode, DBHandle* dbHandle)
{
   bool releaseHandle = false;
   bool retVal = true;
   std::string tableName = TABLE_NAME_CONT_DIRS;

   sqlite3_stmt *stmt;
   const char *sqlTail; // uncompiled part of statement, should never have anything inside
   int sqlRes;

   if ( !dbHandle )
   {
      releaseHandle = true;
      dbHandle = this->dbHandlePool->acquireHandle();
   }

   // prepare statement
   std::string queryStr = "INSERT INTO " + tableName + " (id, saveNodeID) VALUES (?,?);";

   dbHandle->sqliteBlockingExec("BEGIN TRANSACTION");

   sqlRes = dbHandle->sqliteBlockingPrepare(queryStr.c_str(),
      strlen(queryStr.c_str()), &stmt, &sqlTail);
   if ( sqlRes != SQLITE_OK )
   {
      log.log(3,
         "SQL Error; Unable to prepare SQL statement for inserting content directories; "
         "Aborting execution; SQLite Error Code: " + StringTk::intToStr(sqlRes));
      throw FsckDBException("Unable to prepare SQL statement for content directories");
   }

   // NOTE : Keep this in sync with struct FsckContDir and table structure
   FsckContDirListIter iter = contDirs.begin();
   for ( ; iter != contDirs.end(); iter++ )
   {
      short index = 1;

      sqlRes = sqlite3_bind_text(stmt, index++, iter->getID().c_str(),
         strlen(iter->getID().c_str()), 0);
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

      sqlRes = dbHandle->sqliteBlockingStep(stmt);
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

   dbHandle->sqliteBlockingExec("COMMIT");

   if ( releaseHandle )
      dbHandlePool->releaseHandle(dbHandle);

   if (!retVal)
   {
      outFailedInsert = *iter;
      outErrorCode = sqlRes;
      log.log(3,
         "SQL Error; Unable to insert content directory; directory ID: " + iter->getID()
         + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
   }

   return retVal;
}

/*
 * inserts a list of FS-ID files into the database
 *
 * @param chunks the list of chunks to insert
 * @param outFailedInsert reference to a fs-id file, which will be set if a insert fails
 * @param outErrorCode reference to an int, which will be set to sqlite error code according to outFailedInsert
 * @param dbHandle a pointer to the dbHandle to use; can be NULL, in which case a new handle is acquired
 *
 * @return true if no failure occured, false otherwise
 */
bool FsckDB::insertFsIDs(FsckFsIDList& fsIDs, FsckFsID& outFailedInsert,
   int& outErrorCode, DBHandle* dbHandle)
{
   bool releaseHandle = false;
   bool retVal = true;
   std::string tableName = TABLE_NAME_FSIDS;

   sqlite3_stmt *stmt;
   const char *sqlTail; // uncompiled part of statement, should never have anything inside
   int sqlRes;
   std::string saveInodeStr;

   if ( !dbHandle )
   {
      releaseHandle = true;
      dbHandle = this->dbHandlePool->acquireHandle();
   }

   // prepare statement
   std::string queryStr = "INSERT INTO " + tableName + " (id, parentDirID, saveNodeID, saveDevice, "
      "saveInode) VALUES (?,?,?,?,?);";

   dbHandle->sqliteBlockingExec("BEGIN TRANSACTION");

   sqlRes = dbHandle->sqliteBlockingPrepare(queryStr.c_str(),
      strlen(queryStr.c_str()), &stmt, &sqlTail);
   if ( sqlRes != SQLITE_OK )
   {
      log.log(3,
         "SQL Error; Unable to prepare SQL statement for inserting fs-id-files; Aborting execution;"
            " SQLite Error Code: " + StringTk::intToStr(sqlRes));
      throw FsckDBException("Unable to prepare SQL statement for fs-id-files");
   }

   // NOTE : Keep this in sync with struct FsckFsID and table structure
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

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getSaveDevice());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      // saveInode is uint64_t, but sqlite int type can't handle uint64; therefore we save this
      // as text
      saveInodeStr = StringTk::uint64ToStr(iter->getSaveInode());

      sqlRes = sqlite3_bind_text(stmt, index++, saveInodeStr.c_str(), -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = dbHandle->sqliteBlockingStep(stmt);
      if (sqlRes != SQLITE_DONE)
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_reset(stmt);
      if ( sqlRes != SQLITE_OK )
      {
         log.log(3,
            "SQL Error; Unable to reset SQL statement for fs-id file; Aborting execution; "
            "entryID: " + iter->getID() + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
         throw FsckDBException("Unable to reset SQL statement for content directory");
      }
   }

   sqlite3_finalize(stmt);

   dbHandle->sqliteBlockingExec("COMMIT");

   if ( releaseHandle )
      dbHandlePool->releaseHandle(dbHandle);

   if (!retVal)
   {
      outFailedInsert = *iter;
      outErrorCode = sqlRes;
      log.log(3,
         "SQL Error; Unable to insert fs-id file; entryID: " + iter->getID()
         + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
   }

   return retVal;
}

/*
 * inserts a list of used storage targets into the database
 *
 * @param targetIDs the list of targetIDs to insert
 * @param outFailedInsert reference to a target id, which will be set if a insert fails
 * @param outErrorCode reference to an int, which will be set to sqlite error code according to
 * outFailedInsert
 * @param dbHandle a pointer to the dbHandle to use; can be NULL, in which case a new handle is
 * acquired
 *
 * @return true if no failure occured, false otherwise
 */
bool FsckDB::insertUsedTargetIDs(FsckTargetIDList& targetIDs, FsckTargetID& outFailedInsert,
   int& outErrorCode, DBHandle* dbHandle)
{
   bool releaseHandle = false;
   bool retVal = true;
   std::string tableName = TABLE_NAME_USED_TARGETS;

   sqlite3_stmt* stmt;
   const char *sqlTail; // uncompiled part of statement, should never have anything inside
   int sqlRes;

   if ( !dbHandle )
   {
      releaseHandle = true;
      dbHandle = this->dbHandlePool->acquireHandle();
   }

   // prepare statements
   std::string queryStr = "INSERT INTO " + tableName + " (id, targetIDType) VALUES (?,?);";

   dbHandle->sqliteBlockingExec("BEGIN TRANSACTION");

   sqlRes = dbHandle->sqliteBlockingPrepare(queryStr.c_str(),
      strlen(queryStr.c_str()), &stmt, &sqlTail);
   if ( sqlRes != SQLITE_OK )
   {
      log.log(3,
         "SQL Error; Unable to prepare SQL statement for inserting used storage targets; "
         "Aborting execution; SQLite Error Code: " + StringTk::intToStr(sqlRes));
      throw FsckDBException("Unable to prepare SQL statement for used storage targets");
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

      sqlRes = sqlite3_bind_int(stmt, index++, (int) iter->getTargetIDType());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = dbHandle->sqliteBlockingStep(stmt);

      if ( (sqlRes != SQLITE_DONE) && (sqlRes != SQLITE_CONSTRAINT) )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_reset(stmt);
      // constraint violations must also be ignored in sqlite3_reset, because if step returned
      // an error, reset will also return that error (see http://www.sqlite.org/c3ref/reset.html)
      if ( (sqlRes != SQLITE_OK) && (sqlRes != SQLITE_CONSTRAINT) )
      {
         log.log(3,
            "SQL Error; Unable to reset SQL statement for used storage target; "
               "Aborting execution; target ID: " + StringTk::uintToStr(iter->getID())
               + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));

         throw FsckDBException("Unable to reset SQL statement for used storage target");
      }

   }

   sqlite3_finalize(stmt);

   dbHandle->sqliteBlockingExec("COMMIT");

   if ( releaseHandle )
      dbHandlePool->releaseHandle(dbHandle);

   if ( !retVal )
   {
      outFailedInsert = *iter;
      outErrorCode = sqlRes;
      log.log(3,
         "SQL Error; Unable to insert used storage target; target ID: "
            + StringTk::uintToStr(iter->getID()) + "; SQLite Error Code: "
            + StringTk::intToStr(sqlRes));
   }

   return retVal;
}

/*
 * inserts a list of modification events into the database
 *
 * @param chunks the list of chunks to insert
 * @param outFailedInsert reference to a event, which will be set if a insert fails
 * @param outErrorCode reference to an int, which will be set to sqlite error code according to outFailedInsert
 * @param dbHandle a pointer to the dbHandle to use; can be NULL, in which case a new handle is acquired
 *
 * @return true if no failure occured, false otherwise
 */
bool FsckDB::insertModificationEvents(FsckModificationEventList& events,
   FsckModificationEvent& outFailedInsert, int& outErrorCode, DBHandle* dbHandle)
{
   bool releaseHandle = false;
   bool retVal = true;
   std::string tableName = TABLE_NAME_MODIFICATION_EVENTS;

   sqlite3_stmt *stmt;
   const char *sqlTail; // uncompiled part of statement, should never have anything inside
   int sqlRes;

   if ( !dbHandle )
   {
      releaseHandle = true;
      dbHandle = this->dbHandlePool->acquireHandle();
   }

   // prepare statement
   std::string queryStr = "INSERT INTO " + tableName + " (eventType, entryID) VALUES (?,?);";

   dbHandle->sqliteBlockingExec("BEGIN TRANSACTION");

   sqlRes = dbHandle->sqliteBlockingPrepare(queryStr.c_str(),
      strlen(queryStr.c_str()), &stmt, &sqlTail);
   if ( sqlRes != SQLITE_OK )
   {
      log.log(3,
         "SQL Error; Unable to prepare SQL statement for inserting modification events; "
         "Aborting execution; SQLite Error Code: " + StringTk::intToStr(sqlRes));
      throw FsckDBException("Unable to prepare SQL statement for modification events");
   }

   FsckModificationEventListIter iter = events.begin();
   for ( ; iter != events.end(); iter++ )
   {
      short index = 1;

      sqlRes = sqlite3_bind_int(stmt, index++, (int)iter->getEventType());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_text(stmt, index++, iter->getEntryID().c_str(), -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = dbHandle->sqliteBlockingStep(stmt);
      // explicitely ignore sqlite constraint violations here, because we know that the same events
      // will most likely be inserted multiple times
      if ( (sqlRes != SQLITE_DONE) && (sqlRes != SQLITE_CONSTRAINT) )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_reset(stmt);
      // constraint violations must also be ignored in sqlite3_reset, because if step returned
      // an error, reset will also return that error (see http://www.sqlite.org/c3ref/reset.html)
      if ( (sqlRes != SQLITE_OK) && (sqlRes != SQLITE_CONSTRAINT) )
      {
         log.log(3,
            "SQL Error; Unable to reset SQL statement for modification event; entry ID: "
               + iter->getEntryID() + "; eventType: " + StringTk::intToStr(iter->getEventType()) +
               "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
         throw FsckDBException("Unable to reset SQL statement for modification event");
      }
   }

   sqlite3_finalize(stmt);

   dbHandle->sqliteBlockingExec("COMMIT");

   if ( releaseHandle )
      dbHandlePool->releaseHandle(dbHandle);

   if (!retVal)
   {
      outFailedInsert = *iter;
      outErrorCode = sqlRes;
      log.log(3,
         "SQL Error; Unable to insert modification event; entry ID: "
            + iter->getEntryID() + "; eventType: " + StringTk::intToStr(iter->getEventType()) +
            "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
   }

   return retVal;
}

bool FsckDB::updateDirEntries(FsckDirEntryList& dentries, FsckDirEntry& outFailedUpdate,
   int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_DIR_ENTRIES
   ;
   bool retVal = true;
   bool releaseHandle = false;

   sqlite3_stmt* stmt;
   const char *sqlTail; // uncompiled part of statement, should never have anything inside
   int sqlRes;

   /*
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   if ( !dbHandle )
   {
      dbHandle = this->dbHandlePool->acquireHandle();
      releaseHandle = true;
   }

   dbHandle->sqliteBlockingExec("BEGIN TRANSACTION");

   // prepare statement
   std::string queryStr = "UPDATE " + tableName + " SET id=?, entryOwnerNodeID=?, "
      "inodeOwnerNodeID=?, entryType=?, hasInlinedInode=?, saveDevice=?, saveInode=? WHERE "
      "name=? AND parentDirID=? AND saveNodeID=?;";
   sqlRes = dbHandle->sqliteBlockingPrepare(queryStr.c_str(), strlen(queryStr.c_str()),
      &stmt, &sqlTail);
   if ( sqlRes != SQLITE_OK )
   {
      log.log(3,
         "SQL Error; Unable to prepare SQL statement for updating dir entries; Aborting execution;"
            " SQLite Error Code: " + StringTk::intToStr(sqlRes));
      throw FsckDBException("Unable to prepare SQL statement for dir entry");
   }

   // NOTE : Keep this in sync with class FsckDirEntries and table structure
   FsckDirEntryListIter iter = dentries.begin();
   for ( ; iter != dentries.end(); iter++ )
   {
      std::string saveInodeStr;
      short index = 1;

      sqlRes = sqlite3_bind_text(stmt, index++, iter->getID().c_str(), -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getEntryOwnerNodeID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getInodeOwnerNodeID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, (int) iter->getEntryType());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, (int) iter->getHasInlinedInode());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getSaveDevice());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      // saveInode is uint64_t, but sqlite int type can't handle uint64; therefore we save this
      // as text
      saveInodeStr = StringTk::uint64ToStr(iter->getSaveInode());
      sqlRes = sqlite3_bind_text(stmt, index++, saveInodeStr.c_str(), -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

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

      sqlRes = dbHandle->sqliteBlockingStep(stmt);
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

   dbHandle->sqliteBlockingExec("COMMIT");

   if ( releaseHandle )
      dbHandlePool->releaseHandle(dbHandle);

   if ( !retVal )
   {
      outFailedUpdate = *iter;
      outErrorCode = sqlRes;
      log.log(3,
         "SQL Error; Unable to update dir entry; entryID: " + iter->getID()
            + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
   }

   return retVal;
}

bool FsckDB::updateDirInodes(FsckDirInodeList& inodes, FsckDirInode& outFailedUpdate,
   int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_DIR_INODES;
   bool retVal = true;
   bool releaseHandle = false;

   sqlite3_stmt *stmt;
   const char *sqlTail; // uncompiled part of statement, should never have anything inside
   int sqlRes;

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
   std::string queryStr = "UPDATE " + tableName + " SET parentDirID=?, parentNodeID=?, "
      "ownerNodeID=?, size=?, numHardLinks=?, readable=? WHERE id=? AND saveNodeID=?;";

   dbHandle->sqliteBlockingExec("BEGIN TRANSACTION");

   sqlRes = dbHandle->sqliteBlockingPrepare(queryStr.c_str(), strlen(queryStr.c_str()),
      &stmt, &sqlTail);
   if ( sqlRes != SQLITE_OK )
   {
      log.log(3,
         "SQL Error; Unable to prepare SQL statement for updating dir inodes; Aborting execution;"
            " SQLite Error Code: " + StringTk::intToStr(sqlRes));
      throw FsckDBException("Unable to prepare SQL statement for dir inodes");
   }

   // NOTE : Keep this in sync with class FsckDirInodes and table structure
   FsckDirInodeListIter iter = inodes.begin();
   for ( ; iter != inodes.end(); iter++ )
   {
      short index = 1;

      sqlRes = sqlite3_bind_text(stmt, index++, iter->getParentDirID().c_str(), -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getParentNodeID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getOwnerNodeID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getSize());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getNumHardLinks());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, (int) iter->getReadable());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

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

      sqlRes = dbHandle->sqliteBlockingStep(stmt);
      if ( sqlRes != SQLITE_DONE )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_reset(stmt);
      if ( sqlRes != SQLITE_OK )
      {
         log.log(3,
            "SQL Error; Unable to reset SQL statement for dir inode; Aborting execution; entryID: "
               + iter->getID() + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
         throw FsckDBException("Unable to reset SQL statement for dir inode");
      }
   }

   sqlite3_finalize(stmt);

   dbHandle->sqliteBlockingExec("COMMIT");

   if ( releaseHandle )
      dbHandlePool->releaseHandle(dbHandle);

   if ( !retVal )
   {
      outFailedUpdate = *iter;
      outErrorCode = sqlRes;
      log.log(3,
         "SQL Error; Unable to update dir inode; entryID: " + iter->getID()
            + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
   }

   return retVal;
}

bool FsckDB::updateFileInodes(FsckFileInodeList& inodes, FsckFileInode& outFailedUpdate,
   int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_FILE_INODES;
   bool retVal = true;
   bool releaseHandle = false;

   sqlite3_stmt *stmt;
   const char *sqlTail; // uncompiled part of statement, should never have anything inside
   int sqlRes;
   std::string stripeTargetsStr;

   /*
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   if (!dbHandle)
   {
      dbHandle = this->dbHandlePool->acquireHandle();
      releaseHandle = true;
   }

   // prepare statement
   std::string queryStr = "UPDATE " + tableName + " SET parentDirID=?, parentNodeID=?, "
      "origParentUID=?, origParentEntryID=?, pathInfoFlags=?, mode=?, userID=?, groupID=?, "
      "fileSize=?, usedBlocks=?, creationTime=?, modificationTime=?, lastAccessTime=?, numHardLinks=?, "
      "stripeTargets=?, stripePatternType=?, chunkSize=?, isInlined=?, readable=? WHERE id=? "
      "AND saveNodeID=?;";

   dbHandle->sqliteBlockingExec("BEGIN TRANSACTION");

   sqlRes = dbHandle->sqliteBlockingPrepare(queryStr.c_str(), strlen(queryStr.c_str()),
      &stmt, &sqlTail);
   if ( sqlRes != SQLITE_OK )
   {
      log.log(3,
         "SQL Error; Unable to prepare SQL statement for updating file inodes; Aborting execution;"
            " SQLite Error Code: " + StringTk::intToStr(sqlRes));
      throw FsckDBException("Unable to prepare SQL statement for file inodes");
   }

   // NOTE : Keep this in sync with class FsckFileInode and table structure
   FsckFileInodeListIter iter = inodes.begin();
   for (; iter != inodes.end(); iter++)
   {
      std::string usedBlocksString;
      short index = 1;

      sqlRes = sqlite3_bind_text(stmt, index++, iter->getParentDirID().c_str(), -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getParentNodeID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      PathInfo* pathInfo = iter->getPathInfo();
      sqlRes = sqlite3_bind_int64(stmt, index++, pathInfo->getOrigUID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_text(stmt, index++, pathInfo->getOrigParentEntryID().c_str(), -1 , 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, pathInfo->getFlags());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getMode());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getUserID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getGroupID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getFileSize());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      // usedBlocks is uint64_t, but sqlite int type can't handle uint64; therefore we save this
      // as text
      usedBlocksString = StringTk::uint64ToStr(iter->getUsedBlocks());

      sqlRes = sqlite3_bind_text(stmt, index++, usedBlocksString.c_str(), -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getCreationTime());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getModificationTime());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getLastAccessTime());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getNumHardLinks());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      // serialize stripe targets to put them into DB
      stripeTargetsStr = StringTk::uint16VecToStr(iter->getStripeTargets());
      sqlRes = sqlite3_bind_text(stmt, index++, stripeTargetsStr.c_str(), -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, (int)iter->getStripePatternType());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, (int)iter->getChunkSize());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, (int)iter->getIsInlined());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, (int)iter->getReadable());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

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

      sqlRes = dbHandle->sqliteBlockingStep(stmt);
      if ( sqlRes != SQLITE_DONE )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_reset(stmt);
      if ( sqlRes != SQLITE_OK )
      {
         log.log(3,
            "SQL Error; Unable to reset SQL statement for file inode; Aborting execution; entryID: "
               + iter->getID() + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
         throw FsckDBException("Unable to reset SQL statement for file inode");
      }
   }

   sqlite3_finalize(stmt);

   dbHandle->sqliteBlockingExec("COMMIT");

   if ( releaseHandle )
      dbHandlePool->releaseHandle(dbHandle);

   if ( !retVal )
   {
      outFailedUpdate = *iter;
      outErrorCode = sqlRes;
      log.log(3,
         "SQL Error; Unable to update file inode; entryID: " + iter->getID()
            + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
   }

   return retVal;
}

bool FsckDB::updateChunks(FsckChunkList& chunks, FsckChunk& outFailedUpdate,
   int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_CHUNKS;
   bool retVal = true;
   bool releaseHandle = false;

   sqlite3_stmt *stmt;
   const char *sqlTail; // uncompiled part of statement, should never have anything inside
   int sqlRes;

   /*
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   if (!dbHandle)
   {
      dbHandle = this->dbHandlePool->acquireHandle();
      releaseHandle = true;
   }

   // prepare statement
   std::string queryStr = "UPDATE " + tableName + " SET savedPath=?, fileSize=?, usedBlocks=?, "
      "creationTime=?, modificationTime=?, lastAccessTime=?, userID=?, groupID=?, buddyGroupID=? "
      "WHERE id=? AND targetID=?;";

   dbHandle->sqliteBlockingExec("BEGIN TRANSACTION");

   sqlRes = dbHandle->sqliteBlockingPrepare(queryStr.c_str(), strlen(queryStr.c_str()),
      &stmt, &sqlTail);
   if ( sqlRes != SQLITE_OK )
   {
      log.log(3,
         "SQL Error; Unable to prepare SQL statement for updating chunks; Aborting execution;"
            " SQLite Error Code: " + StringTk::intToStr(sqlRes));
      throw FsckDBException("Unable to prepare SQL statement for chunks");
   }

   // NOTE : Keep this in sync with class FsckFileInode and table structure
   FsckChunkListIter iter = chunks.begin();
   for ( ; iter != chunks.end(); iter++ )
   {
      std::string usedBlocksString;
      short index = 1;

      sqlRes = sqlite3_bind_text(stmt, index++, iter->getSavedPath()->getPathAsStr().c_str(),
         -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getFileSize());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      // usedBlocks is uint64_t, but sqlite int type can't handle uint64; therefore we save this
      // as text
      usedBlocksString = StringTk::uint64ToStr(iter->getUsedBlocks());

      sqlRes = sqlite3_bind_text(stmt, index++, usedBlocksString.c_str(), -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getCreationTime());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getModificationTime());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getLastAccessTime());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getUserID());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int64(stmt, index++, iter->getGroupID());
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

      sqlRes = dbHandle->sqliteBlockingStep(stmt);
      if ( sqlRes != SQLITE_DONE )
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

   dbHandle->sqliteBlockingExec("COMMIT");

   if ( releaseHandle )
      dbHandlePool->releaseHandle(dbHandle);

   if ( !retVal )
   {
      outFailedUpdate = *iter;
      outErrorCode = sqlRes;
      log.log(3,
         "SQL Error; Unable to update chunk; entryID: " + iter->getID()
            + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
   }

   return retVal;
}

bool FsckDB::updateFsIDs(FsckFsIDList& fsIDs, FsckFsID& outFailedUpdate,
   int& outErrorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_FSIDS;
   bool retVal = true;
   bool releaseHandle = false;

   sqlite3_stmt *stmt;
   const char *sqlTail; // uncompiled part of statement, should never have anything inside
   int sqlRes;

   /*
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   if (!dbHandle)
   {
      dbHandle = this->dbHandlePool->acquireHandle();
      releaseHandle = true;
   }

   // prepare statement
   std::string queryStr = "UPDATE " + tableName + " SET parentDirID=?, saveDevice=?, "
      "saveInode=?, WHERE id=? AND saveNodeID=?;";

   dbHandle->sqliteBlockingExec("BEGIN TRANSACTION");

   sqlRes = dbHandle->sqliteBlockingPrepare(queryStr.c_str(), strlen(queryStr.c_str()),
      &stmt, &sqlTail);
   if ( sqlRes != SQLITE_OK )
   {
      log.log(3,
         "SQL Error; Unable to prepare SQL statement for updating fs-id files; Aborting execution;"
            " SQLite Error Code: " + StringTk::intToStr(sqlRes));
      throw FsckDBException("Unable to prepare SQL statement for fs-id file");
   }

   // NOTE : Keep this in sync with class FsckFsID and table structure
   FsckFsIDListIter iter = fsIDs.begin();
   for (; iter != fsIDs.end(); iter++)
   {
      short index = 1;
      std::string saveInodeStr;

      sqlRes = sqlite3_bind_text(stmt, index++, iter->getParentDirID().c_str(), -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_bind_int(stmt, index++, iter->getSaveDevice());
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

      // uint64 is too long for sqlite, therefore we need to save as text
      saveInodeStr = StringTk::uint64ToStr(iter->getSaveInode());
      sqlRes = sqlite3_bind_text(stmt, index++, saveInodeStr.c_str(), -1, 0);
      if ( sqlRes != SQLITE_OK )
      {
         retVal = false;
         break;
      }

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

      sqlRes = dbHandle->sqliteBlockingStep(stmt);
      if ( sqlRes != SQLITE_DONE )
      {
         retVal = false;
         break;
      }

      sqlRes = sqlite3_reset(stmt);
      if ( sqlRes != SQLITE_OK )
      {
         log.log(3,
            "SQL Error; Unable to reset SQL statement for fs-id file; Aborting execution; entryID: "
               + iter->getID() + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
         throw FsckDBException("Unable to reset SQL statement for fs-id file");
      }
   }

   sqlite3_finalize(stmt);

   dbHandle->sqliteBlockingExec("COMMIT");

   if ( releaseHandle )
      dbHandlePool->releaseHandle(dbHandle);

   if ( !retVal )
   {
      outFailedUpdate = *iter;
      outErrorCode = sqlRes;
      log.log(3,
         "SQL Error; Unable to update fs-id file; entryID: " + iter->getID()
            + "; SQLite Error Code: " + StringTk::intToStr(sqlRes));
   }

   return retVal;
}

/*
 * replace a targetID in a set of fileInodes in the database
 *
 * @param oldTargetID
 * @param newTargetID
 * @param inodes a list of file inodes; only inodes matching one of these IDs will get updated;
 * may be NULL (and defaults to NULL), in which case all inodes are updated
 * @param dbHandle
 *
 */
bool FsckDB::replaceStripeTarget(uint16_t oldTargetID, uint16_t newTargetID,
   FsckFileInodeList* inodes, DBHandle* dbHandle)
{
   bool retVal = true;

   std::string tableName = TABLE_NAME_FILE_INODES;

   std::string whereClause;

   if ( !inodes )
      whereClause = "1";
   else
   if ( inodes->empty() ) // nothing to do
      return true;
   else
   {
      std::string idListStr;
      for ( FsckFileInodeListIter iter = inodes->begin(); iter != inodes->end(); iter++ )
         idListStr += "'" + iter->getID() + "',";

      // resize string to strip the trailing delimiter
      idListStr.resize(idListStr.length() - 1);

      whereClause = "id IN (" + idListStr + ")";
   }

   std::string queryStr = "UPDATE " + tableName
      + " SET stripeTargets = TRIM(REPLACE(',' || stripeTargets || ',', '," +
      StringTk::uintToStr(oldTargetID) + ",','," + StringTk::uintToStr(newTargetID) + ",'),',') "
         "WHERE " + whereClause;

   bool releaseHandle = false;

   /*
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   if ( !dbHandle )
   {
      dbHandle = this->dbHandlePool->acquireHandle();
      releaseHandle = true;
   }

   dbHandle->sqliteBlockingExec("BEGIN TRANSACTION");

   int sqlRes = dbHandle->sqliteBlockingExec(queryStr);

   if ( sqlRes != SQLITE_OK )
      retVal = false;

   dbHandle->sqliteBlockingExec("COMMIT");

   if ( releaseHandle )
      dbHandlePool->releaseHandle(dbHandle);

   return retVal;
}

/*
 * returns a SQL query string to insert key,value pairs into the database
 *
 * @param fields the key,value pairs to insert
 * @return a string to use in a sql query
 */
/*
std::string FsckDB::buildInsertSql(std::string& tableName, StringMap& fields)
{
   if ( fields.empty() )
      return "";

   std::string keyStr = "";
   std::string valuesStr = "";

   for (StringMapIter fieldsIter = fields.begin(); fieldsIter != fields.end(); fieldsIter++)
   {
      keyStr += fieldsIter->first + ",";
      valuesStr += "'" + fieldsIter->second + "',";
   }

   keyStr = keyStr.substr(0, keyStr.length()-1 );
   valuesStr = valuesStr.substr(0, valuesStr.length()-1 );

   std::string query = "INSERT INTO " + tableName + " (" + keyStr + ") VALUES(" + valuesStr + ");";

   return query;
}
*/

/*
 * returns a SQL query string to update key,value pairs into the database
 *
 * @param tableName the table to use
 * @param updateFields the key,value pairs to insert
 * @param whereFields a key,value map from which the SQL 'where clause' is built
 *
 * @return a string to use in a sql query
 */
/*
std::string FsckDB::buildUpdateSql(std::string& tableName, StringMap& updateFields,
   StringMap& whereFields)
{
   if ( updateFields.empty() )
      return "";

   if ( whereFields.empty() )
   {
      log.log(Log_DEBUG, "An update SQL statement cannot be built without a where clause.");
      return "";
   }

   StringMapIter whereIter = whereFields.begin();

   std::string whereClause = "WHERE " + whereIter->first + "='" + whereIter->second + "'";

   while (++whereIter != whereFields.end())
   {
      whereClause += " AND " + whereIter->first + "='" + whereIter->second + "'";
   }

   std::string setStr = "";

   for (StringMapIter updateIter = updateFields.begin(); updateIter != updateFields.end();
      updateIter++)
   {
      std::string key = updateIter->first;
      std::string value = updateIter->second;
      setStr += key + "='" + value + "',";
   }

   setStr = setStr.substr(0, setStr.length()-1 );

   std::string query = "UPDATE " + tableName + " SET " + setStr + " " + whereClause;

   return query;
}
*/
bool FsckDB::processModificationEvents()
{
   bool retVal = true;

   std::string tableModEvents = TABLE_NAME_MODIFICATION_EVENTS;

   std::string whereClause = "id IN (SELECT entryID FROM " + tableModEvents + ")";

   bool addRes;
   std::string tableName;

   tableName = TABLE_NAME_DIR_ENTRIES;
   addRes = this->addIgnoreAllErrorCodes(tableName, whereClause);

   if (!addRes)
   {
      log.logErr("Unable to process modification events. Table: " + tableName);
      retVal = false;
   }

   tableName = TABLE_NAME_FILE_INODES;
   addRes = this->addIgnoreAllErrorCodes(tableName, whereClause);

   if (!addRes)
   {
      log.logErr("Unable to process modification events. Table: " + tableName);
      retVal = false;
   }

   tableName = TABLE_NAME_DIR_INODES;
   addRes = this->addIgnoreAllErrorCodes(tableName, whereClause);

   if (!addRes)
   {
      log.logErr("Unable to process modification events. Table: " + tableName);
      retVal = false;
   }

   tableName = TABLE_NAME_CHUNKS;
   addRes = this->addIgnoreAllErrorCodes(tableName, whereClause);

   if (!addRes)
   {
      log.logErr("Unable to process modification events. Table: " + tableName);
      retVal = false;
   }

   tableName = TABLE_NAME_CONT_DIRS;
   addRes = this->addIgnoreAllErrorCodes(tableName, whereClause);

   if (!addRes)
   {
      log.logErr("Unable to process modification events. Table: " + tableName);
      retVal = false;
   }

   tableName = TABLE_NAME_FSIDS;
   addRes = this->addIgnoreAllErrorCodes(tableName, whereClause);

   if (!addRes)
   {
      log.logErr("Unable to process modification events. Table: " + tableName);
      retVal = false;
   }

   return retVal;
}

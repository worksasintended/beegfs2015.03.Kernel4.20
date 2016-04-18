#include "FsckDB.h"

#include <toolkit/FsckTkEx.h>

bool FsckDB::addIgnoreErrorCode(FsckDirEntry& dirEntry, FsckErrorCode errorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_DIR_ENTRIES;

   std::string whereClause = "id='" + dirEntry.getID() + "' AND saveNodeID='"
      + StringTk::uintToStr(dirEntry.getSaveNodeID()) + "'";

   return this->addIgnoreErrorCode(tableName, whereClause, errorCode, dbHandle);
}

bool FsckDB::addIgnoreAllErrorCodes(FsckDirEntry& dirEntry, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_DIR_ENTRIES;

   std::string whereClause = "id='" + dirEntry.getID() + "' AND saveNodeID='"
      + StringTk::uintToStr(dirEntry.getSaveNodeID()) + "'";

   return this->addIgnoreAllErrorCodes(tableName, whereClause, dbHandle);
}

bool FsckDB::addIgnoreErrorCode(FsckFileInode& fileInode, FsckErrorCode errorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_FILE_INODES;

   std::string whereClause = "id='" + fileInode.getID() + "' AND saveNodeID='"
      + StringTk::uintToStr(fileInode.getSaveNodeID()) + "'";

   return this->addIgnoreErrorCode(tableName, whereClause, errorCode, dbHandle);
}

bool FsckDB::addIgnoreAllErrorCodes(FsckFileInode& fileInode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_FILE_INODES;

   std::string whereClause = "id='" + fileInode.getID() + "' AND saveNodeID='"
      + StringTk::uintToStr(fileInode.getSaveNodeID()) + "'";

   return this->addIgnoreAllErrorCodes(tableName, whereClause, dbHandle);
}

bool FsckDB::addIgnoreErrorCode(FsckDirInode& dirInode, FsckErrorCode errorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_DIR_INODES;

   std::string whereClause = "id='" + dirInode.getID() + "' AND saveNodeID='"
      + StringTk::uintToStr(dirInode.getSaveNodeID()) + "'";

   return this->addIgnoreErrorCode(tableName, whereClause, errorCode, dbHandle);
}

bool FsckDB::addIgnoreAllErrorCodes(FsckDirInode& dirInode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_DIR_INODES;

   std::string whereClause = "id='" + dirInode.getID() + "' AND saveNodeID='"
      + StringTk::uintToStr(dirInode.getSaveNodeID()) + "'";

   return this->addIgnoreAllErrorCodes(tableName, whereClause, dbHandle);
}

bool FsckDB::addIgnoreErrorCode(FsckChunk& chunk, FsckErrorCode errorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_CHUNKS;

   std::string whereClause = "id='" + chunk.getID() + "' AND targetID='"
      + StringTk::uintToStr(chunk.getTargetID()) + "'";

   return this->addIgnoreErrorCode(tableName, whereClause, errorCode, dbHandle);
}

bool FsckDB::addIgnoreAllErrorCodes(FsckChunk& chunk, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_CHUNKS;

   std::string whereClause = "id='" + chunk.getID() + "' AND targetID='"
      + StringTk::uintToStr(chunk.getTargetID()) + "'";

   return this->addIgnoreAllErrorCodes(tableName, whereClause, dbHandle);
}

bool FsckDB::addIgnoreErrorCode(FsckContDir& contDir, FsckErrorCode errorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_CONT_DIRS;

   std::string whereClause = "id='" + contDir.getID() + "' AND saveNodeID='"
      + StringTk::uintToStr(contDir.getSaveNodeID()) + "'";

   return this->addIgnoreErrorCode(tableName, whereClause, errorCode, dbHandle);
}

bool FsckDB::addIgnoreAllErrorCodes(FsckContDir& contDir, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_CONT_DIRS;

   std::string whereClause = "id='" + contDir.getID() + "' AND saveNodeID='"
      + StringTk::uintToStr(contDir.getSaveNodeID()) + "'";

   return this->addIgnoreAllErrorCodes(tableName, whereClause, dbHandle);
}

bool FsckDB::addIgnoreErrorCode(FsckFsID& fsID, FsckErrorCode errorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_FSIDS;

   std::string whereClause = "id='" + fsID.getID() + "' AND saveNodeID='"
      + StringTk::uintToStr(fsID.getSaveNodeID()) + "'";

   return this->addIgnoreErrorCode(tableName, whereClause, errorCode, dbHandle);
}

bool FsckDB::addIgnoreAllErrorCodes(FsckFsID& fsID, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_CONT_DIRS;

   std::string whereClause = "id='" + fsID.getID() + "' AND parentDirID='" + fsID.getParentDirID()
      + "' AND saveNodeID='" + StringTk::uintToStr(fsID.getSaveNodeID()) + "'";

   return this->addIgnoreAllErrorCodes(tableName, whereClause, dbHandle);
}

bool FsckDB::addIgnoreErrorCode(FsckTargetID& targetID, FsckErrorCode errorCode, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_USED_TARGETS;

   std::string whereClause = "id='" + StringTk::uintToStr(targetID.getID())
      + "' AND targetIDType = '" + StringTk::uintToStr((int) targetID.getTargetIDType()) + "'";

   return this->addIgnoreErrorCode(tableName, whereClause, errorCode, dbHandle);
}

bool FsckDB::addIgnoreAllErrorCodes(FsckTargetID& targetID, DBHandle* dbHandle)
{
   std::string tableName = TABLE_NAME_USED_TARGETS;

   std::string whereClause = "id='" + StringTk::uintToStr(targetID.getID())
       + "' AND targetIDType = '" + StringTk::uintToStr((int) targetID.getTargetIDType()) + "'";

   return this->addIgnoreAllErrorCodes(tableName, whereClause, dbHandle);
}

/*
 * set a repairAction for exactly one dir entry with a certain error code
 *
 * @param dirEntry the dir entry to update
 * @param errorCode the error code of the element;according to this value the DB table will be determined
 * @param repairAction the repair action to be set
 * @param dbHandle a pointer to the dbHandle to use; can be NULL, in which case a new handle is aquired
 *
 * @return a bool indicating success or failure
 */
bool FsckDB::setRepairAction(FsckDirEntry& dirEntry, FsckErrorCode errorCode,
   FsckRepairAction repairAction, DBHandle* dbHandle)
{
   FsckDirEntryList list;
   list.push_back(dirEntry);

   if (setRepairAction(list, errorCode, repairAction, NULL, NULL, dbHandle) > 0)
      return false;
   else
      return true;
}

/*
 * set a repairAction for a set of dirEntries with a certain error code
 *
 * @param dentries the list of dentries to update
 * @errorCode the error code of the elemnts;according to this value the DB table will be determined
 * @repairAction the repair action to be set
 * @param outFailedUpdates a list of dentries which failed to be updated (can be NULL)
 * @param outErrorCodes a list of SQLErrorCodes (sorted according to failedUpdates; can be NULL)
 *
 * @return an unsigned int which represents the amount of failed updates
 */
unsigned FsckDB::setRepairAction(FsckDirEntryList& dentries, FsckErrorCode errorCode,
   FsckRepairAction repairAction, FsckDirEntryList* outFailedUpdates, IntList* outErrorCodes,
   DBHandle* dbHandle)
{
   bool releaseHandle = false;
   unsigned amountFailed = 0;
   std::string tableName = TABLE_NAME_ERROR(errorCode);

   /*
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   if (! dbHandle)
   {
      releaseHandle = true;
      dbHandle = this->dbHandlePool->acquireHandle();
   }

   // take all into one transaction (SQLite would treat every INSERT as transaction otherwise)
   std::string queryStr = "BEGIN TRANSACTION;";
   dbHandle->sqliteBlockingExec(queryStr);

   for (FsckDirEntryListIter iter = dentries.begin(); iter != dentries.end(); iter++)
   {
      FsckDirEntry element = *iter;

      std::string repairActionStr = StringTk::intToStr((int)repairAction);
      queryStr = "UPDATE " + tableName + " SET repairAction='" + repairActionStr +
         "' WHERE name='" + element.getName() + "' AND parentDirID='" + element.getParentDirID()
         +"' AND saveNodeID=" + StringTk::uintToStr(element.getSaveNodeID()) + ";";

      int sqlRes = dbHandle->sqliteBlockingExec(queryStr);
      if ( sqlRes != SQLITE_OK )
      {
         amountFailed++;
         if (outFailedUpdates)
            outFailedUpdates->push_back(*iter);
         if (outErrorCodes)
            outErrorCodes->push_back(sqlRes);
      }
   }

   queryStr = "COMMIT;";
   dbHandle->sqliteBlockingExec(queryStr);

   if (releaseHandle)
      dbHandlePool->releaseHandle(dbHandle);

   return amountFailed;
}

/*
 * set a repairAction for exactly one file inodes with a certain error code
 *
 * @param fileInode the fileInode to update
 * @param errorCode the error code of the element;according to this value the DB table will be determined
 * @param repairAction the repair action to be set
 * @param dbHandle a pointer to the dbHandle to use; can be NULL, in which case a new handle is aquired
 *
 * @return a bool indicating success or failure
 */
bool FsckDB::setRepairAction(FsckFileInode& fileInode, FsckErrorCode errorCode,
   FsckRepairAction repairAction, DBHandle* dbHandle)
{
   FsckFileInodeList list;
   list.push_back(fileInode);

   if (setRepairAction(list, errorCode, repairAction, NULL, NULL, dbHandle) > 0)
      return false;
   else
      return true;
}

/*
 * set a repairAction for a set of file inodes with a certain error code
 *
 * @param fileInodes the list of fileInodes to update
 * @errorCode the error code of the elemnts;according to this value the DB table will be determined
 * @repairAction the repair action to be set
 * @param outFailedUpdates a list of fileInodes which failed to be updated (can be NULL)
 * @param outErrorCodes a list of SQLErrorCodes (sorted according to failedUpdates; can be NULL)
 * @param dbHandle a pointer to the dbHandle to use; can be NULL, in which case a new handle is aquired
 *
 *
 * @return an unsigned int which represents the amount of failed updates
 */
unsigned FsckDB::setRepairAction(FsckFileInodeList& fileInodes, FsckErrorCode errorCode,
   FsckRepairAction repairAction, FsckFileInodeList* outFailedUpdates, IntList* outErrorCodes,
   DBHandle* dbHandle)
{
   bool releaseHandle = false;

   unsigned amountFailed = 0;
   std::string tableName = TABLE_NAME_ERROR(errorCode);

   /*
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   if (! dbHandle)
   {
      releaseHandle = true;
      dbHandle = this->dbHandlePool->acquireHandle();
   }

   // take all into one transaction (SQLite would treat every INSERT as transaction otherwise)
   std::string queryStr = "BEGIN TRANSACTION;";
   dbHandle->sqliteBlockingExec(queryStr);

   for (FsckFileInodeListIter iter = fileInodes.begin(); iter != fileInodes.end(); iter++)
   {
      FsckFileInode element = *iter;

      std::string repairActionStr = StringTk::intToStr((int)repairAction);
      queryStr = "UPDATE " + tableName + " SET repairAction='" + repairActionStr
         + "' WHERE internalID='" + StringTk::uint64ToStr(element.getInternalID()) + "';";

      int sqlRes = dbHandle->sqliteBlockingExec(queryStr);
      if ( sqlRes != SQLITE_OK )
      {
         amountFailed++;
         if (outFailedUpdates)
            outFailedUpdates->push_back(*iter);
         if (outErrorCodes)
            outErrorCodes->push_back(sqlRes);
      }
   }

   queryStr = "COMMIT;";
   dbHandle->sqliteBlockingExec(queryStr);

   if (releaseHandle)
      dbHandlePool->releaseHandle(dbHandle);

   return amountFailed;
}

/*
 * set a repairAction for exactly one dir inode with a certain error code
 *
 * @param dirInode the dir inode to update
 * @param errorCode the error code of the element;according to this value the DB table will be determined
 * @param repairAction the repair action to be set
 * @param dbHandle a pointer to the dbHandle to use; can be NULL, in which case a new handle is aquired
 *
 * @return a bool indicating success or failure
 */
bool FsckDB::setRepairAction(FsckDirInode& dirInode, FsckErrorCode errorCode,
   FsckRepairAction repairAction, DBHandle* dbHandle)
{
   FsckDirInodeList list;
   list.push_back(dirInode);

   if (setRepairAction(list, errorCode, repairAction, NULL, NULL, dbHandle) > 0)
      return false;
   else
      return true;
}

/*
 * set a repairAction for a set of dir inodes with a certain error code
 *
 * @param dirInodes the list of dir inodes to update
 * @errorCode the error code of the elemnts;according to this value the DB table will be determined
 * @repairAction the repair action to be set
 * @param outFailedUpdates a list of dir inodes which failed to be updated (can be NULL)
 * @param outErrorCodes a list of SQLErrorCodes (sorted according to failedUpdates; can be NULL)
 *
 * @return an unsigned int which represents the amount of failed updates
 */
unsigned FsckDB::setRepairAction(FsckDirInodeList& dirInodes, FsckErrorCode errorCode,
   FsckRepairAction repairAction, FsckDirInodeList* outFailedUpdates, IntList* outErrorCodes,
   DBHandle* dbHandle)
{
   bool releaseHandle = false;
   unsigned amountFailed = 0;
   std::string tableName = TABLE_NAME_ERROR(errorCode);

   /*
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   if (! dbHandle)
   {
      releaseHandle = true;
      dbHandle = this->dbHandlePool->acquireHandle();
   }

   // take all into one transaction (SQLite would treat every INSERT as transaction otherwise)
   std::string queryStr = "BEGIN TRANSACTION;";
   dbHandle->sqliteBlockingExec(queryStr);

   for (FsckDirInodeListIter iter = dirInodes.begin(); iter != dirInodes.end(); iter++)
   {
      FsckDirInode element = *iter;

      std::string repairActionStr = StringTk::intToStr((int)repairAction);
      queryStr = "UPDATE " + tableName + " SET repairAction='" + repairActionStr + "' WHERE id='" +
         element.getID() + "';";

      int sqlRes = dbHandle->sqliteBlockingExec(queryStr);
      if ( sqlRes != SQLITE_OK )
      {
         amountFailed++;
         if (outFailedUpdates)
            outFailedUpdates->push_back(*iter);
         if (outErrorCodes)
            outErrorCodes->push_back(sqlRes);
      }
   }

   queryStr = "COMMIT;";
   dbHandle->sqliteBlockingExec(queryStr);

   if (releaseHandle)
      dbHandlePool->releaseHandle(dbHandle);

   return amountFailed;
}

/*
 * set a repairAction for exactly one chunk with a certain error code
 *
 * @param chunk the chunk to update
 * @param errorCode the error code of the element;according to this value the DB table will be determined
 * @param repairAction the repair action to be set
 * @param dbHandle a pointer to the dbHandle to use; can be NULL, in which case a new handle is aquired
 *
 * @return a bool indicating success or failure
 */
bool FsckDB::setRepairAction(FsckChunk& chunk, FsckErrorCode errorCode,
   FsckRepairAction repairAction, DBHandle* dbHandle)
{
   FsckChunkList list;
   list.push_back(chunk);

   if (setRepairAction(list, errorCode, repairAction, NULL, NULL, dbHandle) > 0)
      return false;
   else
      return true;
}

/*
 * set a repairAction for a set of chunks with a certain error code
 *
 * @param chunks the list of chunks to update
 * @errorCode the error code of the elements;according to this value the DB table will be determined
 * @repairAction the repair action to be set
 * @param outFailedUpdates a list of chunks which failed to be updated (can be NULL)
 * @param outErrorCodes a list of SQLErrorCodes (sorted according to failedUpdates; can be NULL)
 *
 * @return an unsigned int which represents the amount of failed updates
 */
unsigned FsckDB::setRepairAction(FsckChunkList& chunks, FsckErrorCode errorCode,
   FsckRepairAction repairAction, FsckChunkList* outFailedUpdates, IntList* outErrorCodes,
   DBHandle* dbHandle)
{
   bool releaseHandle = false;

   unsigned amountFailed = 0;
   std::string tableName = TABLE_NAME_ERROR(errorCode);

   /*
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   if (! dbHandle)
   {
      releaseHandle = true;
      dbHandle = this->dbHandlePool->acquireHandle();
   }

   // take all into one transaction (SQLite would treat every INSERT as transaction otherwise)
   std::string queryStr = "BEGIN TRANSACTION;";
   dbHandle->sqliteBlockingExec(queryStr);

   for (FsckChunkListIter iter = chunks.begin(); iter != chunks.end(); iter++)
   {
      FsckChunk element = *iter;

      std::string repairActionStr = StringTk::intToStr((int)repairAction);

      queryStr = "UPDATE " + tableName + " SET repairAction='" + repairActionStr + "' WHERE id='" +
         element.getID() + "' AND targetID=" + StringTk::uintToStr(element.getTargetID()) + ";";

      int sqlRes = dbHandle->sqliteBlockingExec(queryStr);
      if ( sqlRes != SQLITE_OK )
      {
         amountFailed++;
         if (outFailedUpdates)
            outFailedUpdates->push_back(*iter);
         if (outErrorCodes)
            outErrorCodes->push_back(sqlRes);
      }
   }

   queryStr = "COMMIT;";
   dbHandle->sqliteBlockingExec(queryStr);

   if (releaseHandle)
      dbHandlePool->releaseHandle(dbHandle);

   return amountFailed;
}

/*
 * set a repairAction for exactly one content directory with a certain error code
 *
 * @param contDir the content directory to update
 * @param errorCode the error code of the element;according to this value the DB table will be determined
 * @param repairAction the repair action to be set
 * @param dbHandle a pointer to the dbHandle to use; can be NULL, in which case a new handle is aquired
 *
 * @return a bool indicating success or failure
 */
bool FsckDB::setRepairAction(FsckContDir& contDir, FsckErrorCode errorCode,
   FsckRepairAction repairAction, DBHandle* dbHandle)
{
   FsckContDirList list;
   list.push_back(contDir);

   if (setRepairAction(list, errorCode, repairAction, NULL, NULL, dbHandle) > 0)
      return false;
   else
      return true;
}

/*
 * set a repairAction for a set of content directories with a certain error code
 *
 * @param contDirs the list of content directories to update
 * @errorCode the error code of the elemnts;according to this value the DB table will be determined
 * @repairAction the repair action to be set
 * @param outFailedUpdates a list of content directories which failed to be updated (can be NULL)
 * @param outErrorCodes a list of SQLErrorCodes (sorted according to failedUpdates; can be NULL)
 *
 * @return an unsigned int which represents the amount of failed updates
 */
unsigned FsckDB::setRepairAction(FsckContDirList& contDirs, FsckErrorCode errorCode,
   FsckRepairAction repairAction, FsckContDirList* outFailedUpdates, IntList* outErrorCodes,
   DBHandle* dbHandle)
{
   bool releaseHandle = false;
   unsigned amountFailed = 0;
   std::string tableName = TABLE_NAME_ERROR(errorCode);

   /*
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   if (! dbHandle)
   {
      releaseHandle = true;
      dbHandle = this->dbHandlePool->acquireHandle();
   }

   // take all into one transaction (SQLite would treat every INSERT as transaction otherwise)
   std::string queryStr = "BEGIN TRANSACTION;";
   dbHandle->sqliteBlockingExec(queryStr);

   for (FsckContDirListIter iter = contDirs.begin(); iter != contDirs.end(); iter++)
   {
      FsckContDir element = *iter;

      std::string repairActionStr = StringTk::intToStr((int)repairAction);
      queryStr = "UPDATE " + tableName + " SET repairAction='" + repairActionStr + "' WHERE id='" +
         element.getID() + "';";

      int sqlRes = dbHandle->sqliteBlockingExec(queryStr);
      if ( sqlRes != SQLITE_OK )
      {
         amountFailed++;
         if (outFailedUpdates)
            outFailedUpdates->push_back(*iter);
         if (outErrorCodes)
            outErrorCodes->push_back(sqlRes);
      }
   }

   queryStr = "COMMIT;";
   dbHandle->sqliteBlockingExec(queryStr);

   if (releaseHandle)
      dbHandlePool->releaseHandle(dbHandle);

   return amountFailed;
}

/*
 * set a repairAction for exactly one dentry-by-ID file with a certain error code
 *
 * @param fsID the ID file to update
 * @param errorCode the error code of the element;according to this value the DB table will be determined
 * @param repairAction the repair action to be set
 * @param dbHandle a pointer to the dbHandle to use; can be NULL, in which case a new handle is aquired
 *
 * @return a bool indicating success or failure
 */
bool FsckDB::setRepairAction(FsckFsID& fsID, FsckErrorCode errorCode,
   FsckRepairAction repairAction, DBHandle* dbHandle)
{
   FsckFsIDList list;
   list.push_back(fsID);

   if (setRepairAction(list, errorCode, repairAction, NULL, NULL, dbHandle) > 0)
      return false;
   else
      return true;
}

/*
 * set a repairAction for a set of dentry-by-ID files with a certain error code
 *
 * @param fsIDs the list of id files to update
 * @errorCode the error code of the elements;according to this value the DB table will be determined
 * @repairAction the repair action to be set
 * @param outFailedUpdates a list of elements which failed to be updated (can be NULL)
 * @param outErrorCodes a list of SQLErrorCodes (sorted according to failedUpdates; can be NULL)
 *
 * @return an unsigned int which represents the amount of failed updates
 */
unsigned FsckDB::setRepairAction(FsckFsIDList& fsIDs, FsckErrorCode errorCode,
   FsckRepairAction repairAction, FsckFsIDList* outFailedUpdates, IntList* outErrorCodes,
   DBHandle* dbHandle)
{
   bool releaseHandle = false;
   unsigned amountFailed = 0;
   std::string tableName = TABLE_NAME_ERROR(errorCode);

   /*
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   if (! dbHandle)
   {
      releaseHandle = true;
      dbHandle = this->dbHandlePool->acquireHandle();
   }

   // take all into one transaction (SQLite would treat every INSERT as transaction otherwise)
   std::string queryStr = "BEGIN TRANSACTION;";
   dbHandle->sqliteBlockingExec(queryStr);

   for (FsckFsIDListIter iter = fsIDs.begin(); iter != fsIDs.end(); iter++)
   {
      FsckFsID element = *iter;

      std::string repairActionStr = StringTk::intToStr((int)repairAction);
      queryStr = "UPDATE " + tableName + " SET repairAction='" + repairActionStr + "' WHERE id='" +
         element.getID() + "' AND parentDirID='" + element.getParentDirID() + "' AND saveNodeID=" +
         StringTk::uintToStr(element.getSaveNodeID()) + ";";

      int sqlRes = dbHandle->sqliteBlockingExec(queryStr);
      if ( sqlRes != SQLITE_OK )
      {
         amountFailed++;
         if (outFailedUpdates)
            outFailedUpdates->push_back(*iter);
         if (outErrorCodes)
            outErrorCodes->push_back(sqlRes);
      }
   }

   queryStr = "COMMIT;";
   dbHandle->sqliteBlockingExec(queryStr);

   if (releaseHandle)
      dbHandlePool->releaseHandle(dbHandle);

   return amountFailed;
}

/*
 * set a repairAction for exactly one target ID with a certain error code
 *
 * @param targetID the targetID to update
 * @param errorCode the error code of the element;according to this value the DB table will be determined
 * @param repairAction the repair action to be set
 * @param dbHandle a pointer to the dbHandle to use; can be NULL, in which case a new handle is aquired
 *
 * @return a bool indicating success or failure
 */
bool FsckDB::setRepairAction(FsckTargetID& targetID, FsckErrorCode errorCode,
   FsckRepairAction repairAction, DBHandle* dbHandle)
{
   FsckTargetIDList list;
   list.push_back(targetID);

   if (setRepairAction(list, errorCode, repairAction, NULL, NULL, dbHandle) > 0)
      return false;
   else
      return true;
}

/*
 * set a repairAction for a set of target IDs with a certain error code
 *
 * @param targetIDs the list of targets to update
 * @errorCode the error code of the elemnts;according to this value the DB table will be determined
 * @repairAction the repair action to be set
 * @param outFailedUpdates a list of target ids which failed to be updated (can be NULL)
 * @param outErrorCodes a list of SQLErrorCodes (sorted according to failedUpdates; can be NULL)
 *
 * @return an unsigned int which represents the amount of failed updates
 */
unsigned FsckDB::setRepairAction(FsckTargetIDList& targetIDs, FsckErrorCode errorCode,
   FsckRepairAction repairAction, FsckTargetIDList* outFailedUpdates, IntList* outErrorCodes,
   DBHandle* dbHandle)
{
   bool releaseHandle = false;
   unsigned amountFailed = 0;
   std::string tableName = TABLE_NAME_ERROR(errorCode);

   /*
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   if (! dbHandle)
   {
      releaseHandle = true;
      dbHandle = this->dbHandlePool->acquireHandle();
   }

   // take all into one transaction (SQLite would treat every INSERT as transaction otherwise)
   std::string queryStr = "BEGIN TRANSACTION;";
   dbHandle->sqliteBlockingExec(queryStr);

   for (FsckTargetIDListIter iter = targetIDs.begin(); iter != targetIDs.end(); iter++)
   {
      std::string repairActionStr = StringTk::intToStr((int)repairAction);
      queryStr = "UPDATE " + tableName + " SET repairAction='" + repairActionStr + "' WHERE id='" +
         StringTk::uintToStr(iter->getID()) + "' AND targetIDType='"
         + StringTk::uintToStr((int)iter->getTargetIDType()) + "';";

      int sqlRes = dbHandle->sqliteBlockingExec(queryStr);
      if ( sqlRes != SQLITE_OK )
      {
         amountFailed++;
         if (outFailedUpdates)
            outFailedUpdates->push_back(*iter);
         if (outErrorCodes)
            outErrorCodes->push_back(sqlRes);
      }
   }

   queryStr = "COMMIT;";
   dbHandle->sqliteBlockingExec(queryStr);

   if (releaseHandle)
      dbHandlePool->releaseHandle(dbHandle);

   return amountFailed;
}

bool FsckDB::clearErrorTable(FsckErrorCode errorCode)
{
   bool retVal = true;

   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_DANGLINGDENTRY);

   std::string queryStr = "DELETE FROM " + tableName + " WHERE 1";

   DBHandle *dbHandle = this->dbHandlePool->acquireHandle();

   if ( dbHandle->sqliteBlockingExec(queryStr) != SQLITE_OK )
      retVal = false;

   this->dbHandlePool->releaseHandle(dbHandle);

   return retVal;
}

/*
 * adds exactly one error Code to the ignore errors column of a given table
 *
 * @param tableName the name of the table
 * @param whereClause the where condition to choose which rowas are updated
 * @param errorCode the error code to add
 *
 * @return boolean to indicate an error
 */
bool FsckDB::addIgnoreErrorCode(std::string tableName, std::string whereClause,
   FsckErrorCode errorCode, DBHandle* dbHandle)
{
   bool retVal = true;
   bool releaseHandle = false;

   std::string queryStr = "UPDATE " + tableName + " SET " + IGNORE_ERRORS_FIELD + "="
      + IGNORE_ERRORS_FIELD + "|" + StringTk::intToStr((int) errorCode) + " WHERE " + whereClause;

   /*
    * take all into one transaction (SQLite would treat every INSERT as transaction otherwise);
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   if ( !dbHandle )
   {
      dbHandle = this->dbHandlePool->acquireHandle();
      releaseHandle = true;
   }

   if ( dbHandle->sqliteBlockingExec(queryStr) != SQLITE_OK )
      retVal = false;

   if ( releaseHandle )
      this->dbHandlePool->releaseHandle(dbHandle);

   return retVal;
}

/*
 * adds all existing error codes to the ignore errors column of a given table (i.e. the entry will
 * be completely ignored in all checks)
 *
 * @param tableName the name of the table
 * @param whereClause the where condition to choose which rowas are updated
 * @return boolean to indicate an error
 */
bool FsckDB::addIgnoreAllErrorCodes(std::string tableName, std::string whereClause,
   DBHandle* dbHandle)
{
   bool retVal = true;
   bool releaseHandle = false;

   // FsckErrorCode_UNDEFINED should be the highest ErrorCode, so this should include all codes
   int allErrorCodes = (int) FsckErrorCode_UNDEFINED | (int) (FsckErrorCode_UNDEFINED - 1);

   std::string queryStr = "UPDATE " + tableName + " SET " + IGNORE_ERRORS_FIELD + "="
      + StringTk::intToStr(allErrorCodes) + " WHERE " + whereClause;

   // perform the query

   /*
    * take all into one transaction (SQLite would treat every INSERT as transaction otherwise);
    * by opening the handle here (and not inside executeQuery), we make sure that we have one and
    * the same handle for the complete transaction
    */
   if ( !dbHandle )
   {
      dbHandle = this->dbHandlePool->acquireHandle();
      releaseHandle = true;
   }

   if ( dbHandle->sqliteBlockingExec(queryStr) != SQLITE_OK )
      retVal = false;

   if ( releaseHandle )
      this->dbHandlePool->releaseHandle(dbHandle);

   return retVal;
}

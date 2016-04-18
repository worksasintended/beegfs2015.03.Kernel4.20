#include "FsckDB.h"

#include <toolkit/FsckTkEx.h>

/*
 * retrieves all dentries in DB; this is a simplified version, which returns a list instead of a
 * cursor; only use it if you are sure that the result is not too big
 *
 * @param outDentries a pointer to a list holding the retrieved entries
 */
void FsckDB::getDirEntries(FsckDirEntryList* outDentries)
{
   DBCursor<FsckDirEntry>* cursor = getDirEntriesInternal();

   FsckDirEntry* currentObject = cursor->open();

   while (currentObject)
   {
      outDentries->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to all dentries in DB
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirEntry>* FsckDB::getDirEntries()
{
   return getDirEntriesInternal();
}

/*
 * retrieves a cursor to all dentries in DB with a given ID
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirEntry>* FsckDB::getDirEntries(std::string id, uint maxCount)
{
   return getDirEntriesInternal("id='" + id + "'", maxCount);
}

/*
 * retrieves all dentries in DB with a given id; this is a simplified version, which returns a
 * list instead of a cursor; only use it if you are sure that the result is not too big
 *
 * @param id
 * @param outDentries a pointer to a list holding the retrieved entries
 */
void FsckDB::getDirEntries(std::string id, FsckDirEntryList* outDentries)
{
   DBCursor<FsckDirEntry>* cursor = getDirEntriesInternal("id='" + id + "'", 0);

   FsckDirEntry* currentObject = cursor->open();

   while (currentObject)
   {
      outDentries->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to all dentries in DB with a given parentID
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirEntry>* FsckDB::getDirEntriesByParentID(std::string parentID)
{
   return getDirEntriesInternal("parentDirID='" + parentID + "'");
}

/*
 * get row count
 *
 * @return the number of rows in the table
 */
int64_t FsckDB::countDirEntries()
{
   std::string tableName = TABLE_NAME_DIR_ENTRIES;
   return this->getRowCount(tableName);
}

/*
 * retrieves all file inodes in DB
 *
 * @param outFileInodes a pointer to a list holding the retrieved inodes
 */
void FsckDB::getFileInodes(FsckFileInodeList* outFileInodes)
{
   DBCursor<FsckFileInode>* cursor = getFileInodesInternal();

   FsckFileInode* currentObject = cursor->open();

   while (currentObject)
   {
      outFileInodes->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to all file inodes in DB
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckFileInode>* FsckDB::getFileInodes()
{
   return getFileInodesInternal();
}

/*
 * get all file inodes saved on a given node, with a given target in their stripe pattern
 *
 * @param nodeID
 * @param fsckTargetID
 *
 * @return a pointer to a FsckFileInode; will be NULL if inode was not found; must be deleted by
 * caller!
 */
DBCursor<FsckFileInode>* FsckDB::getFileInodesWithTarget(uint16_t nodeID,
   FsckTargetID& fsckTargetID)
{
   FsckStripePatternType expectedPatternType;

   switch (fsckTargetID.getTargetIDType())
   {
      case FsckTargetIDType_BUDDYGROUP:
         expectedPatternType = FsckStripePatternType_BUDDYMIRROR;
         break;
      case FsckTargetIDType_TARGET:
         expectedPatternType = FsckStripePatternType_RAID0;
         break;
      default:
         expectedPatternType = FsckStripePatternType_INVALID;
         break;
   }

   // that where clause looks a bit hacky but it is the easist and fastest way to get anything that
   // contains exactly the targetID
   std::string whereClause = "',' || stripeTargets || ',' LIKE '%,"
      + StringTk::uintToStr(fsckTargetID.getID()) + ",%' AND saveNodeID="
      + StringTk::uintToStr(nodeID) + " AND stripePatternType="
      + StringTk::intToStr((int) expectedPatternType);

   return getFileInodesInternal(whereClause);
}

/*
 * get a single file inode by ID
 *
 * @param id the ID of the file inode
 *
 * @return a pointer to a FsckFileInode; will be NULL if inode was not found; must be deleted by
 * caller!
 */
FsckFileInode* FsckDB::getFileInode(std::string id)
{
   FsckFileInode* outInode = NULL;

   DBCursor<FsckFileInode>* cursor = getFileInodesInternal("id='" + id + "'");

   FsckFileInode* currentObject = cursor->open();

   if ( currentObject )
   {
      // currentObject is deleted by the caller, so we need to copy its value to a new object
      outInode = new FsckFileInode();
      *outInode = *currentObject;
   }

   cursor->close();

   SAFE_DELETE(cursor);

   return outInode;
}

/*
 * get row count
 *
 * @return the number of rows in the table
 */
int64_t FsckDB::countFileInodes()
{
   std::string tableName = TABLE_NAME_FILE_INODES;
   return this->getRowCount(tableName);
}

/*
 * retrieves all dir inodes in DB
 *
 * @param outDirInodes a pointer to a list holding the retrieved inodes
 */
void FsckDB::getDirInodes(FsckDirInodeList* outDirInodes)
{
   DBCursor<FsckDirInode>* cursor = getDirInodesInternal();

   FsckDirInode* currentObject = cursor->open();

   while (currentObject)
   {
      outDirInodes->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to all dir inodes in DB
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirInode>* FsckDB::getDirInodes()
{
   return getDirInodesInternal();
}

/*
 * get a single directory inode by ID
 *
 * @param id the ID of the directory inode
 *
 * @return a pointer to a FsckDirInode; will be NULL if inode was not found; must be deleted by
 * caller!
 */
FsckDirInode* FsckDB::getDirInode(std::string id)
{
   FsckDirInode* outInode = NULL;

   DBCursor<FsckDirInode>* cursor = getDirInodesInternal("id='" + id + "'");

   FsckDirInode* currentObject = cursor->open();

   if ( currentObject )
   {
      // currentObject is deleted by the caller, so we need to copy its value to a new object
      outInode = new FsckDirInode();
      *outInode = *currentObject;
   }

   cursor->close();

   SAFE_DELETE(cursor);

   return outInode;
}

/*
 * get row count
 *
 * @return the number of rows in the table
 */
int64_t FsckDB::countDirInodes()
{
   std::string tableName = TABLE_NAME_DIR_INODES;
   return this->getRowCount(tableName);
}

/*
 * retrieves all chunks in DB
 *
 * @param outChunks a pointer to a list holding the retrieved chunks
 */
void FsckDB::getChunks(FsckChunkList* outChunks)
{
   DBCursor<FsckChunk>* cursor = getChunksInternal();

   FsckChunk* currentObject = cursor->open();

   while (currentObject)
   {
      outChunks->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to all chunks in DB
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getChunks()
{
   return getChunksInternal();
}

/*
 * retrieves a cursor to all chunks in DB, with a given chunkID
 *
 * @param chunkID
 * @param onlyPrimary if true, only return chunks, which are no mirror chunks
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getChunks(std::string chunkID, bool onlyPrimary)
{
   // TODO: fix that!
   return getChunksInternal("id='" + chunkID + "'");
/*   if (onlyPrimary)
      return getChunksInternal("id='" + chunkID + "' AND mirrorOf=0" );
   else
      return getChunksInternal("id='" + chunkID + "'");
      */
}

/*
 * retrieves exactly one chunk
 *
 * @param chunkID
 * @param targetID
 * @return a pointer to the FsckChunk or NULL if it does not exist (must be deleted by the caller!)
 */
FsckChunk* FsckDB::getChunk(std::string chunkID, uint16_t targetID)
{
   FsckChunk* outChunk = NULL;

   DBCursor<FsckChunk>* cursor = getChunksInternal(
      "id='" + chunkID + "' AND targetID=" + StringTk::uintToStr(targetID));

   FsckChunk* currentObject = cursor->open();

   if (currentObject)
   {
      outChunk = new FsckChunk();
      *outChunk = *currentObject;
   }

   cursor->close();

   SAFE_DELETE(cursor);

   return outChunk;
}

/*
 * retrieves exactly one chunk
 *
 * @param chunkID
 * @param targetID
 * @param buddyGroupID
 * @return a pointer to the FsckChunk or NULL if it does not exist (must be deleted by the caller!)
 */
FsckChunk* FsckDB::getChunk(std::string chunkID, uint16_t targetID, uint16_t buddyGroupID)
{
   FsckChunk* outChunk = NULL;

   DBCursor<FsckChunk>* cursor = getChunksInternal(
      "id='" + chunkID + "' AND targetID=" + StringTk::uintToStr(targetID) + " AND buddyGroupID="
      + StringTk::uintToStr(buddyGroupID));

   FsckChunk* currentObject = cursor->open();

   if (currentObject)
   {
      outChunk = new FsckChunk();
      *outChunk = *currentObject;
   }

   cursor->close();

   SAFE_DELETE(cursor);

   return outChunk;
}

/*
 * get row count
 *
 * @return the number of rows in the table
 */
int64_t FsckDB::countChunks()
{
   std::string tableName = TABLE_NAME_CHUNKS;
   return this->getRowCount(tableName);
}

/*
 * retrieves all cont dirs in DB
 *
 * @param outContDirs a pointer to a list holding the retrieved cont dirs
 */
void FsckDB::getContDirs(FsckContDirList* outContDirs)
{
   DBCursor<FsckContDir>* cursor = getContDirsInternal();

   FsckContDir* currentObject = cursor->open();

   while (currentObject)
   {
      outContDirs->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to all cont dirs in DB
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckContDir>* FsckDB::getContDirs()
{
   return getContDirsInternal();
}

/*
 * get row count
 *
 * @return the number of rows in the table
 */
int64_t FsckDB::countContDirs()
{
   std::string tableName = TABLE_NAME_CONT_DIRS;
   return this->getRowCount(tableName);
}



/*
 * retrieves all fs ids in DB
 *
 * @param outContDirs a pointer to a list holding the retrieved fs ids
 */
void FsckDB::getFsIDs(FsckFsIDList* outFsIDs)
{
   DBCursor<FsckFsID>* cursor = getFsIDsInternal();

   FsckFsID* currentObject = cursor->open();

   while (currentObject)
   {
      outFsIDs->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to all fs ids in DB
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckFsID>* FsckDB::getFsIDs()
{
   return getFsIDsInternal();
}

/*
 * get row count
 *
 * @return the number of rows in the table
 */
int64_t FsckDB::countFsIDs()
{
   std::string tableName = TABLE_NAME_FSIDS;
   return this->getRowCount(tableName);
}

/*
 * retrieves all saved "used target IDs" in DB
 *
 * @param outTargetIDs a pointer to a list holding the retrieved target ids
 */
void FsckDB::getUsedTargetIDs(FsckTargetIDList* outTargetIDs)
{
   DBCursor<FsckTargetID>* cursor = getUsedTargetIDsInternal();

   FsckTargetID* currentObject = cursor->open();

   while (currentObject)
   {
      outTargetIDs->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to all saved "used target IDs" in DB
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckTargetID>* FsckDB::getUsedTargetIDs()
{
   return getUsedTargetIDsInternal();
}

/*
 * get row count
 *
 * @return the number of rows in the table
 */
int64_t FsckDB::countUsedTargetIDs()
{
   std::string tableName = TABLE_NAME_USED_TARGETS;
   return this->getRowCount(tableName);
}

/*
 * retrieves all saved modification events from servers in DB
 *
 * @param outEvents a pointer to a list holding the retrieved events
 */
void FsckDB::getModificationEvents(FsckModificationEventList* outEvents)
{
   DBCursor<FsckModificationEvent>* cursor = getModificationEventsInternal();

   FsckModificationEvent* currentObject = cursor->open();

   while (currentObject)
   {
      outEvents->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor pointing to all modification events from servers in DB
 *
 * @return a DBCursor
 */
DBCursor<FsckModificationEvent>* FsckDB::getModificationEvents()
{
   return getModificationEventsInternal();
}

/*
 * get row count
 *
 * @return the number of rows in the table
 */
int64_t FsckDB::countModificationEvents()
{
   std::string tableName = TABLE_NAME_MODIFICATION_EVENTS;
   return this->getRowCount(tableName);
}


/*
 * retrieves a cursor to all dentries without inodes found in DB
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirEntry>* FsckDB::getDanglingDirEntries()
{
   return getDanglingDirEntriesInternal();
}

/*
 * retrieves a cursor to all dangling dentries representing a directory
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirEntry>* FsckDB::getDanglingDirectoryDirEntries()
{
   return getDanglingDirEntriesInternal("entryType='" +
      StringTk::intToStr((int)FsckDirEntryType_DIRECTORY) + "'");
}

/*
 * retrieves a cursor to all dangling dentries representing a directory with a given repair action
 * set
 *
 * @param repairAction
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirEntry>* FsckDB::getDanglingDirectoryDirEntries(FsckRepairAction repairAction)
{
   std::string repairActionStr = StringTk::intToStr((int) repairAction);
   return getDanglingDirEntriesInternal(
      "entryType='" + StringTk::intToStr((int) FsckDirEntryType_DIRECTORY) + "' AND repairAction='"
         + repairActionStr + "'");
}

/*
 * retrieves a cursor to all dangling dentries representing a file-like type (not a directory)
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirEntry>* FsckDB::getDanglingFileDirEntries()
{
   return getDanglingDirEntriesInternal("entryType<>'" +
      StringTk::intToStr((int)FsckDirEntryType_DIRECTORY) + "'");
}

/*
 * retrieves a cursor to all dangling dentries representing a file with a given repair action
 * set
 *
 * @param repairAction
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirEntry>* FsckDB::getDanglingFileDirEntries(FsckRepairAction repairAction)
{
   std::string repairActionStr = StringTk::intToStr((int) repairAction);
   return getDanglingDirEntriesInternal(
      "entryType<>'" + StringTk::intToStr((int) FsckDirEntryType_DIRECTORY) + "' AND repairAction='"
         + repairActionStr + "'");
}

/*
 * retrieves a cursor to all dentries without inodes with a certain repairAction set
 *
 * @param repairAction the repair action to search for
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirEntry>* FsckDB::getDanglingDirEntries(FsckRepairAction repairAction)
{
   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getDanglingDirEntriesInternal("repairAction='" + repairActionStr +"'");
}

DBCursor<FsckDirEntry>* FsckDB::getDanglingDirEntries(FsckRepairAction repairAction,
   uint16_t nodeID)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_DANGLINGDENTRY);
   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getDanglingDirEntriesInternal(tableName + ".repairAction='" + repairActionStr +"'"
      + " AND " + tableName + ".saveNodeID=" + StringTk::uintToStr(nodeID));
}

DBCursor<FsckDirEntry>* FsckDB::getDanglingDirEntriesByInodeOwner(FsckRepairAction repairAction,
   uint16_t inodeOwnerID)
{
   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getDanglingDirEntriesInternal("repairAction='" + repairActionStr +"'"
      + " AND inodeOwnerNodeID=" + StringTk::uintToStr(inodeOwnerID));
}

/*
 * retrieves all dentries without inodes found in DB; this is a simpliefied version, which returns
 * a list instead of a cursor; only use it if you are sure that the result is not too big
 *
 * @param outDentries a pointer to a list holding the retrieved entries
 *
 * @return boolean value indicating if an error occured
 */
void FsckDB::getDanglingDirEntries(FsckDirEntryList* outDentries)
{
   DBCursor<FsckDirEntry>* cursor = getDanglingDirEntriesInternal();

   FsckDirEntry* currentObject = cursor->open();

   while (currentObject)
   {
      outDentries->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to all dir inodes with a wrong owner set
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirInode>* FsckDB::getInodesWithWrongOwner()
{
   return getInodesWithWrongOwnerInternal();
}

/*
 * retrieves a cursor to dir inodes with a wrong owner set, with a certain repairAction set
 *
 * @param repairAction the repair action to search for
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirInode>* FsckDB::getInodesWithWrongOwner(FsckRepairAction repairAction)
{
   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getInodesWithWrongOwnerInternal("repairAction='" + repairActionStr +"'");
}

/*
 * retrieves all dir inodes with a wrong owner set; this is a simplified version, which returns a
 * list instead of a cursor; only use it if you are sure that the result is not too big
 *
 * @param outDirInodes a pointer to a list holding the retrieved inodes
 *
 * @return boolean value indicating if an error occured
 */
void FsckDB::getInodesWithWrongOwner(FsckDirInodeList* outDirInodes)
{
   DBCursor<FsckDirInode>* cursor = getInodesWithWrongOwnerInternal();

   FsckDirInode* currentObject = cursor->open();

   while (currentObject)
   {
      outDirInodes->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to dir inodes with a wrong owner set, with a certain
 * repairAction set, which are saved on a certain node
 *
 * @param repairAction the repair action to search for
 * @param nodeID node to retrieve dir inodes from
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirInode>* FsckDB::getInodesWithWrongOwner(FsckRepairAction repairAction,
   uint16_t nodeID)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_WRONGINODEOWNER);
   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getInodesWithWrongOwnerInternal(tableName + ".repairAction='" + repairActionStr +"'"
      + " AND " + tableName + ".saveNodeID=" + StringTk::uintToStr(nodeID));
}

/*
 * retrieves a cursor to all dentries, in which a wrong inode owner is saved
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirEntry>* FsckDB::getDirEntriesWithWrongOwner()
{
   return getDirEntriesWithWrongOwnerInternal();
}

/*
 * retrieves a cursor to dentries in which a wrong inode owner is saved, with a certain
 * repairAction set
 *
 * @param repairAction the repair action to search for
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirEntry>* FsckDB::getDirEntriesWithWrongOwner(FsckRepairAction repairAction)
{
   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getDirEntriesWithWrongOwnerInternal("repairAction='" + repairActionStr +"'");
}

/*
 * retrieves a cursor to dentries in which a wrong inode owner is saved, with a certain
 * repairAction set, which are saved on a certain node
 *
 * @param repairAction the repair action to search for
 * @param nodeID node to retrieve dir entries from
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirEntry>* FsckDB::getDirEntriesWithWrongOwner(FsckRepairAction repairAction,
   uint16_t nodeID)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_WRONGOWNERINDENTRY);
   std::string repairActionStr = StringTk::intToStr((int)repairAction);

   return getDirEntriesWithWrongOwnerInternal(tableName + ".repairAction='" + repairActionStr +"'"
      + " AND " + tableName + ".saveNodeID=" + StringTk::uintToStr(nodeID));
}

/*
 * retrieves all dentries, in which a wrong inode owner is saved, this is a simplified
 * version, which returns a list instead of a cursor; only use it if you are sure that the
 * result is not too big
 *
 * @param outDentries a pointer to a list holding the retrieved entries
 *
 * @return boolean value indicating if an error occured
 */
void FsckDB::getDirEntriesWithWrongOwner(FsckDirEntryList* outDentries)
{
   DBCursor<FsckDirEntry>* cursor = getDirEntriesWithWrongOwnerInternal();

   FsckDirEntry* currentObject = cursor->open();

   while (currentObject)
   {
      outDentries->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to all dentries with broke (or missing) dentry-by-id files
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirEntry>* FsckDB::getDirEntriesWithBrokenByIDFile()
{
   return getDirEntriesWithBrokenByIDFileInternal();
}

/*
 * retrieves a cursor to all dentries with broke (or missing) dentry-by-id files, with a
 * certain repairAction set
 *
 * @param repairAction the repair action to search for
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirEntry>* FsckDB::getDirEntriesWithBrokenByIDFile(FsckRepairAction repairAction)
{
   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getDirEntriesWithBrokenByIDFileInternal("repairAction='" + repairActionStr +"'");
}

/*
 * retrieves a cursor to all dentries with broke (or missing) dentry-by-id files, with a
 * certain repairAction set, which are saved on a certain node
 *
 * @param repairAction the repair action to search for
 * @param nodeID node to retrieve dir entries from
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirEntry>* FsckDB::getDirEntriesWithBrokenByIDFile(FsckRepairAction repairAction,
   uint16_t nodeID)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_BROKENFSID);
   std::string repairActionStr = StringTk::intToStr((int)repairAction);

   return getDirEntriesWithBrokenByIDFileInternal(
      tableName + ".repairAction='" + repairActionStr + "'" + " AND " + tableName + ".saveNodeID="
         + StringTk::uintToStr(nodeID));
}

/*
 * retrieves a cursor to all dentries with broke (or missing) dentry-by-id files this is a
 * simplified version, which returns a list instead of a cursor; only use it if you are sure that
 * the result is not too big
 *
 * @param outDentries a pointer to a list holding the retrieved entries
 *
 * @return boolean value indicating if an error occured
 */
void FsckDB::getDirEntriesWithBrokenByIDFile(FsckDirEntryList* outDentries)
{
   DBCursor<FsckDirEntry>* cursor = getDirEntriesWithBrokenByIDFileInternal();

   FsckDirEntry* currentObject = cursor->open();

   while (currentObject)
   {
      outDentries->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to a set of dentry-by-id files, with no associated dentry
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckFsID>* FsckDB::getOrphanedDentryByIDFiles()
{
   return getOrphanedDentryByIDFilesInternal();
}

/*
 * retrieves a cursor to a set of dentry-by-id files, with no associated dentry, with a
 * certain repairAction set
 *
 * @param repairAction the repair action to search for
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckFsID>* FsckDB::getOrphanedDentryByIDFiles(FsckRepairAction repairAction)
{
   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getOrphanedDentryByIDFilesInternal("repairAction='" + repairActionStr +"'");
}

/*
 * retrieves a cursor to a set of dentry-by-id files, with no associated dentry, with a
 * certain repairAction set, which are saved on a certain node
 *
 * @param repairAction the repair action to search for
 * @param nodeID node to retrieve files from
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckFsID>* FsckDB::getOrphanedDentryByIDFiles(FsckRepairAction repairAction,
   uint16_t nodeID)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDFSID);
   std::string repairActionStr = StringTk::intToStr((int)repairAction);

   return getOrphanedDentryByIDFilesInternal(
      tableName + ".repairAction='" + repairActionStr + "'" + " AND " + tableName + ".saveNodeID="
         + StringTk::uintToStr(nodeID));
}

/*
 * retrieves a cursor to a set of dentry-by-id files, with no associated dentry; this is a
 * simplified version, which returns a list instead of a cursor; only use it if you are sure that
 * the result is not too big
 *
 * @param outIDs a pointer to a list holding the retrieved fsID files
 *
 * @return boolean value indicating if an error occured
 */
void FsckDB::getOrphanedDentryByIDFiles(FsckFsIDList* outIDs)
{
   DBCursor<FsckFsID>* cursor = getOrphanedDentryByIDFilesInternal();

   FsckFsID* currentObject = cursor->open();

   while (currentObject)
   {
      outIDs->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to all orphaned dir inodes
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirInode>* FsckDB::getOrphanedDirInodes()
{
   return getOrphanedDirInodesInternal();
}

/*
 * retrieves a cursor to orphaned dir inodes, with a certain repairAction set
 *
 * @param repairAction the repair action to search for
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirInode>* FsckDB::getOrphanedDirInodes(FsckRepairAction repairAction)
{
   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getOrphanedDirInodesInternal("repairAction='" + repairActionStr +"'");
}

/*
 * retrieves all orphaned dir inodes; this is a simplified version, which returns a list instead
 * of a cursor; only use it if you are sure that the result is not too big
 *
 * @param outDirInodes a pointer to a list holding the retrieved inodes
 *
 * @return boolean value indicating if an error occured
 */
void FsckDB::getOrphanedDirInodes(FsckDirInodeList* outDirInodes)
{
   DBCursor<FsckDirInode>* cursor = getOrphanedDirInodesInternal();

   FsckDirInode* currentObject = cursor->open();

   while (currentObject)
   {
      outDirInodes->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to all orphaned file inodes
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckFileInode>* FsckDB::getOrphanedFileInodes()
{
   return getOrphanedFileInodesInternal();
}

/*
 * retrieves a cursor to orphaned file inodes, with a certain repairAction set
 *
 * @param repairAction the repair action to search for
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckFileInode>* FsckDB::getOrphanedFileInodes(FsckRepairAction repairAction)
{
   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getOrphanedFileInodesInternal("repairAction='" + repairActionStr +"'");
}

/*
 * retrieves a cursor to orphaned file inodes, with a certain repairAction set, but only on a
 * certain metadata node
 *
 * @param repairAction the repair action to search for
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckFileInode>* FsckDB::getOrphanedFileInodes(FsckRepairAction repairAction,
   uint16_t saveNodeID)
{
   std::string repairActionStr = StringTk::intToStr((int) repairAction);
   return getOrphanedFileInodesInternal("repairAction='" + repairActionStr + "' AND saveNodeID='"
      + StringTk::uintToStr(saveNodeID) + "'");
}

/*
 * retrieves all orphaned file inodes; this is a simplified version, which returns a list instead
 * of a cursor; only use it if you are sure that the result is not too big
 *
 * @param outFileInodes a pointer to a list holding the retrieved inodes
 *
 * @return boolean value indicating if an error occured
 */
void FsckDB::getOrphanedFileInodes(FsckFileInodeList* outFileInodes)
{
   DBCursor<FsckFileInode>* cursor = getOrphanedFileInodesInternal();

   FsckFileInode* currentObject = cursor->open();

   while (currentObject)
   {
      outFileInodes->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to all orphaned chunks
 *
 * @param orderedByID if true, results will be given ordered by ID
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getOrphanedChunks(bool orderedByID)
{
      return getOrphanedChunksInternal("1", orderedByID);
}

/*
 * retrieves a cursor to orphaned chunks, with a certain repairAction set
 *
 * @param repairAction the repair action to search for
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getOrphanedChunks(FsckRepairAction repairAction, bool orderedByID)
{
   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getOrphanedChunksInternal("repairAction='" + repairActionStr +"'", orderedByID);
}

/*
 * retrieves a cursor to orphaned chunks on a given target, with a certain repairAction set
 *
 * @param repairAction the repair action to search for
 * @param targetID
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getOrphanedChunks(FsckRepairAction repairAction, uint16_t targetID)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDCHUNK);

   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getOrphanedChunksInternal(tableName + ".repairAction='" + repairActionStr +"'"
      + " AND " + tableName + ".targetID=" + StringTk::uintToStr(targetID));
}

/*
 * retrieves all orphaned chunks; this is a simplified version, which returns a list instead
 * of a cursor; only use it if you are sure that the result is not too big
 *
 * @param outChunks a pointer to a list holding the retrieved chunks
 *
 * @return boolean value indicating if an error occured
 */
void FsckDB::getOrphanedChunks(FsckChunkList* outChunks)
{
   DBCursor<FsckChunk>* cursor = getOrphanedChunksInternal();

   FsckChunk* currentObject = cursor->open();

   while (currentObject)
   {
      outChunks->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to all dir inodes without a .cont directory
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirInode>* FsckDB::getInodesWithoutContDir()
{
   return getInodesWithoutContDirInternal();
}

/*
 * retrieves a cursor to dir inodes without a .cont directory, with a certain repairAction set
 *
 * @param repairAction the repair action to search for
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirInode>* FsckDB::getInodesWithoutContDir(FsckRepairAction repairAction)
{
   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getInodesWithoutContDirInternal("repairAction='" + repairActionStr +"'");
}

/*
 * retrieves a cursor to dir inodes without a .cont directory, with a certain repairAction set
 *
 * @param repairAction the repair action to search for
 * @param nodeID the node on which the entries, which are retrieved, are saved
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirInode>* FsckDB::getInodesWithoutContDir(FsckRepairAction repairAction,
   uint16_t nodeID)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_MISSINGCONTDIR);

   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getInodesWithoutContDirInternal("repairAction='" + repairActionStr +"' AND " + tableName
      + ".saveNodeID=" + StringTk::uintToStr(nodeID));
}

/*
 * retrieves all dir inodes without a .cont directory; this is a simplified version, which returns
 * a list instead of a cursor; only use it if you are sure that the result is not too big
 *
 * @param outInodes a pointer to a list holding the retrieved inodes
 *
 * @return boolean value indicating if an error occured
 */
void FsckDB::getInodesWithoutContDir(FsckDirInodeList* outInodes)
{
   DBCursor<FsckDirInode>* cursor = getInodesWithoutContDirInternal();

   FsckDirInode* currentObject = cursor->open();

   while (currentObject)
   {
      outInodes->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to all .cont directories without an inode
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckContDir>* FsckDB::getOrphanedContDirs()
{
   return getOrphanedContDirsInternal();
}

/*
 * retrieves a cursor to content directories without an inode, with a certain repairAction set
 *
 * @param repairAction the repair action to search for
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckContDir>* FsckDB::getOrphanedContDirs(FsckRepairAction repairAction)
{
   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getOrphanedContDirsInternal("repairAction='" + repairActionStr +"'");
}

/*
 * retrieves a cursor to orphaned cont dirs on a given node, with a certain repairAction set
 *
 * @param repairAction the repair action to search for
 * @param nodeID
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckContDir>* FsckDB::getOrphanedContDirs(FsckRepairAction repairAction,
   uint16_t nodeID)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDCONTDIR);

   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getOrphanedContDirsInternal(tableName + ".repairAction='" + repairActionStr +"'"
      + " AND " + tableName + ".saveNodeID=" + StringTk::uintToStr(nodeID));
}

/*
 * retrieves all .cont directories without an inode; this is a simplified version, which returns
 * a list instead of a cursor; only use it if you are sure that the result is not too big
 *
 * @param outContDirs a pointer to a list holding the retrieved content dirs
 *
 * @return boolean value indicating if an error occured
 */
void FsckDB::getOrphanedContDirs(FsckContDirList* outContDirs)
{
   DBCursor<FsckContDir>* cursor = getOrphanedContDirsInternal();

   FsckContDir* currentObject = cursor->open();

   while (currentObject)
   {
      outContDirs->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to all to all file inodes with wrong attributes (filesize, etc. ...)
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckFileInode>* FsckDB::getInodesWithWrongFileAttribs()
{
   return getInodesWithWrongFileAttribsInternal();
}

/*
 * retrieves a cursor to file inodes with wrong stat attributes (filesize, etc., ...), with a
 * certain repairAction set
 *
 * @param repairAction the repair action to search for
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckFileInode>* FsckDB::getInodesWithWrongFileAttribs(FsckRepairAction repairAction)
{
   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getInodesWithWrongFileAttribsInternal("repairAction='" + repairActionStr +"'");
}

/*
 * retrieves a cursor to file inodes with wrong stat attributes (filesize, etc., ...), with a
 * certain repairAction set, which are saved on a given node
 *
 * @param repairAction the repair action to search for
 * @param nodeID
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckFileInode>* FsckDB::getInodesWithWrongFileAttribs(FsckRepairAction repairAction,
   uint16_t nodeID)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_WRONGFILEATTRIBS);
   std::string tableNameInodes = TABLE_NAME_FILE_INODES;

   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getInodesWithWrongFileAttribsInternal(tableName + ".repairAction='" + repairActionStr
      + "'" + " AND " + tableNameInodes + ".saveNodeID=" + StringTk::uintToStr(nodeID));
}

/*
 * retrieves all file inodes with wrong stat attributes (filesize, ctime, ...)
 * this is a simplified version, which returns a list instead of a cursor; only use it if you are
 * sure that the result is not too big
 *
 * @param outFileInodes a pointer to a list holding the retrieved inodes
 *
 * @return boolean value indicating if an error occured
 */
void FsckDB::getInodesWithWrongFileAttribs(FsckFileInodeList* outFileInodes)
{
   DBCursor<FsckFileInode>* cursor = getInodesWithWrongFileAttribsInternal();

   FsckFileInode* currentObject = cursor->open();

   while (currentObject)
   {
      outFileInodes->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to all to all dir inodes with wrong attributes (size, etc. ...)
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirInode>* FsckDB::getInodesWithWrongDirAttribs()
{
   return getInodesWithWrongDirAttribsInternal();
}

/*
 * retrieves a cursor to dir inodes with wrong attributes (size, etc., ...), with a
 * certain repairAction set
 *
 * @param repairAction the repair action to search for
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirInode>* FsckDB::getInodesWithWrongDirAttribs(FsckRepairAction repairAction)
{
   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getInodesWithWrongDirAttribsInternal("repairAction='" + repairActionStr +"'");
}

/*
 * retrieves a cursor to dir inodes with wrong attributes (size, etc., ...), with a
 * certain repairAction set, which are saved on a given node
 *
 * @param repairAction the repair action to search for
 * @param nodeID
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirInode>* FsckDB::getInodesWithWrongDirAttribs(FsckRepairAction repairAction,
   uint16_t nodeID)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_WRONGDIRATTRIBS);
   std::string tableNameInodes = TABLE_NAME_DIR_INODES;

   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getInodesWithWrongDirAttribsInternal(tableName + ".repairAction='" + repairActionStr
      + "'" + " AND " + tableNameInodes + ".saveNodeID=" + StringTk::uintToStr(nodeID));
}

/*
 * retrieves all dir inodes with wrong attributes
 * this is a simplified version, which returns a list instead of a cursor; only use it if you are
 * sure that the result is not too big
 *
 * @param outDirInodes a pointer to a list holding the retrieved inodes
 *
 * @return boolean value indicating if an error occured
 */
void FsckDB::getInodesWithWrongDirAttribs(FsckDirInodeList* outDirInodes)
{
   DBCursor<FsckDirInode>* cursor = getInodesWithWrongDirAttribsInternal();

   FsckDirInode* currentObject = cursor->open();

   while (currentObject)
   {
      outDirInodes->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to all to all targetIDs/buddyGroupIDs, which are used in stripe patterns,
 * but do not exist
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckTargetID>* FsckDB::getMissingStripeTargets()
{
   return getMissingStripeTargetsInternal();
}

/*
 * retrieves a cursor to all to all targetIDs, which are used in stripe patterns, but do not exist,
 * with a certain repairAction set
 *
 * @param repairAction the repair action to search for
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckTargetID>* FsckDB::getMissingStripeTargets(FsckRepairAction repairAction)
{
   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getMissingStripeTargetsInternal("repairAction='" + repairActionStr +"'");
}

/*
 * retrieves all targetIDs, which are used in stripe patterns, but do not exist
 * this is a simplified version, which returns a list instead of a cursor; only use it if you are
 * sure that the result is not too big
 *
 * @param outStripeTargets a pointer to a list holding the retrieved targetIDs
 *
 * @return boolean value indicating if an error occured
 */
void FsckDB::getMissingStripeTargets(FsckTargetIDList* outStripeTargets)
{
   DBCursor<FsckTargetID>* cursor = getMissingStripeTargetsInternal();

   FsckTargetID* currentObject = cursor->open();

   while (currentObject)
   {
      outStripeTargets->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves all targetIDs, which are used in stripe patterns, but do not exist; with a certain
 * repair action set
 * this is a simplified version, which returns a list instead of a cursor; only use it if you are
 * sure that the result is not too big
 *
 * @param outStripeTargets a pointer to a list holding the retrieved targetIDs
 *
 * @return boolean value indicating if an error occured
 */
void FsckDB::getMissingStripeTargets(FsckTargetIDList* outStripeTargets,
   FsckRepairAction repairAction)
{
   DBCursor<FsckTargetID>* cursor = getMissingStripeTargets(repairAction);

   FsckTargetID* currentObject = cursor->open();

   while (currentObject)
   {
      outStripeTargets->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to a set of dir entries, which symbolize files, that are using non-existant
 * stripe targets
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirEntry>* FsckDB::getFilesWithMissingStripeTargets()
{
   return getFilesWithMissingStripeTargetsInternal();
}

/*
 * retrieves a cursor to a set of dir entries, which symbolize files, that are using non-existant
 * stripe targets
 *
 * @param repairAction the repair action to search for
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirEntry>* FsckDB::getFilesWithMissingStripeTargets(FsckRepairAction repairAction)
{
   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getFilesWithMissingStripeTargetsInternal("repairAction='" + repairActionStr +"'");
}

DBCursor<FsckDirEntry>* FsckDB::getFilesWithMissingStripeTargets(FsckRepairAction repairAction,
   uint16_t dentrySaveNodeID)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_FILEWITHMISSINGTARGET);

   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getFilesWithMissingStripeTargetsInternal(tableName + ".repairAction='" + repairActionStr
      + "' AND " + tableName + ".saveNodeID='" + StringTk::uintToStr(dentrySaveNodeID) + "'");
}

/*
 * retrieves a cursor to a set of dir entries, which symbolize files, that are using non-existant
 * stripe targets
 * this is a simplified version, which returns a list instead of a cursor; only use it if you are
 * sure that the result is not too big
 *
 * @param outDirEntries a pointer to a list holding the retrieved dentries
 *
 * @return boolean value indicating if an error occured
 */
void FsckDB::getFilesWithMissingStripeTargets(FsckDirEntryList* outDirEntries)
{
   DBCursor<FsckDirEntry >* cursor = getFilesWithMissingStripeTargets();

   FsckDirEntry* currentObject = cursor->open();

   while (currentObject)
   {
      outDirEntries->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to a set of dir entries, which symbolize files, that are using non-existant
 * stripe targets
 * this is a simplified version, which returns a list instead of a cursor; only use it if you are
 * sure that the result is not too big
 *
 * @param outDirEntries a pointer to a list holding the retrieved dentries
 * @param repairAction the repair action to search for
 *
 * @return boolean value indicating if an error occured
 */
void FsckDB::getFilesWithMissingStripeTargets(FsckDirEntryList* outDirEntries,
   FsckRepairAction repairAction)
{
   DBCursor<FsckDirEntry>* cursor = getFilesWithMissingStripeTargets(repairAction);

   FsckDirEntry* currentObject = cursor->open();

   while (currentObject)
   {
      outDirEntries->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}


/*
 * retrieves a cursor to all chunks, for which the mirror chunk is missing
 *
 * @param orderedByID if true, results will be ordered by ID
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getMissingMirrorChunks(bool orderedByID)
{
   return getMissingMirrorChunksInternal("1", orderedByID);
}

/*
 * retrieves a cursor to all chunks, for which the mirror chunk is missing
 *
 * @param repairAction the repair action to search for
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getMissingMirrorChunks(FsckRepairAction repairAction, bool orderedByID)
{
   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getMissingMirrorChunksInternal("repairAction='" + repairActionStr +"'", orderedByID);
}

/*
 * retrieves a cursor to all chunks, for which the mirror chunk is missing
 *
 * @param repairAction the repair action to search for
 * @param targetID
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getMissingMirrorChunks(FsckRepairAction repairAction,
   uint16_t targetID)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_MISSINGMIRRORCHUNK);

   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getMissingMirrorChunksInternal(tableName + ".repairAction='" + repairActionStr +"'"
      + " AND " + tableName + ".targetID=" + StringTk::uintToStr(targetID));
}

/*
 * retrieves a cursor to all chunks, for which the mirror chunk is missing; this is a simplified
 * version, which returns a list instead of a cursor; only use it if you are sure that the result
 * is not too big
 *
 * @param outChunks a pointer to a list holding the retrieved chunks
 *
 * @return boolean value indicating if an error occured
 */
void FsckDB::getMissingMirrorChunks(FsckChunkList* outChunks)
{
   DBCursor<FsckChunk>* cursor = getMissingMirrorChunksInternal();

   FsckChunk* currentObject = cursor->open();

   while (currentObject)
   {
      outChunks->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to all chunks, for which the mirror chunk is present, but no primary chunk
 *
 * @param orderedByID if true, results will be ordered by ID
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getMissingPrimaryChunks(bool orderedByID)
{
   return getMissingPrimaryChunksInternal("1", orderedByID);
}

/*
 * retrieves a cursor to all chunks, for which the mirror chunk is present, but no primary chunk
 *
 * @param repairAction the repair action to search for
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getMissingPrimaryChunks(FsckRepairAction repairAction, bool orderedByID)
{
   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getMissingPrimaryChunksInternal("repairAction='" + repairActionStr +"'", orderedByID);
}

/*
 * retrieves a cursor to all chunks, for which the mirror chunk is present, but no primary chunk
 *
 * @param repairAction the repair action to search for
 * @param targetID
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getMissingPrimaryChunks(FsckRepairAction repairAction,
   uint16_t targetID)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_MISSINGPRIMARYCHUNK);

   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getMissingPrimaryChunksInternal(tableName + ".repairAction='" + repairActionStr +"'"
      + " AND " + tableName + ".targetID=" + StringTk::uintToStr(targetID));
}

/*
 * retrieves a cursor to all chunks, for which the mirror chunk is present, but no primary chunk
 * exists; this is a simplified version, which returns a list instead of a cursor; only use it if
 * you are sure that the result is not too big
 *
 * @param outChunks a pointer to a list holding the retrieved chunks
 *
 * @return boolean value indicating if an error occured
 */
void FsckDB::getMissingPrimaryChunks(FsckChunkList* outChunks)
{
   DBCursor<FsckChunk>* cursor = getMissingPrimaryChunksInternal();

   FsckChunk* currentObject = cursor->open();

   while (currentObject)
   {
      outChunks->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to all chunks, for which the attribs of the primary and the mirror chunk
 * differ
 *
 * @param orderedByID if true, results will be ordered by ID
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getDifferingChunkAttribs(bool orderedByID)
{
   return getDifferingChunkAttribsInternal("1", orderedByID);
}

/*
 * retrieves a cursor to all chunks, for which the attribs of the primary and the mirror chunk
 * differ
 *
 * @param repairAction the repair action to search for
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getDifferingChunkAttribs(FsckRepairAction repairAction,
   bool orderedByID)
{
   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getDifferingChunkAttribsInternal("repairAction='" + repairActionStr +"'", orderedByID);
}

/*
 * retrieves a cursor to all chunks, for which the attribs of the primary and the mirror chunk
 * differ
 *
 * @param repairAction the repair action to search for
 * @param targetID
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getDifferingChunkAttribs(FsckRepairAction repairAction,
   uint16_t targetID)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_DIFFERINGCHUNKATTRIBS);

   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getDifferingChunkAttribsInternal(tableName + ".repairAction='" + repairActionStr +"'"
      + " AND " + tableName + ".targetID=" + StringTk::uintToStr(targetID));
}

/*
 * retrieves a cursor to all chunks, for which the attribs of the primary and the mirror chunk
 * differ; this is a simplified version, which returns a list instead of a cursor; only use it if
 * you are sure that the result is not too big
 *
 * @param outChunks a pointer to a list holding the retrieved chunks
 *
 * @return boolean value indicating if an error occured
 */
void FsckDB::getDifferingChunkAttribs(FsckChunkList* outChunks)
{
   DBCursor<FsckChunk>* cursor = getDifferingChunkAttribsInternal();

   FsckChunk* currentObject = cursor->open();

   while (currentObject)
   {
      outChunks->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to all chunks, which have wrong uid/gid
 *
 * @param orderedByID if true, results will be ordered by ID
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getChunksWithWrongPermissions(bool orderedByID)
{
   return getChunksWithWrongPermissionsInternal("1", orderedByID);
}

/*
 * retrieves a cursor to all chunks, which have wrong uid/gid
 *
 * @param repairAction the repair action to search for
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getChunksWithWrongPermissions(FsckRepairAction repairAction,
   bool orderedByID)
{
   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getChunksWithWrongPermissionsInternal("repairAction='" + repairActionStr +"'",
      orderedByID);
}

/*
 * retrieves a cursor to all chunks, which have wrong uid/gid
 *
 * @param repairAction the repair action to search for
 * @param targetID
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getChunksWithWrongPermissions(FsckRepairAction repairAction,
   uint16_t targetID)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_CHUNKWITHWRONGPERM);

   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getChunksWithWrongPermissionsInternal(tableName + ".repairAction='" + repairActionStr +"'"
      + " AND " + tableName + ".targetID=" + StringTk::uintToStr(targetID));
}

/*
 * retrieves a list of all chunks, which have wrong uid/gid
 * this is a simplified version, which returns a list instead of a cursor; only use it if
 * you are sure that the result is not too big
 *
 * @param outChunks a pointer to a list holding the retrieved chunks
 *
 * @return boolean value indicating if an error occured
 */
void FsckDB::getChunksWithWrongPermissions(FsckChunkList* outChunks)
{
   DBCursor<FsckChunk>* cursor = getChunksWithWrongPermissionsInternal();

   FsckChunk* currentObject = cursor->open();

   while (currentObject)
   {
      outChunks->push_back(*currentObject);
      currentObject = cursor->step();
   }

   cursor->close();

   SAFE_DELETE(cursor);
}

/*
 * retrieves a cursor to all chunks, which are saved in the wrong location
 *
 * @param orderedByID if true, results will be ordered by ID
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getChunksInWrongPath(bool orderedByID)
{
   return getChunksInWrongPathInternal("1", orderedByID);
}

DBCursor<FsckChunk>* FsckDB::getChunksInWrongPath(FsckRepairAction repairAction,
   bool orderedByID)
{
   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getChunksInWrongPathInternal("repairAction='" + repairActionStr +"'",
      orderedByID);
}

DBCursor<FsckChunk>* FsckDB::getChunksInWrongPath(FsckRepairAction repairAction,
   uint16_t targetID)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_CHUNKINWRONGPATH);

   std::string repairActionStr = StringTk::intToStr((int)repairAction);
   return getChunksInWrongPathInternal(tableName + ".repairAction='" + repairActionStr +"'"
      + " AND " + tableName + ".targetID=" + StringTk::uintToStr(targetID));
}

/*
 * get the amount of errors in DB with a specified errorCode
 *
 * @param errorCode the errorCode
 * @return the amount of errors in DB
 */
int64_t FsckDB::countErrors(FsckErrorCode errorCode)
{
   // check only for those without a repair action set
   std::string repairActionStr = StringTk::intToStr((int)FsckRepairAction_UNDEFINED);
   std::string tableName = TABLE_NAME_ERROR(errorCode);

   return this->getRowCount(tableName, "repairAction='" + repairActionStr + "'");
}

/*
 * retrieves a cursor to a set of dentries in DB
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirEntry>* FsckDB::getDirEntriesInternal(std::string sqlWhereClause, uint maxCount)
{
   std::string tableName = TABLE_NAME_DIR_ENTRIES;

   std::string sqlLimitStmt = "";

   if (maxCount != 0)
      sqlLimitStmt = "LIMIT " + StringTk::uintToStr(maxCount);

   std::string queryStr = "SELECT * FROM " + tableName + " WHERE " + sqlWhereClause + " " +
      sqlLimitStmt;

   DBCursor<FsckDirEntry>* cursor = new DBCursor<FsckDirEntry>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * retrieves a cursor to a set of file inodes in DB
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckFileInode>* FsckDB::getFileInodesInternal(std::string sqlWhereClause)
{
   std::string tableName = TABLE_NAME_FILE_INODES;

   std::string queryStr = "SELECT * FROM " + tableName + " WHERE " + sqlWhereClause;

   DBCursor<FsckFileInode>* cursor = new DBCursor<FsckFileInode>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * retrieves a cursor to a set of dir inodes in DB
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirInode>* FsckDB::getDirInodesInternal(std::string sqlWhereClause)
{
   std::string tableName = TABLE_NAME_DIR_INODES;

   std::string queryStr = "SELECT * FROM " + tableName + " WHERE " + sqlWhereClause;

   DBCursor<FsckDirInode>* cursor = new DBCursor<FsckDirInode>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * retrieves a cursor to a set of chunks in DB
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getChunksInternal(std::string sqlWhereClause)
{
   std::string tableName = TABLE_NAME_CHUNKS;

   std::string queryStr = "SELECT * FROM " + tableName + " WHERE " + sqlWhereClause;

   DBCursor<FsckChunk>* cursor = new DBCursor<FsckChunk>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * retrieves a cursor to a set of cont dirs in DB
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckContDir>* FsckDB::getContDirsInternal(std::string sqlWhereClause)
{
   std::string tableName = TABLE_NAME_CONT_DIRS;

   std::string queryStr = "SELECT * FROM " + tableName + " WHERE " + sqlWhereClause;

   DBCursor<FsckContDir>* cursor = new DBCursor<FsckContDir>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * retrieves a cursor to a set of fs ids in DB
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckFsID>* FsckDB::getFsIDsInternal(std::string sqlWhereClause)
{
   std::string tableName = TABLE_NAME_FSIDS;

   std::string queryStr = "SELECT * FROM " + tableName + " WHERE " + sqlWhereClause;

   DBCursor<FsckFsID>* cursor = new DBCursor<FsckFsID>(this->dbHandlePool, queryStr);

   return cursor;
}


/*
 * retrieves a cursor to a set of "used target IDs" in DB
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckTargetID>* FsckDB::getUsedTargetIDsInternal(std::string sqlWhereClause)
{
   std::string tableName = TABLE_NAME_USED_TARGETS;

   std::string queryStr = "SELECT * FROM " + tableName + " WHERE " + sqlWhereClause;

   DBCursor<FsckTargetID>* cursor = new DBCursor<FsckTargetID>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * retrieves a cursor to a set of modification events from servers in DB
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckModificationEvent>* FsckDB::getModificationEventsInternal(std::string sqlWhereClause)
{
   std::string tableName = TABLE_NAME_MODIFICATION_EVENTS;

   std::string queryStr = "SELECT * FROM " + tableName + " WHERE " + sqlWhereClause;

   DBCursor<FsckModificationEvent>* cursor = new DBCursor<FsckModificationEvent>(this->dbHandlePool,
      queryStr);

   return cursor;
}

/*
 * retrieves a cursor to a set of dentries, for which no inodes where found, from DB
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirEntry>* FsckDB::getDanglingDirEntriesInternal(std::string sqlWhereClause)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_DANGLINGDENTRY);
   std::string dirEntryTableName = TABLE_NAME_DIR_ENTRIES;

   sqlWhereClause += " AND " + tableName + ".name=" + dirEntryTableName + ".name AND " +
      tableName + ".parentDirID=" + dirEntryTableName + ".parentDirID AND " + tableName +
      ".saveNodeID=" + dirEntryTableName + ".saveNodeID";

   std::string queryStr = "SELECT " + dirEntryTableName + ".* FROM " + tableName + "," +
      dirEntryTableName + " WHERE " + sqlWhereClause;

   DBCursor<FsckDirEntry>* cursor = new DBCursor<FsckDirEntry>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * retrieves a cursor to all dir inodes, which have a a wrong owner set
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirInode>* FsckDB::getInodesWithWrongOwnerInternal(std::string sqlWhereClause)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_WRONGINODEOWNER);
   std::string inodeTableName = TABLE_NAME_DIR_INODES;

   std::string compareField = "id";

   sqlWhereClause += " AND " + tableName + "." + compareField + "=" + inodeTableName + "." +
      compareField;

   std::string queryStr = "SELECT " + inodeTableName + ".* FROM " + tableName + "," +
      inodeTableName + " WHERE " + sqlWhereClause;

   DBCursor<FsckDirInode>* cursor = new DBCursor<FsckDirInode>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * retrieves a cursor to a set of dentries, in which a wrong inode owner is saved
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirEntry>* FsckDB::getDirEntriesWithWrongOwnerInternal(std::string sqlWhereClause)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_WRONGOWNERINDENTRY);
   std::string dirEntryTableName = TABLE_NAME_DIR_ENTRIES;

   std::string compareField = "id";

   sqlWhereClause += " AND " + tableName + ".name=" + dirEntryTableName + ".name AND " +
      tableName + ".parentDirID=" + dirEntryTableName + ".parentDirID AND " + tableName +
      ".saveNodeID=" + dirEntryTableName + ".saveNodeID";

   std::string queryStr = "SELECT " + dirEntryTableName + ".* FROM " + tableName + "," +
      dirEntryTableName + " WHERE " + sqlWhereClause;

   DBCursor<FsckDirEntry>* cursor = new DBCursor<FsckDirEntry>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * retrieves a cursor to a set of dentries with broke (or missing) dentry-by-id files
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirEntry>* FsckDB::getDirEntriesWithBrokenByIDFileInternal(std::string sqlWhereClause)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_BROKENFSID);
   std::string dirEntryTableName = TABLE_NAME_DIR_ENTRIES;

   sqlWhereClause += " AND " + tableName + ".name=" + dirEntryTableName + ".name AND " +
      tableName + ".parentDirID=" + dirEntryTableName + ".parentDirID AND " + tableName +
      ".saveNodeID=" + dirEntryTableName + ".saveNodeID";

   std::string queryStr = "SELECT " + dirEntryTableName + ".* FROM " + tableName + "," +
      dirEntryTableName + " WHERE " + sqlWhereClause;

   DBCursor<FsckDirEntry>* cursor = new DBCursor<FsckDirEntry>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * retrieves a cursor to a set of dentry-by-id files, with no associated dentry
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckFsID>* FsckDB::getOrphanedDentryByIDFilesInternal(std::string sqlWhereClause)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDFSID);
   std::string fsIDTableName = TABLE_NAME_FSIDS;

   sqlWhereClause += " AND " + tableName + ".id=" + fsIDTableName + ".id AND " + tableName +
      ".saveNodeID=" + fsIDTableName + ".saveNodeID";

   std::string queryStr = "SELECT " + fsIDTableName + ".* FROM " + tableName + "," +
      fsIDTableName + " WHERE " + sqlWhereClause;

   DBCursor<FsckFsID>* cursor = new DBCursor<FsckFsID>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * retrieves a cursor to a set of orphaned dir inodes
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirInode>* FsckDB::getOrphanedDirInodesInternal(std::string sqlWhereClause)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDDIRINODE);
   std::string inodeTableName = TABLE_NAME_DIR_INODES;

   std::string compareField = "id";

   sqlWhereClause += " AND " + tableName + "." + compareField + "=" + inodeTableName + "." +
      compareField;

   std::string queryStr = "SELECT " + inodeTableName + ".* FROM " + tableName + "," +
      inodeTableName + " WHERE " + sqlWhereClause;

   DBCursor<FsckDirInode>* cursor = new DBCursor<FsckDirInode>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * retrieves a cursor to a set of orphaned file inodes
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckFileInode>* FsckDB::getOrphanedFileInodesInternal(std::string sqlWhereClause)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDFILEINODE);
   std::string inodeTableName = TABLE_NAME_FILE_INODES;

   std::string compareField = "internalID";

   sqlWhereClause += " AND " + tableName + "." + compareField + "=" + inodeTableName + "." +
      compareField;

   std::string queryStr = "SELECT " + inodeTableName + ".* FROM " + tableName + "," +
      inodeTableName + " WHERE " + sqlWhereClause;

   DBCursor<FsckFileInode>* cursor = new DBCursor<FsckFileInode>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * retrieves a cursor to a set of orphaned chunks
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getOrphanedChunksInternal(std::string sqlWhereClause, bool sortedByID)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDCHUNK);
   std::string chunkTableName = TABLE_NAME_CHUNKS;

   sqlWhereClause += " AND " + tableName + ".id=" + chunkTableName + ".id AND " + tableName
      + ".targetID=" + chunkTableName + ".targetID AND " + tableName
      + ".buddyGroupID=" + chunkTableName + ".buddyGroupID";

   std::string queryStr = "SELECT " + chunkTableName + ".* FROM " + tableName + "," +
      chunkTableName + " WHERE " + sqlWhereClause;

   if (sortedByID)
      queryStr += " ORDER BY id";

   DBCursor<FsckChunk>* cursor = new DBCursor<FsckChunk>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * retrieves a cursor to a set of dir inodes without cont dirs
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirInode>* FsckDB::getInodesWithoutContDirInternal(std::string sqlWhereClause)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_MISSINGCONTDIR);
   std::string inodeTableName = TABLE_NAME_DIR_INODES;

   sqlWhereClause += " AND " + tableName + ".id=" + inodeTableName + ".id";

   std::string queryStr = "SELECT " + inodeTableName + ".* FROM " + tableName + "," +
      inodeTableName + " WHERE " + sqlWhereClause;

   DBCursor<FsckDirInode>* cursor = new DBCursor<FsckDirInode>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * retrieves a cursor to a set of content dirs without inodes
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckContDir>* FsckDB::getOrphanedContDirsInternal(std::string sqlWhereClause)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDCONTDIR);
   std::string contDirTableName = TABLE_NAME_CONT_DIRS;

   std::string compareField = "id";

   sqlWhereClause += " AND " + tableName + "." + compareField + "=" + contDirTableName + "." +
      compareField;

   std::string queryStr = "SELECT " + contDirTableName + ".* FROM " + tableName + "," +
      contDirTableName + " WHERE " + sqlWhereClause;

   DBCursor<FsckContDir>* cursor = new DBCursor<FsckContDir>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * retrieves a cursor to a set of file inodes with wrong stat attributes (filesize, ctime, ...)
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckFileInode>* FsckDB::getInodesWithWrongFileAttribsInternal(std::string sqlWhereClause)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_WRONGFILEATTRIBS);
   std::string inodeTableName = TABLE_NAME_FILE_INODES;

   std::string compareField = "internalID";

   sqlWhereClause += " AND " + tableName + "." + compareField + "=" + inodeTableName + "." +
      compareField;

   std::string queryStr = "SELECT " + inodeTableName + ".* FROM " + tableName + "," +
      inodeTableName + " WHERE " + sqlWhereClause;

   DBCursor<FsckFileInode>* cursor = new DBCursor<FsckFileInode>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * retrieves a cursor to a set of dir inodes with wrong attributes
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirInode>* FsckDB::getInodesWithWrongDirAttribsInternal(std::string sqlWhereClause)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_WRONGDIRATTRIBS);
   std::string inodeTableName = TABLE_NAME_DIR_INODES;

   std::string compareField = "id";

   sqlWhereClause += " AND " + tableName + "." + compareField + "=" + inodeTableName + "." +
      compareField;

   std::string queryStr = "SELECT " + inodeTableName + ".* FROM " + tableName + "," +
      inodeTableName + " WHERE " + sqlWhereClause;

   DBCursor<FsckDirInode>* cursor = new DBCursor<FsckDirInode>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * retrieves a cursor to a set of stripe targets / buddy group ids, which are used in stripe
 * pattern, but do not exist
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckTargetID>* FsckDB::getMissingStripeTargetsInternal(std::string sqlWhereClause)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_MISSINGTARGET);

   std::string queryStr = "SELECT * FROM " + tableName + " WHERE " + sqlWhereClause;

   DBCursor<FsckTargetID>* cursor = new DBCursor<FsckTargetID>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * retrieves a cursor to a set of dir entries, which symbolize files, that are using non-existant
 * stripe targets
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckDirEntry>* FsckDB::getFilesWithMissingStripeTargetsInternal(std::string sqlWhereClause)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_FILEWITHMISSINGTARGET);
   std::string tableNameDentries = TABLE_NAME_DIR_ENTRIES;

   std::string queryStr = "SELECT " + tableNameDentries + ".* FROM " + tableName + ","
      + tableNameDentries + " WHERE (" + sqlWhereClause + ") AND " + tableName + ".name="
      + tableNameDentries + ".name AND " + tableName + ".parentDirID="
      + tableNameDentries + ".parentDirID AND " + tableName + ".saveNodeID="
      + tableNameDentries + ".saveNodeID";

   DBCursor<FsckDirEntry>* cursor = new DBCursor<FsckDirEntry>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * retrieves a cursor to a set of chunks, for which the mirror chunk is missing
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getMissingMirrorChunksInternal(std::string sqlWhereClause,
   bool sortedByID)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_MISSINGMIRRORCHUNK);
   std::string chunkTableName = TABLE_NAME_CHUNKS;

   sqlWhereClause += " AND " + tableName + ".id=" + chunkTableName + ".id AND " + tableName
      + ".targetID=" + chunkTableName + ".targetID AND " + tableName
      + ".buddyGroupID=" + chunkTableName + ".buddyGroupID";

   std::string queryStr = "SELECT " + chunkTableName + ".* FROM " + tableName + "," +
      chunkTableName + " WHERE " + sqlWhereClause;

   if (sortedByID)
      queryStr += " ORDER BY id";

   DBCursor<FsckChunk>* cursor = new DBCursor<FsckChunk>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * retrieves a cursor to a set of chunks, for which the primary chunk is missing, but a mirror
 * exists
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getMissingPrimaryChunksInternal(std::string sqlWhereClause,
   bool sortedByID)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_MISSINGPRIMARYCHUNK);
   std::string chunkTableName = TABLE_NAME_CHUNKS;

   sqlWhereClause += " AND " + tableName + ".id=" + chunkTableName + ".id AND " + tableName
      + ".targetID=" + chunkTableName + ".targetID AND " + tableName
      + ".buddyGroupID=" + chunkTableName + ".buddyGroupID";

   std::string queryStr = "SELECT " + chunkTableName + ".* FROM " + tableName + "," +
      chunkTableName + " WHERE " + sqlWhereClause;

   if (sortedByID)
      queryStr += " ORDER BY id";

   DBCursor<FsckChunk>* cursor = new DBCursor<FsckChunk>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * retrieves a cursor to a set of chunks, where the attribs of the primary and the mirror chunk
 * differ
 *
 * NOTE: maps to the mirror chunk, not the primary one!
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getDifferingChunkAttribsInternal(std::string sqlWhereClause,
   bool sortedByID)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_DIFFERINGCHUNKATTRIBS);
   std::string chunkTableName = TABLE_NAME_CHUNKS;

   sqlWhereClause += " AND " + tableName + ".id=" + chunkTableName + ".id AND " + tableName
      + ".targetID=" + chunkTableName + ".targetID AND " + tableName
      + ".buddyGroupID=" + chunkTableName + ".buddyGroupID";

   std::string queryStr = "SELECT " + chunkTableName + ".* FROM " + tableName + "," +
      chunkTableName + " WHERE " + sqlWhereClause;

   if (sortedByID)
      queryStr += " ORDER BY id";

   DBCursor<FsckChunk>* cursor = new DBCursor<FsckChunk>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * retrieves a cursor to a set of chunks, which have wrong uid/gid
 *
 * important for quota
 *
 * @param sqlWhereClause the where clause of the SQL query to execute; defaults to "1"
 *
 * @return a DBCursor pointer; make sure to delete it after use
 */
DBCursor<FsckChunk>* FsckDB::getChunksWithWrongPermissionsInternal(std::string sqlWhereClause,
   bool sortedByID)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_CHUNKWITHWRONGPERM);
   std::string chunkTableName = TABLE_NAME_CHUNKS;

   sqlWhereClause += " AND " + tableName + ".id=" + chunkTableName + ".id AND " + tableName
      + ".targetID=" + chunkTableName + ".targetID AND " + tableName
      + ".buddyGroupID=" + chunkTableName + ".buddyGroupID";

   std::string queryStr = "SELECT " + chunkTableName + ".* FROM " + tableName + "," +
      chunkTableName + " WHERE " + sqlWhereClause;

   if (sortedByID)
      queryStr += " ORDER BY id";

   DBCursor<FsckChunk>* cursor = new DBCursor<FsckChunk>(this->dbHandlePool, queryStr);

   return cursor;
}

DBCursor<FsckChunk>* FsckDB::getChunksInWrongPathInternal(std::string sqlWhereClause,
   bool sortedByID)
{
   std::string tableName = TABLE_NAME_ERROR(FsckErrorCode_CHUNKINWRONGPATH);
   std::string chunkTableName = TABLE_NAME_CHUNKS;

   sqlWhereClause += " AND " + tableName + ".id=" + chunkTableName + ".id AND " + tableName
      + ".targetID=" + chunkTableName + ".targetID AND " + tableName
      + ".buddyGroupID=" + chunkTableName + ".buddyGroupID";

   std::string queryStr = "SELECT " + chunkTableName + ".* FROM " + tableName + "," +
      chunkTableName + " WHERE " + sqlWhereClause;

   if (sortedByID)
      queryStr += " ORDER BY id";

   DBCursor<FsckChunk>* cursor = new DBCursor<FsckChunk>(this->dbHandlePool, queryStr);

   return cursor;
}

/*
 * get the amount of rows in a given table
 *
 * @param tableName the name of the table
 * @param whereClause a where clause to apply before counting (defaults to "1")
 * @return the row count
 */
int64_t FsckDB::getRowCount(std::string tableName, std::string whereClause)
{
   sqlite3_stmt* stmt = NULL;
   int64_t retVal = -1;

   std::string queryStr = "SELECT COUNT() FROM " + tableName + " WHERE " + whereClause;

   DBHandle *dbHandle = this->dbHandlePool->acquireHandle();

   // SQLITE_BUSY should basically be impossible, but as we cannot guarantee it, we have to handle
   // it
   int sqlite_retVal = dbHandle->sqliteBlockingPrepare(queryStr.c_str(), queryStr.length(), &stmt);

   if ( sqlite_retVal != SQLITE_OK )
   {
      goto cleanup;
   }

   if ( dbHandle->sqliteBlockingStep(stmt) == SQLITE_ROW )
   {
      retVal = sqlite3_column_int64(stmt, 0);
   }

   cleanup:
   // sqlite3_finalize frees memory used by stmt
   if ( stmt )
      sqlite3_finalize(stmt);

   this->dbHandlePool->releaseHandle(dbHandle);

   return retVal;
}

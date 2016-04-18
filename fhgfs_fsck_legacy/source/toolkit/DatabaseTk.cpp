#include "DatabaseTk.h"

#include <common/app/log/LogContext.h>
#include <common/storage/striping/Raid0Pattern.h>
#include <common/toolkit/FsckTk.h>
#include <common/toolkit/StringTk.h>
#include <database/FsckDB.h>
#include <common/storage/PathVec.h>
#include <common/toolkit/StorageTk.h>

DatabaseTk::DatabaseTk()
{
}

DatabaseTk::~DatabaseTk()
{
}

FsckDirEntry DatabaseTk::createDummyFsckDirEntry(FsckDirEntryType entryType)
{
   FsckDirEntryList list;
   createDummyFsckDirEntries(1, &list, entryType);
   return list.front();
}

void DatabaseTk::createDummyFsckDirEntries(uint amount, FsckDirEntryList* outList,
   FsckDirEntryType entryType)
{
   createDummyFsckDirEntries(0,amount, outList, entryType);
}

void DatabaseTk::createDummyFsckDirEntries(uint from, uint amount, FsckDirEntryList* outList,
   FsckDirEntryType entryType)
{
   for ( uint i = from; i < (from+amount); i++ )
   {
      std::string index = StringTk::uintToStr(i);

      std::string id = "entryID" + index;
      std::string name = "dentryName" + index;
      std::string parentID = "parentID" + index;
      uint16_t entryOwnerNodeID = i + 1000;
      uint16_t inodeOwnerNodeID = i + 2000;
      uint16_t saveNodeID = i + 3000;
      int saveDevice = i;
      uint64_t saveInode = i;

      FsckDirEntry dentry(id, name, parentID, entryOwnerNodeID, inodeOwnerNodeID, entryType, true,
         saveNodeID, saveDevice, saveInode);

      outList->push_back(dentry);
   }
}

FsckFileInode DatabaseTk::createDummyFsckFileInode()
{
   FsckFileInodeList list;
   createDummyFsckFileInodes(1, &list);
   return list.front();
}

void DatabaseTk::createDummyFsckFileInodes(uint amount, FsckFileInodeList* outList)
{
   createDummyFsckFileInodes(0,amount,outList);
}

void DatabaseTk::createDummyFsckFileInodes(uint from, uint amount, FsckFileInodeList* outList)
{
   for ( uint i = from; i < (from + amount); i++ )
   {
      std::string index = StringTk::uintToStr(i);
      UInt16Vector targetIDs;
      targetIDs.push_back(i + 1000);
      targetIDs.push_back(i + 2000);
      targetIDs.push_back(i + 3000);
      targetIDs.push_back(i + 4000);
      Raid0Pattern stripePatternIn(1024, targetIDs);

      std::string id = "entryID" + index;
      std::string parentDirID = "parentDir" + index;
      uint16_t parentNodeID = i + 1000;

      int mode = S_IRWXU;
      unsigned userID = 1000;
      unsigned groupID = 1000;

      uint64_t usedBlocks = 0;
      int64_t fileSize = usedBlocks*512;
      int64_t creationTime = 15000000;
      int64_t modificationTime = 15000000;
      int64_t lastAccessTime = 15000000;
      unsigned numHardLinks = 1;

      PathInfo pathInfo(userID, parentDirID, PATHINFO_FEATURE_ORIG);

      FsckStripePatternType stripePatternType = FsckTk::stripePatternToFsckStripePattern(
         &stripePatternIn);
      unsigned chunkSize = 524288;

      uint16_t saveNodeID = i + 2000;

      FsckFileInode fileInode(id, parentDirID, parentNodeID, pathInfo, mode, userID, groupID,
         fileSize, creationTime, modificationTime, lastAccessTime, numHardLinks, usedBlocks,
         targetIDs, stripePatternType, chunkSize, saveNodeID, true);

      outList->push_back(fileInode);
   }
}

FsckDirInode DatabaseTk::createDummyFsckDirInode()
{
   FsckDirInodeList list;
   createDummyFsckDirInodes(1, &list);
   return list.front();
}

void DatabaseTk::createDummyFsckDirInodes(uint amount, FsckDirInodeList* outList)
{
   createDummyFsckDirInodes(0, amount, outList);
}

void DatabaseTk::createDummyFsckDirInodes(uint from, uint amount, FsckDirInodeList* outList)
{
   for ( uint i = from; i < (from + amount); i++ )
   {
      std::string index = StringTk::uintToStr(i);
      UInt16Vector targetIDs;
      targetIDs.push_back(i + 1000);
      targetIDs.push_back(i + 2000);
      targetIDs.push_back(i + 3000);
      targetIDs.push_back(i + 4000);
      Raid0Pattern stripePatternIn(1024, targetIDs);

      std::string id = "entryID" + index;
      std::string parentDirID = "parentDir" + index;
      uint16_t parentNodeID = i + 1000;
      uint16_t ownerNodeID = i + 2000;

      int64_t size = 10;
      unsigned numHardLinks = 12;

      FsckStripePatternType stripePatternType = FsckTk::stripePatternToFsckStripePattern(
         &stripePatternIn);
      uint16_t saveNodeID = i + 3000;

      FsckDirInode dirInode(id, parentDirID, parentNodeID, ownerNodeID, size, numHardLinks,
         targetIDs, stripePatternType, saveNodeID);
      outList->push_back(dirInode);
   }
}

FsckChunk DatabaseTk::createDummyFsckChunk()
{
   FsckChunkList list;
   createDummyFsckChunks(1, &list);
   return list.front();
}

void DatabaseTk::createDummyFsckChunks(uint amount, FsckChunkList* outList)
{
   createDummyFsckChunks(amount, 1, outList);
}

void DatabaseTk::createDummyFsckChunks(uint amount, uint numTargets, FsckChunkList* outList)
{
   createDummyFsckChunks(0, amount, numTargets, outList);
}

void DatabaseTk::createDummyFsckChunks(uint from, uint amount, uint numTargets,
   FsckChunkList* outList)
{
   for ( uint i = from; i < (from + amount); i++ )
   {
      std::string index = StringTk::uintToStr(i);

      for ( uint j = 0; j < numTargets; j++ )
      {
         std::string id = "entryID" + index;
         uint16_t targetID = j;

         PathInfo pathInfo(0, META_ROOTDIR_ID_STR,  PATHINFO_FEATURE_ORIG);
         PathVec chunkDirPathVec; // ignored!
         std::string chunkFilePathStr;
         StorageTk::getChunkDirChunkFilePath(&pathInfo, id, true, chunkDirPathVec,
            chunkFilePathStr);
         Path savedPath(chunkFilePathStr);

         uint64_t usedBlocks = 300;
         int64_t fileSize = usedBlocks*512;
         int64_t creationTime = 15000000;
         int64_t modificationTime = 15000000;
         int64_t lastAccessTime = 15000000;

         FsckChunk chunk(id, targetID, savedPath, fileSize, usedBlocks, creationTime,
            modificationTime, lastAccessTime, 0, 0);
         outList->push_back(chunk);
      }
   }
}

FsckContDir DatabaseTk::createDummyFsckContDir()
{
   FsckContDirList list;
   createDummyFsckContDirs(1, &list);
   return list.front();
}

void DatabaseTk::createDummyFsckContDirs(uint amount, FsckContDirList* outList)
{
   createDummyFsckContDirs(0, amount, outList);
}

void DatabaseTk::createDummyFsckContDirs(uint from, uint amount, FsckContDirList* outList)
{
   for ( uint i = from; i < (from + amount); i++ )
   {
      std::string index = StringTk::uintToStr(i);

      std::string id = "entryID" + index;
      uint16_t saveNodeID = i;

      FsckContDir contDir(id, saveNodeID);
      outList->push_back(contDir);
   }
}

FsckFsID DatabaseTk::createDummyFsckFsID()
{
   FsckFsIDList list;
   createDummyFsckFsIDs(1, &list);
   return list.front();
}

void DatabaseTk::createDummyFsckFsIDs(uint amount, FsckFsIDList* outList)
{
   createDummyFsckFsIDs(0, amount, outList);
}

void DatabaseTk::createDummyFsckFsIDs(uint from, uint amount, FsckFsIDList* outList)
{
   for ( uint i = from; i < (from + amount); i++ )
   {
      std::string index = StringTk::uintToStr(i);

      std::string id = "entryID" + index;
      std::string parentDirID = "parentDirID" + index;
      uint16_t saveNodeID = i;
      int saveDevice = i;
      uint64_t saveInode = i;

      FsckFsID fsID(id, parentDirID, saveNodeID, saveDevice, saveInode);
      outList->push_back(fsID);
   }
}

void DatabaseTk::createFsckDirEntryFromRow(sqlite3_stmt* stmt, FsckDirEntry *outEntry,
   unsigned colShift)
{
   unsigned column = colShift;

   std::string entryID = (char*)sqlite3_column_text( stmt, column++ );
   std::string name = (char*)sqlite3_column_text( stmt, column++ );
   std::string parentID = (char*)sqlite3_column_text( stmt, column++ );
   uint16_t entryOwnerNodeID = (uint16_t)sqlite3_column_int( stmt, column++ );
   uint16_t inodeOwnerNodeID = (uint16_t)sqlite3_column_int( stmt, column++ );
   FsckDirEntryType entryType = (FsckDirEntryType) sqlite3_column_int( stmt, column++ );
   bool hasInlinedInode = (bool)sqlite3_column_int( stmt, column++ );
   uint16_t saveNodeID = (uint16_t)sqlite3_column_int( stmt, column++ );
   int saveDevice = (int)sqlite3_column_int( stmt, column++ );
   // saveInode is uint64_t, but sqlite int type can't handle uint64; therefore we saved this
   // as text
   uint64_t saveInode = StringTk::strToUInt64((char*)sqlite3_column_text( stmt, column++ ));

   outEntry->update(entryID, name, parentID, entryOwnerNodeID, inodeOwnerNodeID, entryType,
      hasInlinedInode, saveNodeID, saveDevice, saveInode);
}

void DatabaseTk::createFsckFileInodeFromRow(sqlite3_stmt* stmt, FsckFileInode *outInode,
   unsigned colShift)
{
   unsigned column = colShift;

   int64_t internalID = (int64_t)sqlite3_column_int64( stmt, column++ );

   std::string id = (char*)sqlite3_column_text( stmt, column++ );
   std::string parentDirID = (char*)sqlite3_column_text( stmt, column++ );
   uint16_t parentNodeID = (uint16_t)sqlite3_column_int( stmt, column++ );

   // pathInfo
   unsigned origParentUID = (unsigned)sqlite3_column_int64( stmt, column++ );
   std::string origParentEntryID = (char*)sqlite3_column_text( stmt, column++ );
   int pathInfoFlags = (int)sqlite3_column_int( stmt, column++ );
   PathInfo pathInfo(origParentUID, origParentEntryID, pathInfoFlags);

   int mode = (int)sqlite3_column_int( stmt, column++ );
   unsigned userID = (unsigned)sqlite3_column_int64( stmt, column++ );
   unsigned groupID = (unsigned)sqlite3_column_int64( stmt, column++ );

   int64_t fileSize = (int64_t)sqlite3_column_int64( stmt, column++ );
   // numBlocks is uint64_t, but sqlite int type can't handle uint64; therefore we saved this
   // as text
   uint64_t usedBlocks = StringTk::strToUInt64((char*)sqlite3_column_text( stmt, column++ ));
   int64_t creationTime = (int64_t)sqlite3_column_int64( stmt, column++ );
   int64_t modificationTime = (int64_t)sqlite3_column_int64( stmt, column++ );
   int64_t lastAccessTime = (int64_t)sqlite3_column_int64( stmt, column++ );
   unsigned numHardLinks = (unsigned)sqlite3_column_int64( stmt, column++ );

   FsckStripePatternType stripePatternType =
      (FsckStripePatternType)sqlite3_column_int( stmt, column++ );
   std::string stripeTargetsStr = (char*)sqlite3_column_text( stmt, column++ );

   UInt16Vector stripeTargets;
   StringTk::strToUint16Vec(stripeTargetsStr, &stripeTargets);

   unsigned chunkSize = sqlite3_column_int64( stmt, column++ );

   uint16_t saveNodeID = (uint16_t)sqlite3_column_int( stmt, column++ );
   bool isInlined = (bool)sqlite3_column_int( stmt, column++);

   bool readable = (bool)sqlite3_column_int( stmt, column++);

   outInode->update(internalID, id, parentDirID, parentNodeID, pathInfo, mode, userID, groupID,
      fileSize, usedBlocks, creationTime, modificationTime, lastAccessTime, numHardLinks,
      stripeTargets, stripePatternType, chunkSize, saveNodeID, isInlined, readable);
}

void DatabaseTk::createFsckDirInodeFromRow(sqlite3_stmt* stmt, FsckDirInode *outInode,
   unsigned colShift)
{
   unsigned column = colShift;

   std::string id = (char*)sqlite3_column_text( stmt, column++ );
   std::string parentDirID = (char*)sqlite3_column_text( stmt, column++ );
   uint16_t parentNodeID = (uint16_t)sqlite3_column_int( stmt, column++ );
   uint16_t ownerNodeID = (uint16_t)sqlite3_column_int( stmt, column++);

   int64_t size = (int64_t)sqlite3_column_int64( stmt, column++ );
   unsigned numHardLinks = (unsigned)sqlite3_column_int( stmt, column++ );

   FsckStripePatternType stripePatternType =
      (FsckStripePatternType)sqlite3_column_int( stmt, column++ );

   std::string stripeTargetsStr = (char*)sqlite3_column_text( stmt, column++ );

   UInt16Vector stripeTargets;
   StringTk::strToUint16Vec(stripeTargetsStr, &stripeTargets);

   uint16_t saveNodeID = (uint16_t)sqlite3_column_int( stmt, column++ );

   bool readable = (bool)sqlite3_column_int( stmt, column++);

   outInode->update(id, parentDirID, parentNodeID, ownerNodeID, size, numHardLinks, stripeTargets,
      stripePatternType, saveNodeID, readable);
}

void DatabaseTk::createFsckChunkFromRow(sqlite3_stmt* stmt, FsckChunk *outChunk,
   unsigned colShift)
{
   unsigned column = colShift;

   std::string id = (char*)sqlite3_column_text( stmt, column++ );
   uint16_t targetID = (uint16_t)sqlite3_column_int( stmt, column++ );

   std::string savedPathStr = (char*)sqlite3_column_text( stmt, column++ );
   Path savedPath(savedPathStr);

   int64_t fileSize = (int64_t)sqlite3_column_int64( stmt, column++ );
   // numBlocks is uint64_t, but sqlite int type can't handle uint64; therefore we saved this
   // as text
   uint64_t usedBlocks = StringTk::strToUInt64((char*)sqlite3_column_text( stmt, column++ ));

   int64_t creationTime = (int64_t)sqlite3_column_int64( stmt, column++ );
   int64_t modifictaionTime = (int64_t)sqlite3_column_int64( stmt, column++ );
   int64_t lastAccessTime = (int64_t)sqlite3_column_int64( stmt, column++ );

   unsigned userID = (unsigned)sqlite3_column_int64( stmt, column++ );
   unsigned groupID = (unsigned)sqlite3_column_int64( stmt, column++ );

   uint16_t buddyGroupID  = (uint16_t)sqlite3_column_int( stmt, column++ );

   outChunk->update(id, targetID, savedPath, fileSize, usedBlocks, creationTime, modifictaionTime,
      lastAccessTime, userID, groupID, buddyGroupID);
}

void DatabaseTk::createFsckContDirFromRow(sqlite3_stmt* stmt, FsckContDir *outContDir,
   unsigned colShift)
{
   unsigned column = colShift;

   std::string id = (char*)sqlite3_column_text( stmt, column++ );
   uint16_t saveNodeID = (uint16_t)sqlite3_column_int( stmt, column++ );

   outContDir->update(id, saveNodeID);
}

void DatabaseTk::createFsckFsIDFromRow(sqlite3_stmt* stmt, FsckFsID *outFsID,
   unsigned colShift)
{
   unsigned column = colShift;

   std::string id = (char*)sqlite3_column_text( stmt, column++ );
   std::string parentDirID = (char*)sqlite3_column_text( stmt, column++ );
   uint16_t saveNodeID = (uint16_t)sqlite3_column_int( stmt, column++ );
   int saveDevice = (int)sqlite3_column_int( stmt, column++ );
   // saveInode is uint64_t, but sqlite int type can't handle uint64; therefore we saved this
   // as text
   uint64_t saveInode = StringTk::strToUInt64((char*)sqlite3_column_text( stmt, column++ ));

   outFsID->update(id, parentDirID, saveNodeID, saveDevice, saveInode);
}

void DatabaseTk::createFsckModificationEventFromRow(sqlite3_stmt* stmt,
   FsckModificationEvent *outModificationEvent, unsigned colShift)
{
   unsigned column = colShift;

   ModificationEventType eventType = (ModificationEventType) sqlite3_column_int(stmt,
      column++);
   std::string entryID = (char*)sqlite3_column_text( stmt, column++ );

   outModificationEvent->update(eventType, entryID);
}

void DatabaseTk::createFsckTargetIDFromRow(sqlite3_stmt* stmt, FsckTargetID *outTargetID,
   unsigned colShift)
{
   unsigned column = colShift;

   uint16_t id = (uint16_t)sqlite3_column_int( stmt, column++ );
   int type = (int)sqlite3_column_int( stmt, column++ );

   outTargetID->update(id, (FsckTargetIDType)type);
}

void DatabaseTk::createObjectFromRow(sqlite3_stmt* stmt, FsckDirEntry* outObject,
   unsigned colShift)
{
   DatabaseTk::createFsckDirEntryFromRow(stmt, outObject, colShift);
}

void DatabaseTk::createObjectFromRow(sqlite3_stmt* stmt, FsckFileInode* outObject,
   unsigned colShift)
{
   DatabaseTk::createFsckFileInodeFromRow(stmt, outObject, colShift);
}

void DatabaseTk::createObjectFromRow(sqlite3_stmt* stmt, FsckDirInode* outObject,
   unsigned colShift)
{
   DatabaseTk::createFsckDirInodeFromRow(stmt, outObject, colShift);
}

void DatabaseTk::createObjectFromRow(sqlite3_stmt* stmt, FsckChunk* outObject,
   unsigned colShift)
{
   DatabaseTk::createFsckChunkFromRow(stmt, outObject, colShift);
}

void DatabaseTk::createObjectFromRow(sqlite3_stmt* stmt, FsckContDir* outObject,
   unsigned colShift)
{
   DatabaseTk::createFsckContDirFromRow(stmt, outObject, colShift);
}

void DatabaseTk::createObjectFromRow(sqlite3_stmt* stmt, FsckFsID* outObject,
   unsigned colShift)
{
   DatabaseTk::createFsckFsIDFromRow(stmt, outObject, colShift);
}

void DatabaseTk::createObjectFromRow(sqlite3_stmt* stmt, FsckModificationEvent* outObject,
   unsigned colShift)
{
   DatabaseTk::createFsckModificationEventFromRow(stmt, outObject, colShift);
}

void DatabaseTk::createObjectFromRow(sqlite3_stmt* stmt, FsckTargetID* outObject,
   unsigned colShift)
{
   DatabaseTk::createFsckTargetIDFromRow(stmt, outObject, colShift);
}

bool DatabaseTk::getFullPath(FsckDB* db, FsckDirEntry* dirEntry, std::string& outPath)
{
   if (!dirEntry)
   {
      outPath = "[<not available>]";
      return false;
   }

   outPath = "/" + dirEntry->getName();

   std::string parentEntryID = dirEntry->getParentDirID();

   int depth = 0;

   while ( parentEntryID != META_ROOTDIR_ID_STR )
   {
      depth++;

      if (depth > 255)
      {
         outPath = "[<unresolved>]" + outPath;
         return false;
      }

      /*
       * only get the first dentry; this information is important for the database, even if there is
       * only one matching dentry. After the match the database will search further until the end of
       * the database is reached, which can take a long time. By limiting the result set it will
       * definitely stop after the first match
       */
      DBCursor<FsckDirEntry>* cursor = db->getDirEntries(parentEntryID, 1);
      FsckDirEntry* currentDirEntry = cursor->open();

      if ( !currentDirEntry )
      {
         cursor->close();
         SAFE_DELETE(cursor);
         outPath = "[<unresolved>]" + outPath;
         return false;
      }

      outPath = "/" + currentDirEntry->getName() + outPath;

      parentEntryID = currentDirEntry->getParentDirID();

      cursor->close();
      SAFE_DELETE(cursor);
   }

   return true;
}

/*
 * only returns the path of the first dentry matching the given id (might be more due to hardlinks)
 */
bool DatabaseTk::getFullPath(FsckDB* db, std::string id, std::string& outPath)
{
   bool retVal = false;

   /*
    * only get the first dentry; this information is important for the database, even if there is
    * only one matching dentry. After the match the database will search further until the end of
    * the database is reached, which can take a long time. By limiting the result set it will
    * definitely stop after the first match
    */
   DBCursor<FsckDirEntry>* cursor = db->getDirEntries(id, 1);

   FsckDirEntry* dirEntry = cursor->open();

   if (dirEntry)
      retVal = getFullPath(db, dirEntry, outPath);
   else
      outPath = "[<not available>]";

   cursor->close();
   SAFE_DELETE(cursor);

   return retVal;
}

std::string DatabaseTk::calculateExpectedChunkPath(std::string entryID, unsigned origParentUID,
   std::string origParentEntryID, int pathInfoFlags)
{
   std::string resStr;

   bool hasOrigFeature;
   if (pathInfoFlags == PATHINFO_FEATURE_ORIG)
      hasOrigFeature = true;
   else
      hasOrigFeature = false;

   PathInfo pathInfo(origParentUID, origParentEntryID, pathInfoFlags);
   PathVec chunkDirPath;
   std::string chunkFilePath;
   StorageTk::getChunkDirChunkFilePath(&pathInfo, entryID, hasOrigFeature, chunkDirPath,
      chunkFilePath);

   if (hasOrigFeature) // we can use the PathVec chunkDirPath
      resStr = chunkDirPath.getPathAsStr();
   else
   {
      /* chunk in old format => PathVec chunkDirPath is not set in getChunkDirChunkFilePath(),
       * so it is a bit more tricky */

      // we need to strip the filename itself (including the '/')
      size_t startPos = 0;
      size_t len = chunkFilePath.find_last_of('/');

      // generate a substring as result
      resStr = chunkFilePath.substr(startPos, len);
   }

   return resStr;
}

bool DatabaseTk::removeDatabaseFiles(const std::string& databasePath)
{
   size_t arraySize = sizeof(dbSchemas) / sizeof(dbSchemas[0]);

   for ( size_t i = 0; i < arraySize; i++ )
   {
      std::string filepath = databasePath + "/" + dbSchemas[i] + ".db";
      if ( StorageTk::pathExists(databasePath) )
      {
         int unlinkRes = unlink(filepath.c_str());
         if ( (unlinkRes == -1) && (errno != ENOENT) )
         {
            LogContext(__func__).logErr(
               "Unable to unlink file. Filepath: " + filepath + "; errorStr: "
                  + System::getErrString());

            return false;
         }
      }
   }

   // remove main schema file
   std::string filepath = databasePath + "/" + DATABASE_MAINSCHEMANAME + ".db";
   int unlinkRes = unlink(filepath.c_str());
   if ( (unlinkRes == -1) && (errno != ENOENT) )
   {
      LogContext(__func__).logErr(
         "Unable to unlink file. Filepath: " + filepath + "; errorStr: "
            + System::getErrString());

      return false;
   }

   return true;
}

/*
 * check if all database files exist in database path
 *
 * @return true if all exist, false if one or more are missing
 */
bool DatabaseTk::databaseFilesExist(const std::string& databasePath)
{
   size_t arraySize = sizeof(dbSchemas) / sizeof(dbSchemas[0]);

   // check main schema file
   std::string filepath = databasePath + "/" + DATABASE_MAINSCHEMANAME + ".db";
   if ( ! StorageTk::pathExists(filepath) )
         return false;

   // check additional schema files
   for ( size_t i = 0; i < arraySize; i++ )
   {
      std::string filepath = databasePath + "/" + dbSchemas[i] + ".db";
      if ( ! StorageTk::pathExists(filepath) )
            return false;
   }

   return true;
}

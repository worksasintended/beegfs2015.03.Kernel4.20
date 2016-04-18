#include "DatabaseTk.h"

#include <common/app/log/LogContext.h>
#include <common/storage/striping/Raid0Pattern.h>
#include <common/toolkit/FsckTk.h>
#include <common/toolkit/StringTk.h>
#include <database/FsckDB.h>
#include <database/FsckDBTable.h>
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
   createDummyFsckDirEntries(7, 1, &list, entryType);
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
      std::string index = StringTk::uintToHexStr(i);

      std::string id = index + "-1-1";
      std::string name = "dentryName" + index;
      std::string parentID = index + "-2-1";
      uint16_t entryOwnerNodeID(i + 1000);
      uint16_t inodeOwnerNodeID(i + 2000);
      uint16_t saveNodeID(i + 3000);
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
   createDummyFsckFileInodes(7, 1, &list);
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
      std::string index = StringTk::uintToHexStr(i);
      UInt16Vector targetIDs;
      while (targetIDs.size() < i)
         targetIDs.push_back(i + 1000 * (targetIDs.size() + 1) );
      Raid0Pattern stripePatternIn(1024, targetIDs);

      std::string id = index + "-1-1";
      std::string parentDirID = index + "-2-1";
      uint16_t parentNodeID(i + 1000);

      unsigned userID = 1000;
      unsigned groupID = 1000;

      uint64_t usedBlocks = 0;
      int64_t fileSize = usedBlocks*512;
      unsigned numHardLinks = 1;

      PathInfo pathInfo(userID, parentDirID, PATHINFO_FEATURE_ORIG);

      FsckStripePatternType stripePatternType = FsckTk::stripePatternToFsckStripePattern(
         &stripePatternIn);
      unsigned chunkSize = 524288;

      uint16_t saveNodeID(i + 2000);

      FsckFileInode fileInode(id, parentDirID, parentNodeID, pathInfo, 0, userID, groupID,
         fileSize, 0, 0, 0, numHardLinks, usedBlocks,
         targetIDs, stripePatternType, chunkSize, saveNodeID, true);

      fileInode.setSaveInode(1000 + i);
      fileInode.setSaveDevice(42);

      outList->push_back(fileInode);
   }
}

FsckDirInode DatabaseTk::createDummyFsckDirInode()
{
   FsckDirInodeList list;
   createDummyFsckDirInodes(7, 1, &list);
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
      std::string index = StringTk::uintToHexStr(i);
      UInt16Vector targetIDs;
      Raid0Pattern stripePatternIn(1024, targetIDs);

      std::string id = index + "-1-1";
      std::string parentDirID = index + "-2-1";
      uint16_t parentNodeID(i + 1000);
      uint16_t ownerNodeID(i + 2000);

      int64_t size = 10;
      unsigned numHardLinks = 12;

      FsckStripePatternType stripePatternType = FsckTk::stripePatternToFsckStripePattern(
         &stripePatternIn);
      uint16_t saveNodeID(i + 3000);

      FsckDirInode dirInode(id, parentDirID, parentNodeID, ownerNodeID, size, numHardLinks,
         targetIDs, stripePatternType, saveNodeID);
      outList->push_back(dirInode);
   }
}

FsckChunk DatabaseTk::createDummyFsckChunk()
{
   FsckChunkList list;
   createDummyFsckChunks(7, 1, &list);
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
      std::string index = StringTk::uintToHexStr(i);

      for ( uint j = 0; j < numTargets; j++ )
      {
         std::string id = index + "-1-1";
         uint16_t targetID = j;

         PathInfo pathInfo(0, META_ROOTDIR_ID_STR,  PATHINFO_FEATURE_ORIG);
         PathVec chunkDirPathVec; // ignored!
         std::string chunkFilePathStr;
         StorageTk::getChunkDirChunkFilePath(&pathInfo, id, true, chunkDirPathVec,
            chunkFilePathStr);
         Path savedPath(chunkFilePathStr);

         uint64_t usedBlocks = 300;
         int64_t fileSize = usedBlocks*512;

         FsckChunk chunk(id, targetID, savedPath, fileSize, usedBlocks, 0, 0, 0, 0, 0);
         outList->push_back(chunk);
      }
   }
}

FsckContDir DatabaseTk::createDummyFsckContDir()
{
   FsckContDirList list;
   createDummyFsckContDirs(7, 1, &list);
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
      std::string index = StringTk::uintToHexStr(i);

      std::string id = index + "-1-1";
      uint16_t saveNodeID(i);

      FsckContDir contDir(id, saveNodeID);
      outList->push_back(contDir);
   }
}

FsckFsID DatabaseTk::createDummyFsckFsID()
{
   FsckFsIDList list;
   createDummyFsckFsIDs(7, 1, &list);
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
      std::string index = StringTk::uintToHexStr(i);

      std::string id = index + "-1-1";
      std::string parentDirID = index + "-2-1";
      uint16_t saveNodeID(i);
      int saveDevice = i;
      uint64_t saveInode = i;

      FsckFsID fsID(id, parentDirID, saveNodeID, saveDevice, saveInode);
      outList->push_back(fsID);
   }
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

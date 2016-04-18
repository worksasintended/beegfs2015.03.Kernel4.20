#ifndef FSCKFILEINODE_H
#define FSCKFILEINODE_H

#include <common/Common.h>
#include <common/toolkit/FsckTk.h>
#include <common/storage/StatData.h>

class FsckFileInode;

typedef std::list<FsckFileInode> FsckFileInodeList;
typedef FsckFileInodeList::iterator FsckFileInodeListIter;

class FsckFileInode
{
      friend class TestDatabase;

   public:
      FsckFileInode(std::string id, std::string parentDirID, uint16_t parentNodeID,
         PathInfo& pathInfo, int mode, unsigned userID, unsigned groupID,
         int64_t fileSize, int64_t creationTime, int64_t modificationTime, int64_t lastAccessTime,
         unsigned numHardLinks, uint64_t usedBlocks, UInt16Vector& stripeTargets,
         FsckStripePatternType stripePatternType, unsigned chunkSize, uint16_t saveNodeID,
         bool isInlined, bool readable = true)
      {
         SettableFileAttribs settableFileAttribs;

         settableFileAttribs.mode = mode;
         settableFileAttribs.userID = userID;
         settableFileAttribs.groupID = groupID;
         settableFileAttribs.modificationTimeSecs = modificationTime;
         settableFileAttribs.lastAccessTimeSecs = lastAccessTime;

         StatData statData(fileSize, &settableFileAttribs, creationTime, 0, numHardLinks, 0);

         initialize(id, parentDirID, parentNodeID, pathInfo, &statData, usedBlocks, stripeTargets,
            stripePatternType, chunkSize, saveNodeID, isInlined, readable);
      }

      FsckFileInode(std::string id, std::string parentDirID, uint16_t parentNodeID,
         PathInfo& pathInfo, StatData* statData, uint64_t usedBlocks, UInt16Vector& stripeTargets,
         FsckStripePatternType stripePatternType, unsigned chunkSize, uint16_t saveNodeID,
         bool isInlined, bool readable = true)
      {
         initialize(id, parentDirID, parentNodeID, pathInfo, statData, usedBlocks, stripeTargets,
            stripePatternType, chunkSize, saveNodeID, isInlined, readable);
      }

      FsckFileInode()
      {
         this->internalID = 0;
      }

      void initialize(std::string& id, std::string& parentDirID, uint16_t parentNodeID,
         PathInfo& pathInfo, StatData* statData, uint64_t usedBlocks, UInt16Vector& stripeTargets,
         FsckStripePatternType stripePatternType, unsigned chunkSize, uint16_t saveNodeID,
         bool isInlined, bool readable)
      {
         this->internalID = 0;
         this->id = id;
         this->parentDirID = parentDirID;
         this->parentNodeID = parentNodeID;

         /* check pathInfo; deserialization from disk does not set origParentEntryID, if file was
          * never moved. On fsck side, we need this set, so we set it to the current parent dir
          * here */
         if (pathInfo.getOrigParentEntryID().empty())
            pathInfo.setOrigParentEntryID(parentDirID);

         this->pathInfo = pathInfo;
         this->mode = statData->getMode();
         this->userID = statData->getUserID();
         this->groupID = statData->getGroupID();
         this->fileSize = statData->getFileSize();
         this->creationTime = statData->getCreationTimeSecs();
         this->modificationTime = statData->getModificationTimeSecs();
         this->lastAccessTime = statData->getLastAccessTimeSecs();
         this->numHardLinks = statData->getNumHardlinks();
         this->usedBlocks = usedBlocks;
         this->stripeTargets = stripeTargets;
         this->stripePatternType = stripePatternType;
         this->chunkSize = chunkSize;
         this->saveNodeID = saveNodeID;
         this->isInlined = isInlined;
         this->readable = readable;
      }



      size_t serialize(char* outBuf);
      bool deserialize(const char* buf, size_t bufLen, unsigned* outLen);
      unsigned serialLen();

   private:
      /*
       * we need an "internal" ID for the database here (which will be autoincremented by DB),
       * because otherwise it is not possible to uniquely identify a file inode entry, as it can
       * happen that entry IDs are present more than once (e.g. if fsck was run online and a file
       * was moved and therefore gathered twice);
       * for newly created inodes, this id should always be 0, so that the DB assigns an ID
       * (starting with 1)
       */
      uint64_t internalID;

      std::string id; // filesystem-wide unique string
      std::string parentDirID;
      uint16_t parentNodeID;

      PathInfo pathInfo;

      int mode;
      unsigned userID;
      unsigned groupID;

      uint64_t usedBlocks; // 512byte-blocks
      int64_t fileSize; // in byte
      int64_t creationTime; // secs since the epoch
      int64_t modificationTime; // secs since the epoch
      int64_t lastAccessTime; // secs since the epoch
      unsigned numHardLinks;

      UInt16Vector stripeTargets;
      FsckStripePatternType stripePatternType;
      unsigned chunkSize;

      uint16_t saveNodeID; // id of the node, where this inode is saved on

      bool isInlined;

      bool readable;

   public:
      uint64_t getInternalID() const
      {
         return internalID;
      }

      void setInternalID(uint64_t internalID)
      {
         this->internalID = internalID;
      }

      const std::string& getID() const
      {
         return id;
      }

      const std::string& getParentDirID() const
      {
         return parentDirID;
      }

      void setParentDirID(const std::string& parentDirID)
      {
         this->parentDirID = parentDirID;
      }

      uint16_t getParentNodeID() const
      {
         return parentNodeID;
      }

      void setParentNodeID(uint16_t parentNodeID)
      {
         this->parentNodeID = parentNodeID;
      }

      PathInfo* getPathInfo()
      {
         return &(this->pathInfo);
      }

      int getMode() const
      {
         return mode;
      }

      void setMode(int mode)
      {
         this->mode = mode;
      }

      unsigned getUserID() const
      {
         return this->userID;
      }

      void setUserID(unsigned userID)
      {
         this->userID = userID;
      }

      unsigned getGroupID() const
      {
         return this->groupID;
      }

      void setGroupID(unsigned groupID)
      {
         this->groupID = groupID;
      }

      int64_t getFileSize() const
      {
         return this->fileSize;
      }

      void setFileSize(int64_t fileSize)
      {
         this->fileSize = fileSize;
      }

      int64_t getCreationTime() const
      {
         return this->creationTime;
      }

      void setCreationTime(int64_t creationTime)
      {
         this->creationTime = creationTime;
      }

      int64_t getModificationTime() const
      {
         return this->modificationTime;
      }

      void setModificationTime(int64_t modificationTime)
      {
         this->modificationTime = modificationTime;
      }

      int64_t getLastAccessTime() const
      {
         return this->lastAccessTime;
      }

      void setLastAccessTime(int64_t lastAccessTime)
      {
         this->lastAccessTime = lastAccessTime;
      }

      unsigned getNumHardLinks() const
      {
         return this->numHardLinks;
      }

      void setNumHardLinks(unsigned numHardLinks)
      {
         this->numHardLinks = numHardLinks;
      }

      uint64_t getUsedBlocks() const
      {
         return this->usedBlocks;
      }

      UInt16Vector getStripeTargets() const
      {
         return stripeTargets;
      }

      UInt16Vector* getStripeTargets()
      {
         return &stripeTargets;
      }

      void setStripeTargets(UInt16Vector& stripeTargets)
      {
         this->stripeTargets = stripeTargets;
      }

      FsckStripePatternType getStripePatternType() const
      {
         return stripePatternType;
      }

      void setStripePatternType(FsckStripePatternType stripePatternType)
      {
         this->stripePatternType = stripePatternType;
      }

      unsigned getChunkSize() const
      {
         return chunkSize;
      }

      void setChunkSize(unsigned chunkSize)
      {
         this->chunkSize = chunkSize;
      }

      uint16_t getSaveNodeID() const
      {
         return saveNodeID;
      }

      bool getIsInlined() const
      {
         return isInlined;
      }

      void setIsInlined(bool isInlined)
      {
         this->isInlined = isInlined;
      }

      bool getReadable() const
      {
         return readable;
      }

      void setReadable(bool readable)
      {
         this->readable = readable;
      }

      bool operator<(const FsckFileInode& other)
      {
         if ( id < other.id )
            return true;
         else
            return false;
      }

      bool operator==(const FsckFileInode& other)
      {
         if ( id.compare(other.id) != 0 )
            return false;
         else
         if ( parentDirID.compare(other.parentDirID) != 0 )
            return false;
         else
         if ( parentNodeID != other.parentNodeID )
            return false;
         else
         if ( mode != other.mode )
            return false;
         else
         if ( userID != other.userID )
            return false;
         else
         if ( groupID != other.groupID )
            return false;
         else
         if ( fileSize != other.fileSize )
            return false;
         else
         if ( creationTime != other.creationTime )
            return false;
         else
         if ( modificationTime != other.modificationTime )
            return false;
         else
         if ( lastAccessTime != other.lastAccessTime )
            return false;
         else
         if ( numHardLinks != other.numHardLinks )
            return false;
         else
         if ( usedBlocks != other.usedBlocks)
            return false;
         else
         if ( stripeTargets != other.stripeTargets )
            return false;
         else
         if ( stripePatternType != other.stripePatternType )
            return false;
         else
         if ( chunkSize != other.chunkSize )
            return false;
         else
         if ( saveNodeID != other.saveNodeID )
            return false;
         else
         if ( isInlined != other.isInlined )
            return false;
         else
         if ( readable != other.readable )
            return false;
         else
            return true;
      }

      bool operator!= (const FsckFileInode& other)
      {
         return !(operator==( other ) );
      }

      void update(uint64_t internalID, std::string id, std::string parentDirID,
         uint16_t parentNodeID, PathInfo& pathInfo, int mode, unsigned userID, unsigned groupID,
         int64_t fileSize, uint64_t usedBlocks, int64_t creationTime, int64_t modificationTime, int64_t lastAccessTime,
         unsigned numHardLinks, UInt16Vector& stripeTargets,
         FsckStripePatternType stripePatternType, unsigned chunkSize, uint16_t saveNodeID,
         bool isInlined, bool readable)
      {
         this->internalID = internalID;

         this->id = id;
         this->parentDirID = parentDirID;
         this->parentNodeID = parentNodeID;

         this->pathInfo = pathInfo;

         this->mode = mode;
         this->userID = userID;
         this->groupID = groupID;

         this->fileSize = fileSize;
         this->creationTime = creationTime;
         this->modificationTime = modificationTime;
         this->lastAccessTime = lastAccessTime;
         this->numHardLinks = numHardLinks;

         this->usedBlocks = usedBlocks;

         this->stripeTargets = stripeTargets;
         this->stripePatternType = stripePatternType;
         this->chunkSize = chunkSize;

         this->saveNodeID = saveNodeID;
         this->isInlined = isInlined;
         this->readable = readable;
      }

      void print()
      {
         printf("internalID: %" PRId64 "\n", internalID);
         printf("id: %s\n", id.c_str());
         printf("parentDirID: %s\n", parentDirID.c_str());
         printf("parentNodeID: %hu\n", parentNodeID);
         printf("pathInfo.origParentEntryID: %s\n", pathInfo.getOrigParentEntryID().c_str());
         printf("pathInfo.origUID: %u\n", pathInfo.getOrigUID());
         printf("pathInfo.flags: %u\n", pathInfo.getFlags());
         printf("mode: %i\n", mode);
         printf("userID: %u\n", userID);
         printf("groupID: %u\n", groupID);
         printf("fileSize: %" PRId64 "\n", fileSize);
         printf("creationTime: %" PRId64 "\n", creationTime);
         printf("modificationTime: %" PRId64 "\n", modificationTime);
         printf("lastAccessTime: %" PRId64 "\n", lastAccessTime);
         printf("numHardLinks: %u\n", numHardLinks);
         printf("usedBlocks: %" PRIu64 "\n", usedBlocks);
         for (size_t i=0; i<stripeTargets.size(); i++)
         {
            printf("stripeTarget %zd: %hu\n", i, stripeTargets[i]);
         }
         printf("stripePatternType: %i\n", (int)stripePatternType);
         printf("chunkSize: %u\n", chunkSize);
         printf("saveNodeID: %hu\n", saveNodeID);
         printf("isInlined: %i\n", (int)isInlined);
         printf("readable: %i\n", (int)readable);
      }
};

#endif /* FSCKFILEINODE_H */

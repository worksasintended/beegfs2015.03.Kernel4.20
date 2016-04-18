/*
 * Information provided by stat()
 *
 * Remember to keep this class in sync with StatData.h of fhgfs-client_module!
 * However, there are a few differences to the fhgfs-client StatData as server side StatData also
 * need to handle disk serialization.
 */

#ifndef STATDATA_H_
#define STATDATA_H_

#include <common/storage/striping/ChunkFileInfo.h>
#include <common/storage/striping/StripePattern.h>
#include <common/storage/StorageDefinitions.h>
#include <common/toolkit/TimeAbs.h>
#include "ChunksBlocksVec.h"

#define STATDATA_FEATURE_SPARSE_FILE   1

// stat blocks are 512 bytes, ">> 9" is then the same as "/ 512", but faster
#define STATDATA_SIZETOBLOCKSBIT_SHIFT  9

class StatData;


class StatData
{
   public:
      StatData()
      {
         this->flags = 0;
      }

      StatData(int64_t fileSize, SettableFileAttribs* settableAttribs,
         int64_t createTime, int64_t attribChangeTime, unsigned nlink, unsigned contentsVersion)
      {
         this->flags = 0;

         this->fileSize             = fileSize;
         this->settableFileAttribs = *settableAttribs;
         this->creationTimeSecs     = createTime;
         this->attribChangeTimeSecs = attribChangeTime;
         this->nlink                = nlink;
         this->contentsVersion      = contentsVersion;
      }

      // used on file creation
      StatData(int mode, int64_t uid, int64_t gid, size_t numTargets) : chunkBlocksVec(numTargets)
      {
         int64_t nowSecs = TimeAbs().getTimeval()->tv_sec;

         this->flags                = 0;

         this->fileSize             = 0;
         this->creationTimeSecs     = nowSecs;
         this->attribChangeTimeSecs = nowSecs;
         this->nlink                = 1;
         this->contentsVersion      = 0;

         this->settableFileAttribs.mode    = mode;
         this->settableFileAttribs.userID  = uid;
         this->settableFileAttribs.groupID = gid;
         this->settableFileAttribs.modificationTimeSecs = nowSecs;
         this->settableFileAttribs.lastAccessTimeSecs   = nowSecs;

      }

   private:

      unsigned flags;

      int64_t fileSize;
      int64_t creationTimeSecs;     // real creation time
      int64_t attribChangeTimeSecs; // this corresponds to unix ctime
      unsigned nlink;

      SettableFileAttribs settableFileAttribs;

      unsigned contentsVersion; // inc'ed when file contents are modified (for cache invalidation)

      ChunksBlocksVec chunkBlocksVec; // serialized to disk, but not over network


   public:

      size_t serialize(bool isDiskDirInode, bool hasFlags, bool isNet, char* outBuf);
      bool deserialize(bool isDiskDirInode, bool hasFlags, bool isNet,
         const char* buf, size_t bufLen, unsigned* outLen);
      unsigned serialLen();

      void updateDynamicFileAttribs(ChunkFileInfoVec& fileInfoVec, StripePattern* stripePattern);


      // setters

      /**
       * Just used to init all values to something to mute compiler warnings, e.g. when we send
       * this struct in a message after a stat error and thus know that the receiver will not use
       * the values.
       */
      void setAllFake()
      {
         this->flags                = 0;

         this->fileSize             = 0;
         this->creationTimeSecs     = 0;
         this->attribChangeTimeSecs = 0;
         this->nlink                = 1;
         this->contentsVersion      = 0;

         this->settableFileAttribs.mode    = 0;
         this->settableFileAttribs.userID  = 0;
         this->settableFileAttribs.groupID = 0;
         this->settableFileAttribs.modificationTimeSecs = 0;
         this->settableFileAttribs.lastAccessTimeSecs   = 0;
      }

      /**
       * Only supposed to be used by the DirInode constructor
       */
      void setInitNewDirInode(SettableFileAttribs* settableAttribs, int64_t currentTime)
      {
         this->fileSize             = 0; // irrelevant for dirs, numSubdirs + numFiles on stat()
         this->creationTimeSecs     = currentTime;
         this->attribChangeTimeSecs = currentTime;
         this->nlink                = 2; // "." and ".." for now only

         this->settableFileAttribs = *settableAttribs;

         this->contentsVersion      = 0;
      }

      /**
       * Update statData with the new given data
       *
       * Note: We do not update everything (such as creationTimeSecs), but only values that
       *       really make sense to be updated at all.
       */
      void set(StatData* newStatData)
      {
         this->fileSize             = newStatData->fileSize;
         this->attribChangeTimeSecs = newStatData->attribChangeTimeSecs;
         this->nlink                = newStatData->nlink;

         this->settableFileAttribs  = newStatData->settableFileAttribs;

         this->contentsVersion      = newStatData->contentsVersion;

         this->flags                = newStatData->flags;
         this->chunkBlocksVec       = newStatData->chunkBlocksVec;
      }

      void setSparseFlag()
      {
         this->flags |= STATDATA_FEATURE_SPARSE_FILE;
      }

      void unsetSparseFlag()
      {
         this->flags &= ~STATDATA_FEATURE_SPARSE_FILE;
      }

      // getters
      void get(StatData& outStatData)
      {
         outStatData.fileSize             = this->fileSize;
         outStatData.creationTimeSecs     = this->creationTimeSecs;
         outStatData.attribChangeTimeSecs = this->attribChangeTimeSecs;
         outStatData.nlink                = this->nlink;

         outStatData.settableFileAttribs = this->settableFileAttribs;

         outStatData.contentsVersion      = this->contentsVersion;
      }

      void setAttribChangeTimeSecs(int64_t attribChangeTimeSecs)
      {
         this->attribChangeTimeSecs = attribChangeTimeSecs;
      }

      void setFileSize(int64_t fileSize)
      {
         this->fileSize = fileSize;
      }

      void setMode(int mode)
      {
         this->settableFileAttribs.mode = mode;
      }

      void setModificationTimeSecs(int64_t mtime)
      {
         this->settableFileAttribs.modificationTimeSecs = mtime;
      }

      void setLastAccessTimeSecs(int64_t atime)
      {
         this->settableFileAttribs.lastAccessTimeSecs = atime;
      }

      void setUserID(unsigned uid)
      {
         this->settableFileAttribs.userID = uid;
      }

      void setGroupID(unsigned gid)
      {
         this->settableFileAttribs.groupID = gid;
      }

      void setSettableFileAttribs(SettableFileAttribs newAttribs)
      {
         this->settableFileAttribs = newAttribs;
      }

      /**
       * Set the number of used blocks (of a chunk file) for the given target
       */
      void setTargetChunkBlocks(unsigned target, uint64_t chunkBlocks, size_t numStripeTargets)
      {
         this->chunkBlocksVec.setNumBlocks(target, chunkBlocks, numStripeTargets);
      }

      /**
       * Return the number of blocks (of a chunk file) for the given target
       */
      uint64_t getTargetChunkBlocks(unsigned target) const
      {
         return this->chunkBlocksVec.getNumBlocks(target);
      }

      /**
       * Return the number of used blocks. If the file is a sparse file we use values
       * from chunkBlocksVec. If the sparse-flag is not set we estimaete the number of blocks
       * from the file size.
       *
       * Note: Even if the file is a sparse file, some chunks might have the estimated number of
       *       blocks only, depending on if chunkBlocksVec was ever updated with exact values.
       */
      uint64_t getNumBlocks()
      {
         if (getIsSparseFile() )
            return this->chunkBlocksVec.getBlockSum();
         else
            return this->fileSize >> STATDATA_SIZETOBLOCKSBIT_SHIFT;
      }

      bool getIsSparseFile()
      {
         if (this->flags & STATDATA_FEATURE_SPARSE_FILE)
            return true;
         else
            return false;
      }

      void decNumHardLinks()
      {
         this->nlink--;
      }

      /**
       *  Increase or decrease the link count by the given value
       */
      void incDecNumHardLinks(int value)
      {
         this->nlink += value;
      }

      void setNumHardLinks(unsigned numHardLinks)
      {
         this->nlink = numHardLinks;
      }


      // getters
      int64_t getFileSize() const
      {
         return this->fileSize;
      }

      int getCreationTimeSecs() const
      {
         return this->creationTimeSecs;
      }

      int64_t getAttribChangeTimeSecs() const
      {
         return this->attribChangeTimeSecs;
      }

      int64_t getModificationTimeSecs() const
      {
         return this->settableFileAttribs.modificationTimeSecs;
      }

      int64_t getLastAccessTimeSecs() const
      {
         return this->settableFileAttribs.lastAccessTimeSecs;
      }

      unsigned getNumHardlinks() const
      {
         return this->nlink;
      }

      SettableFileAttribs* getSettableFileAttribs(void)
      {
         return &this->settableFileAttribs;
      }

      unsigned getMode() const
      {
         return this->settableFileAttribs.mode;
      }

      unsigned getUserID() const
      {
         return this->settableFileAttribs.userID;
      }

      unsigned getGroupID() const
      {
         return this->settableFileAttribs.groupID;
      }

};



#endif /* STATDATA_H_ */

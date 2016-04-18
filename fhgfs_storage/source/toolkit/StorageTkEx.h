#ifndef STORAGETKEX_H_
#define STORAGETKEX_H_

#include <app/config/Config.h>
#include <common/Common.h>
#include <common/fsck/FsckChunk.h>
#include <common/storage/Path.h>
#include <common/storage/Storagedata.h>
#include <common/storage/StorageErrors.h>
#include <common/threading/Mutex.h>
#include <common/threading/SafeMutexLock.h>
#include <common/toolkit/StorageTk.h>
#include <common/threading/SafeRWLock.h>

#include <dirent.h>


#define STORAGETK_FORMAT_MIN_VERSION        2
#define STORAGETK_FORMAT_CURRENT_VERSION    3
#define CHUNK_XATTR_ENTRYINFO_NAME          "user.fhgfs"
#define MAX_SERIAL_ENTRYINFO_SIZE           4096

#define STORAGETK_DEFAULTCHUNKDIRMODE   (S_IRWXU | S_IRWXG | S_IRWXO)
#define STORAGETK_DEFAULTCHUNKFILEMODE  (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

class StorageTkEx
{
   public:
      static bool attachEntryInfoToChunk(int fd, const char *serialEntryInfoBuf,
         unsigned serialEntryInfoBufLen, bool overwriteExisting = false);
      static bool attachEntryInfoToChunk(int targetFD, PathInfo* pathInfo, std::string chunkID,
         const char *serialEntryInfoBuf, unsigned serialEntryInfoBufLen,
         bool overwriteExisting = false);
      static bool readEntryInfoFromChunk(uint16_t targetID, PathInfo* pathInfo, std::string chunkID,
         EntryInfo* outEntryInfo);
   private:
      StorageTkEx() {}

      
   public:
      // inliners
      static bool getDynamicFileAttribs(const int fd, int64_t* outFilesize,
         int64_t* outAllocedBlocks, int64_t* outModificationTimeSecs,
         int64_t* outLastAccessTimeSecs)
      {
         struct stat statbuf;
         int statRes = fstat(fd, &statbuf);
         if (statRes)
            return false;

         *outFilesize = statbuf.st_size;
         *outAllocedBlocks = statbuf.st_blocks;
         *outModificationTimeSecs = statbuf.st_mtime;
         *outLastAccessTimeSecs = statbuf.st_atime;
         
         return true;
      }

      static bool getDynamicFileAttribs(const int dirFD, const char* path, int64_t* outFilesize,
         int64_t* outAllocedBlocks, int64_t* outModificationTimeSecs,
         int64_t* outLastAccessTimeSecs)
      {
         struct stat statbuf;
         int statRes = fstatat(dirFD, path, &statbuf, 0);
         if (statRes)
            return false;

         *outFilesize = statbuf.st_size;
         *outAllocedBlocks = statbuf.st_blocks;
         *outModificationTimeSecs = statbuf.st_mtime;
         *outLastAccessTimeSecs = statbuf.st_atime;
         
         return true;
      }

      /**
       * @return chunksPath/hashDir1/hashDir2
       */
      static std::string getChunksHashDir(const std::string chunksPath,
         unsigned firstLevelHashDirNum, unsigned secondLevelHashDirNum)
      {
         return chunksPath + "/" + StringTk::uintToHexStr(firstLevelHashDirNum) + "/"
            + StringTk::uintToHexStr(secondLevelHashDirNum);
      }
};

#endif /*STORAGETKEX_H_*/

#ifndef STORAGETKEX_H_
#define STORAGETKEX_H_

#include <app/config/Config.h>
#include <common/Common.h>
#include <common/storage/Path.h>
#include <common/threading/Mutex.h>
#include <common/threading/SafeMutexLock.h>
#include <common/toolkit/MetaStorageTk.h>
#include <common/toolkit/StorageTk.h>

#include <dirent.h>


/*
 * Note: Some inliners are in Commons MetaStoreTk::
 */

#define STORAGETK_FORMAT_MIN_VERSION        3
#define STORAGETK_FORMAT_CURRENT_VERSION    4

// forward declarations
class DirInode;
class FileInode;

class StorageTkEx
{
   public:
      static bool createStorageFormatFile(const std::string pathStr);
      static void checkStorageFormatFile(const std::string pathStr) throw (InvalidConfigException);

      static FhgfsOpsErr getContDirIDsIncremental(unsigned hashDirNum, int64_t lastOffset,
         unsigned maxOutEntries, StringList* outContDirIDs, int64_t* outNewOffset);
      static bool getNextContDirID(unsigned hashDirNum, int64_t lastOffset, std::string* outID,
         int64_t* outNewOffset);

      static DirEntryType readInodeFromFileUnlocked(std::string& metaFilename,
         FileInode** outFileInode, DirInode** outDirInode);
      static DirEntryType readInodeFromFileXAttrUnlocked(std::string& metaFilename,
         FileInode** outFileInode, DirInode** outDirInode);
      static DirEntryType readInodeFromFileContentsUnlocked(std::string& metaFilename,
         FileInode** outFileInode, DirInode** outDirInode);

   private:
      StorageTkEx()
      {
      }
      

   public:

      // inliners

      static std::string getMetaInodeHashDir(const std::string entriesPath,
         unsigned firstLevelhashDirNum, unsigned secondLevelhashDirNum)
      {
         return entriesPath + "/" + StringTk::uintToHexStr(firstLevelhashDirNum) + "/"
            + StringTk::uintToHexStr(secondLevelhashDirNum);
      }

      static std::string getMetaDentriesHashDir(const std::string structurePath,
         unsigned firstLevelhashDirNum, unsigned secondLevelhashDirNum)
      {
         return structurePath + "/" + StringTk::uintToHexStr(firstLevelhashDirNum) + "/"
            + StringTk::uintToHexStr(secondLevelhashDirNum);
      }
};

#endif /*STORAGETKEX_H_*/

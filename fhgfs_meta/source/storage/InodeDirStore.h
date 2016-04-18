#ifndef INODEDIRSTORE_H_
#define INODEDIRSTORE_H_

#include <common/Common.h>
#include <common/threading/Mutex.h>
#include <common/toolkit/AtomicObjectReferencer.h>
#include <common/toolkit/MetadataTk.h>
#include <common/toolkit/Random.h>
#include <common/storage/StorageDefinitions.h>
#include <common/storage/StorageErrors.h>

class DirInode;

typedef AtomicObjectReferencer<DirInode*> DirectoryReferencer;
typedef std::map<std::string, DirectoryReferencer*> DirectoryMap;
typedef DirectoryMap::iterator DirectoryMapIter;
typedef DirectoryMap::const_iterator DirectoryMapCIter;
typedef DirectoryMap::value_type DirectoryMapVal;

typedef std::map<std::string, DirectoryReferencer*> DirCacheMap; // keys are dirIDs (same as DirMap)
typedef DirCacheMap::iterator DirCacheMapIter;
typedef DirCacheMap::const_iterator DirCacheMapCIter;
typedef DirCacheMap::value_type DirCacheMapVal;

/**
 * Layer in between our inodes and the data on the underlying file system. So we read/write from/to
 * underlying files and this class is to do this corresponding data access.
 * This object is used for for _directories_ only.
 */
class InodeDirStore
{
   friend class DirInode;
   friend class MetaStore;
   
   public:
      InodeDirStore();

      ~InodeDirStore()
      {
         this->clearStoreUnlocked();

      };
      
      bool dirInodeInStoreUnlocked(std::string dirID);
      DirInode* referenceDirInode(std::string dirID, bool forceLoad);
      void releaseDir(std::string dirID);
      FhgfsOpsErr removeDirInode(std::string dirID);
      void removeAllDirs(bool ignoreEmptiness);

      size_t getSize();
      size_t getCacheSize();

      FhgfsOpsErr stat(std::string dirID, StatData& outStatData,
         uint16_t* outParentNodeID, std::string* outParentEntryID);
      FhgfsOpsErr setAttr(std::string dirID, int validAttribs, SettableFileAttribs* attribs);

      bool cacheSweepAsync();


   private:
      DirectoryMap dirs;

      size_t refCacheSyncLimit; // synchronous access limit (=> async limit plus some grace size)
      size_t refCacheAsyncLimit; // asynchronous cleanup limit (this is what the user configures)
      Random randGen; // for random cache removal
      DirCacheMap refCache;
      
      RWLock rwlock;

      void releaseDirUnlocked(std::string dirID);

      FhgfsOpsErr makeDirInode(DirInode* dir);
      FhgfsOpsErr makeDirInode(DirInode* dir, const CharVector& defaultACLXAttr,
         const CharVector& accessACLXAttr);
      FhgfsOpsErr makeDirInodeUnlocked(DirInode* dir);
      FhgfsOpsErr makeDirInodeUnlocked(DirInode* dir, const CharVector& defaultACLXAttr,
         const CharVector& accessACLXAttr);
      FhgfsOpsErr isRemovableUnlocked(std::string dirID, uint16_t* outMirrorNodeID);
      FhgfsOpsErr removeDirInodeUnlocked(std::string dirID, DirInode** outRemovedDir);
      
      bool InsertDirInodeUnlocked(std::string id, DirectoryMapIter& newElemIter, bool forceLoad);
      
      FhgfsOpsErr setDirParent(EntryInfo* entryInfo, uint16_t parentNodeID);

      void clearStoreUnlocked();

      void cacheAddUnlocked(std::string& dirID, DirectoryReferencer* dirRefer);
      void cacheRemoveUnlocked(std::string& dirID);
      void cacheRemoveAllUnlocked();
      bool cacheSweepUnlocked(bool isSyncSweep);
};

#endif /*INODEDIRSTORE_H_*/

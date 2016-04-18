#include <common/threading/SafeMutexLock.h>
#include <program/Program.h>
#include <storage/PosixACL.h>
#include "DirInode.h"
#include "InodeDirStore.h"


#define DIRSTORE_REFCACHE_REMOVE_SKIP_SYNC     (4) /* n-th elements to be removed on sync sweep */
#define DIRSTORE_REFCACHE_REMOVE_SKIP_ASYNC    (3) /* n-th elements to be removed on async sweep */

/**
  * not inlined as we need to include <program/Program.h>
  */
InodeDirStore::InodeDirStore()
{
   Config* cfg = Program::getApp()->getConfig();

   this->refCacheSyncLimit = cfg->getTuneDirMetadataCacheLimit();
   this->refCacheAsyncLimit = refCacheSyncLimit - (refCacheSyncLimit/2);
}

/**
 * @param dir belongs to the store after calling this method - so do not free it and don't
 * use it any more afterwards (re-get it from this store if you need it)
 */
FhgfsOpsErr InodeDirStore::makeDirInode(DirInode* dir)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE); // L O C K

   FhgfsOpsErr mkRes = makeDirInodeUnlocked(dir);

   safeLock.unlock(); // U N L O C K

   return mkRes;
}

/**
 * @param dir belongs to the store after calling this method - so do not free it and don't
 * use it any more afterwards (re-get it from this store if you need it)
 * @param defaultACLXAttr will be set as the posix_default_acl extended attribute
 * @param accessACLXAttr will be set as the access_default_acl extended attribute
 */
FhgfsOpsErr InodeDirStore::makeDirInode(DirInode* dir,
   const CharVector& defaultACLXAttr, const CharVector& accessACLXAttr)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE); // L O C K

   const std::string dirID(dir->getID() );

   FhgfsOpsErr mkRes = makeDirInodeUnlocked(dir, defaultACLXAttr, accessACLXAttr);

   safeLock.unlock(); // U N L O C K

   return mkRes;
}

/**
 * @param dir belongs to the store after calling this method - so do not free it and don't
 * use it any more afterwards (re-get it from this store if you need it)
 */
FhgfsOpsErr InodeDirStore::makeDirInodeUnlocked(DirInode* dir, const CharVector& defaultACLXAttr,
         const CharVector& accessACLXAttr)
{
   FhgfsOpsErr retVal = dir->storePersistentMetaData(defaultACLXAttr, accessACLXAttr);

   delete(dir);

   return retVal;
}

/**
 * @param dir belongs to the store after calling this method - so do not free it and don't
 * use it any more afterwards (re-get it from this store if you need it)
 */
FhgfsOpsErr InodeDirStore::makeDirInodeUnlocked(DirInode* dir)
{
   FhgfsOpsErr retVal = dir->storePersistentMetaData();

   delete(dir);

   return retVal;
}

bool InodeDirStore::dirInodeInStoreUnlocked(std::string dirID)
{
   DirectoryMapIter iter = this->dirs.find(dirID);
   if(iter != this->dirs.end() )
      return true;

   return false;
}


/**
 * Note: remember to call releaseDir()
 * 
 * @param forceLoad  Force to load the DirInode from disk. Defaults to false.
 * @return NULL if no such dir exists (or if the dir is being moved), but cannot be NULL if
 *    forceLoad is false
 */
DirInode* InodeDirStore::referenceDirInode(std::string dirID, bool forceLoad)
{
   const char* logContext = "InodeDirStore referenceDirInode";
   DirInode* dir = NULL;
   bool wasReferenced = true; /* Only try to add to cache if not in memory yet.
                               * Any attempt to add it to the cache causes a cache sweep, which is
                               * rather expensive.
                               * Note: when set to false we also need a write-lock! */
   
   SafeRWLock safeLock(&this->rwlock, SafeRWLock_READ); // L O C K

   DirectoryMapIter iter;
   int retries = 0; // 0 -> read-locked
   while (retries < RWLOCK_LOCK_UPGRADE_RACY_RETRIES) // one as read-lock and one as write-lock
   {
      iter = this->dirs.find(dirID);
      if (iter == this->dirs.end() && retries == 0)
      {
         safeLock.unlock();
         safeLock.lock(SafeRWLock_WRITE);
      }
      retries++;
   }

   if(iter == this->dirs.end() )
   { // Not in map yet => try to load it. We must be write-locked here!
      InsertDirInodeUnlocked(dirID, iter, forceLoad); // (will set "iter != end" if loaded)
      wasReferenced = false;
   }

   if(iter != dirs.end() )
   { // exists in map
      DirectoryReferencer* dirRefer = iter->second;
      DirInode* dirNonRef = dirRefer->getReferencedObject();
      
      if(!dirNonRef->getExclusive() ) // check moving
      {
         dir = dirRefer->reference();
         LOG_DEBUG(logContext, Log_SPAM,  std::string("DirID: ") + dir->getID() +
            " Refcount: " + StringTk::intToStr(dirRefer->getRefCount() ) );
         IGNORE_UNUSED_VARIABLE(logContext);

         if (wasReferenced == false)
            cacheAddUnlocked(dirID, dirRefer);

      }
      else
      {
         // note: we don't need to check unload here, because exclusive means there already is a
         //       reference so we definitely didn't load here
      }
   }

   safeLock.unlock(); // U N L O C K

   /* Only try to load the DirInode after giving up the lock. DirInodes are usually referenced
    * without being loaded at all from the kernel client, so we can afford the extra lock if loading
    * the DirInode fails. */
   if (forceLoad && dir && (dir->loadIfNotLoaded() == false) )
   { // loading from disk failed, release the dir again
      releaseDir(dirID);
      dir = NULL;
   }

   return dir;
}

/**
 * Release reduce the refcounter of an DirInode here
 */
void InodeDirStore::releaseDir(std::string dirID)
{
   SafeRWLock safeLock(&this->rwlock, SafeRWLock_WRITE); // L O C K

   releaseDirUnlocked(dirID);

   safeLock.unlock(); // U N L O C K
}

void InodeDirStore::releaseDirUnlocked(std::string dirID)
{
   const char* logContext = "InodeDirStore releaseDirInode";

   App* app = Program::getApp();

   DirectoryMapIter iter = this->dirs.find(dirID);
   if(likely(iter != this->dirs.end() ) )
   { // dir exists => decrease refCount
      DirectoryReferencer* dirRefer = iter->second;

      if (likely(dirRefer->getRefCount() ) )
      {
         dirRefer->release();
         DirInode* dirNonRef = dirRefer->getReferencedObject();
  
         LOG_DEBUG(logContext, Log_SPAM,  std::string("DirID: ") + dirID +
            " Refcount: " + StringTk::intToStr(dirRefer->getRefCount() ) );

         if(!dirRefer->getRefCount() )
         { // dropped last reference => unload dir

            if (unlikely( dirNonRef->fileStore.getSize() && !app->getSelfTerminate() ) )
            {
               std::string logMsg = "Bug: Refusing to release the directory, "
                  "its fileStore still has references!";
               LogContext(logContext).logErr(logMsg + std::string(" dirID: ") + dirID);

               dirRefer->reference();
            }
            else
            { // as expected, fileStore is empty
               delete(dirRefer);
               this->dirs.erase(iter);
            }
         }
      }
      else
      { // attempt to release a Dir without a refCount
         std::string logMsg = std::string("Bug: Refusing to release dir with a zero refCount") +
            std::string("dirID: ") + dirID;
         LogContext(logContext).logErr(logMsg);
         this->dirs.erase(iter);
      }
   }
   else
   {
      LogContext(logContext).logErr("Bug: releaseDir requested, but dir not referenced! "
         "DirID: " + dirID);
      LogContext(logContext).logBacktrace();
   }
}

FhgfsOpsErr InodeDirStore::removeDirInode(std::string dirID)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE); // L O C K

   FhgfsOpsErr delErr = removeDirInodeUnlocked(dirID, NULL);

   safeLock.unlock(); // U N L O C K

   return delErr;
}

/**
 * Check whether the dir is removable (i.e. not in use).
 *
 * Note: Caller must make sure that there is no cache reference that would lead to a false in-use
 * result.
 *
 * @param outMirrorNodeID if this returns success and the dir is mirrored, this param will be set to
 * the mirror of this dir, i.e. !=0; (note: mirroring has actually nothing to do with removable
 * check, but we have it here because this method already loads the dir inode, so that we can avoid
 * another inode load in removeDirInodeUnlocked() for mirror checking.)
 */
FhgfsOpsErr InodeDirStore::isRemovableUnlocked(std::string dirID, uint16_t* outMirrorNodeID)
{
   const char* logContext = "InodeDirStore check if dir is removable";
   DirectoryMapCIter iter = dirs.find(dirID);

   *outMirrorNodeID = 0;

   if(iter != dirs.end() )
   { // dir currently loaded, refuse to let it rmdir'ed
      DirectoryReferencer* dirRefer = iter->second;
      DirInode* dir = dirRefer->getReferencedObject();

      if (unlikely(!dirRefer->getRefCount() ) )
      {
         LogContext(logContext).logErr("Bug: unreferenced dir found! EntryID: " + dirID);

         return FhgfsOpsErr_INTERNAL;
      }

      dir->loadIfNotLoaded();

      if(dir->getNumSubEntries() )
         return FhgfsOpsErr_NOTEMPTY;

      if(dir->getExclusive() )
         return FhgfsOpsErr_INUSE;

      /* "if (dirRefer->getRefCount() )" is implicit, another thread has still referenced it
       * TODO: disallow new file/subdir creates for this dirInode, add the dirInode to the
       *       disposable list and remove the dirEntry. Then delete this dirInode as soon as
       *       the other thread releases it. */

      return FhgfsOpsErr_INUSE;
   }
   else
   { // not loaded => static checking
      DirInode dirInode(dirID);

      if (!dirInode.loadFromFile() )
         return FhgfsOpsErr_PATHNOTEXISTS;

      if(dirInode.getNumSubEntries() )
         return FhgfsOpsErr_NOTEMPTY;

      // unlink allowed => assign outMirrorNodeID
      if(dirInode.getFeatureFlags() & DIRINODE_FEATURE_MIRRORED)
         *outMirrorNodeID = dirInode.getMirrorNodeID();
   }

   return FhgfsOpsErr_SUCCESS;
}


/**
 * Note: This method does not lock the mutex, so it must already be locked when calling this
 * 
 * @param outRemovedDir will be set to the removed dir which must then be deleted by the caller
 * (can be NULL if the caller is not interested in the dir)
 */
FhgfsOpsErr InodeDirStore::removeDirInodeUnlocked(std::string dirID, DirInode** outRemovedDir)
{
   if(outRemovedDir)
      *outRemovedDir = NULL;
   
   cacheRemoveUnlocked(dirID); /* we should move this after isRemovable()-check as soon as we can
      remove referenced dirs */

   uint16_t mirrorNodeID; // will be set if dir mirrored and isRemovableUnlocked() returns success

   FhgfsOpsErr removableRes = isRemovableUnlocked(dirID, &mirrorNodeID);
   if(removableRes != FhgfsOpsErr_SUCCESS)
      return removableRes;

   if(outRemovedDir)
   { // instantiate outDir
      *outRemovedDir = DirInode::createFromFile(dirID);
      if(!(*outRemovedDir) )
         return FhgfsOpsErr_INTERNAL;
   }

   bool persistenceOK = DirInode::unlinkStoredInode(dirID);
   if(!persistenceOK)
   {
      if(outRemovedDir)
      {
         delete(*outRemovedDir);
         *outRemovedDir = NULL;
      }

      return FhgfsOpsErr_INTERNAL;
   }

   // add entry to mirror queue
   if(mirrorNodeID)
      MetadataMirrorer::removeInodeStatic(dirID, true, mirrorNodeID);

   return FhgfsOpsErr_SUCCESS;
}

/**
 * Note: This does not load any entries, so it will only return the number of already loaded
 * entries. (Only useful for debugging and statistics probably.)
 */
size_t InodeDirStore::getSize()
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K
   
   size_t dirsSize = dirs.size();

   safeLock.unlock(); // U N L O C K

   return dirsSize;
}

/**
 * @param outParentNodeID may be NULL
 * @param outParentEntryID may be NULL (if outParentNodeID is NULL)
 */
FhgfsOpsErr InodeDirStore::stat(std::string dirID, StatData& outStatData,
   uint16_t* outParentNodeID, std::string* outParentEntryID)
{
   const char* logContext = "InodeDirStore (stat)";

   FhgfsOpsErr statRes = FhgfsOpsErr_PATHNOTEXISTS;

   uint16_t localNodeID = Program::getApp()->getLocalNode()->getNumID();
   
   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K
   
   DirectoryMapIter iter = dirs.find(dirID);
   if(iter != dirs.end() )
   { // dir loaded
      DirectoryReferencer* dirRefer = iter->second;
      DirInode* dir = dirRefer->getReferencedObject();

      bool loadRes = dir->loadIfNotLoaded(); // might not be loaded at all...
      if (!loadRes)
      {
         goto outErr; /* Loading from disk failed, the dir is only referenced, but does not exist
                       * Might be due to our dir-reference optimization or a corruption problem */
      }

      if(localNodeID != dir->getOwnerNodeID() )
      {
         /* check for owner node ID is especially important for root dir
          * NOTE: this check is only performed, if directory is already loaded, because all MDSs
          * need to be able to reference the root directory even if owner is not set (it is not set
          * on first startup). */
         LogContext(logContext).log(Log_WARNING, "Owner verification failed. EntryID: " + dirID);
         statRes = FhgfsOpsErr_NOTOWNER;
      }
      else
      {
         statRes = dir->getStatData(outStatData, outParentNodeID, outParentEntryID);
      }

      safeLock.unlock();
   }
   else
   {
      safeLock.unlock(); /* Unlock first, no need to hold a lock to just read the file from disk.
                          * If xattr or data are not writte yet we just raced and return
                          * FhgfsOpsErr_PATHNOTEXISTS */

      // read data on disk
      statRes = DirInode::getStatData(dirID, outStatData, outParentNodeID, outParentEntryID);
   }

   return statRes;

outErr:
   safeLock.unlock(); // U N L O C K
   return statRes;
}

/**
 * @param validAttribs SETATTR_CHANGE_...-Flags
 */
FhgfsOpsErr InodeDirStore::setAttr(std::string dirID, int validAttribs,
   SettableFileAttribs* attribs)
{
   FhgfsOpsErr retVal = FhgfsOpsErr_PATHNOTEXISTS;
   
   SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE); // L O C K
   
   DirectoryMapIter iter = dirs.find(dirID);
   if(iter == dirs.end() )
   { // not loaded => load, apply, destroy
      DirInode dir(dirID);

      if(dir.loadFromFile() )
      { // loaded
         bool setRes = dir.setAttrData(validAttribs, attribs);

         retVal = setRes ? FhgfsOpsErr_SUCCESS : FhgfsOpsErr_INTERNAL;
      }
   }
   else
   { // dir loaded
      DirectoryReferencer* dirRefer = iter->second;
      DirInode* dir = dirRefer->getReferencedObject();
      
      if(!dir->getExclusive() )
      {
         bool setRes = dir->setAttrData(validAttribs, attribs);

         retVal = setRes ? FhgfsOpsErr_SUCCESS : FhgfsOpsErr_INTERNAL;
      }
   }

   safeLock.unlock(); // U N L O C K
   
   return retVal;
}

/**
 * Creates and empty DirInode and inserts it into the map.
 *
 * Note: We only need to hold a read-lock here, as we check if inserting an entry into the map
 *       succeeded.
 *
 * @return newElemIter only valid if true is returned, untouched otherwise
 */
bool InodeDirStore::InsertDirInodeUnlocked(std::string dirID, DirectoryMapIter& newElemIter,
   bool forceLoad)
{
   bool retVal = false;

   DirInode* inode = new DirInode(dirID);
   if (unlikely (!inode) )
      return retVal; // out of memory

   if (forceLoad)
   { // load from disk requested
      if (!inode->loadIfNotLoaded() )
      {
         delete inode;
         return false;
      }
   }

   std::pair<DirectoryMapIter, bool> pairRes =
      this->dirs.insert(DirectoryMapVal(dirID, new DirectoryReferencer(inode) ) );

   if (pairRes.second == false)
   {
      // element already exists in the map, we raced with another thread
      delete inode;

      newElemIter = this->dirs.find(dirID);
      if (likely (newElemIter != this->dirs.end() ) )
         retVal = true;
   }
   else
   {
      newElemIter  = pairRes.first;
      retVal = true;
   }

   return retVal;
}

void InodeDirStore::clearStoreUnlocked()
{
   LOG_DEBUG("DirectoryStore::clearStoreUnlocked", Log_DEBUG,
      std::string("# of loaded entries to be cleared: ") + StringTk::intToStr(dirs.size() ) );

   cacheRemoveAllUnlocked();

   for(DirectoryMapIter iter = dirs.begin(); iter != dirs.end(); iter++)
   {
      DirectoryReferencer* dirRef = iter->second;

      // will also call destructor for dirInode and sub-objects as dirInode->fileStore
      delete(dirRef);
   }
   
   dirs.clear();
}

/**
 * Note: Make sure to call this only after the new reference has been taken by the caller
 * (otherwise it might happen that the new element is deleted during sweep if it was cached
 * before and appears to be unneeded now).
 */
void InodeDirStore::cacheAddUnlocked(std::string& dirID, DirectoryReferencer* dirRefer)
{
   const char* logContext = "InodeDirStore cache add DirInode";

   Config* cfg = Program::getApp()->getConfig();

   unsigned cacheLimit = cfg->getTuneDirMetadataCacheLimit();

   if (unlikely(cacheLimit == 0) )
      return; // cache disabled by user config

   // (we do cache sweeping before insertion to make sure we don't sweep the new entry)
   cacheSweepUnlocked(true);

   if(refCache.insert(DirCacheMapVal(dirID, dirRefer) ).second)
   { // new insert => inc refcount
      dirRefer->reference();

      LOG_DEBUG(logContext, Log_SPAM,  std::string("DirID: ") + dirID +
         " Refcount: " + StringTk::intToStr(dirRefer->getRefCount() ) );
      IGNORE_UNUSED_VARIABLE(logContext);
   }

}

void InodeDirStore::cacheRemoveUnlocked(std::string& dirID)
{
   DirCacheMapIter iter = refCache.find(dirID);
   if(iter == refCache.end() )
      return;

   releaseDirUnlocked(dirID);
   refCache.erase(iter);
}

void InodeDirStore::cacheRemoveAllUnlocked()
{
   for(DirCacheMapIter iter = refCache.begin(); iter != refCache.end(); /* iter inc inside loop */)
   {
      releaseDirUnlocked(iter->first);

      DirCacheMapIter iterNext(iter);
      iterNext++;

      // cppcheck-suppress erase [special comment to mute false cppcheck alarm]
      refCache.erase(iter);

      iter = iterNext;
   }
}

/**
 * @param isSyncSweep true if this is a synchronous sweep (e.g. we need to free a few elements to
 * allow quick insertion of a new element), false is this is an asynchronous sweep (that might take
 * a bit longer).
 * @return true if a cache flush was triggered, false otherwise
 */
bool InodeDirStore::cacheSweepUnlocked(bool isSyncSweep)
{
   // sweeping means we remove every n-th element from the cache, starting with a random element
      // in the range 0 to n
   size_t cacheLimit;
   size_t removeSkipNum;

   // check type of sweep and set removal parameters accordingly

   if(isSyncSweep)
   { // sync sweep settings
      cacheLimit = refCacheSyncLimit;
      removeSkipNum = DIRSTORE_REFCACHE_REMOVE_SKIP_SYNC;
   }
   else
   { // async sweep settings
      cacheLimit = refCacheAsyncLimit;
      removeSkipNum = DIRSTORE_REFCACHE_REMOVE_SKIP_ASYNC;
   }

   if(refCache.size() <= cacheLimit)
      return false;


   // pick a random start element (note: the element will be removed in first loop pass below)

   unsigned randStart = randGen.getNextInRange(0, removeSkipNum - 1);
   DirCacheMapIter iter = refCache.begin();

   while(randStart--)
      iter++;


   // walk over all cached elements and remove every n-th element

   unsigned i = removeSkipNum-1; /* counts to every n-th element ("remoteSkipNum-1" to remove the
      random start element in the first loop pass) */

   while(iter != refCache.end() )
   {
      i++;

      if(i == removeSkipNum)
      {
         releaseDirUnlocked(iter->first);

         DirCacheMapIter iterNext(iter);
         iterNext++;

         refCache.erase(iter);

         iter = iterNext;

         i = 0;
      }
      else
         iter++;
   }

   return true;
}

/**
 * @return true if a cache flush was triggered, false otherwise
 */
bool InodeDirStore::cacheSweepAsync()
{
   const char* logContext = "Async cache sweep";

   //LOG_DEBUG(logContext, Log_SPAM, "Start cache sweep."); // debug in
   IGNORE_UNUSED_VARIABLE(logContext);

   SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE); // L O C K

   bool retVal = cacheSweepUnlocked(false);

   safeLock.unlock(); // U N L O C K

   // LOG_DEBUG(logContext, Log_SPAM, "Stop cache sweep."); // debug in

   return retVal;
}

/**
 * @return current number of cached entries
 */
size_t InodeDirStore::getCacheSize()
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   size_t dirsSize = refCache.size();

   safeLock.unlock(); // U N L O C K

   return dirsSize;
}

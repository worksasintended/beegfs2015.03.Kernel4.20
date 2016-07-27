#include <common/storage/striping/Raid0Pattern.h>
#include <common/storage/Storagedata.h>
#include <common/toolkit/FsckTk.h>
#include <net/msghelpers/MsgHelperStat.h>
#include <net/msghelpers/MsgHelperMkFile.h>
#include <net/msghelpers/MsgHelperXAttr.h>
#include <storage/PosixACL.h>
#include <program/Program.h>
#include "MetaStore.h"

#include <attr/xattr.h>
#include <dirent.h>
#include <sys/types.h>

#define MAX_DEBUG_LOCK_TRY_TIME 30 // max lock wait time in seconds

/**
 * Reference the given directory.
 *
 * @param dirID      the ID of the directory to reference
 * @param forceLoad  not all operations require the directory to be loaded from disk (i.e. stat()
 *                   of a sub-entry) and the DirInode is only used for directory locking. Other
 *                   operations need the directory to locked and those shall set forceLoad = true.
 *                   Should be set to true if we are going to write to the DirInode anyway or
 *                   if DirInode information are required.
 * @return           cannot be NULL if forceLoad=false, even if the directory with the given id
 *                   does not exist. But it can be NULL of forceLoad=true, as we only then
 *                   check if the directory exists on disk at all.
 */
DirInode* MetaStore::referenceDir(const std::string dirID, const bool forceLoad)
{
   DirInode* dirInode;

   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   dirInode = referenceDirUnlocked(dirID, forceLoad);

   safeLock.unlock(); // U N L O C K

   return dirInode;
}

DirInode* MetaStore::referenceDirUnlocked(std::string dirID, bool forceLoad)
{
   return dirStore.referenceDirInode(dirID, forceLoad);
}

/**
 * Reference the directory inode and lock it. We don't advise to load the dirInode from disk here,
 * so with minor exceptions (e.g. out of memory) we will always get the dirInode.
 * Also aquire a read-lock for the directory.
 */
DirInode* MetaStore::referenceAndReadLockDirUnlocked(std::string dirID, bool forceLoad)
{
   DirInode *dirInode = referenceDirUnlocked(dirID, forceLoad);
   if (likely(dirInode))
      dirInode->rwlock.readLock();

   return dirInode;
}

/**
 * Similar to referenceAndReadLockDirUnlocked(), but write-lock here.
 */
DirInode* MetaStore::referenceAndWriteLockDirUnlocked(std::string dirID)
{
   DirInode *dirInode = referenceDirUnlocked(dirID, false);
   if (likely(dirInode))
      dirInode->rwlock.writeLock();

   return dirInode;
}

void MetaStore::releaseDir(std::string dirID)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   releaseDirUnlocked(dirID);

   safeLock.unlock(); // U N L O C K
}

void MetaStore::releaseDirUnlocked(std::string dirID)
{
   dirStore.releaseDir(dirID);
}

/**
 * Unlock a release a directory reference.
 */
void MetaStore::unlockAndReleaseDirUnlocked(std::string dirID, DirInode* dir)
{

   /*
    * Debug the reference / release code. As we don't keep the lock order here
    * (1st DirStore and then DirInode, but DirInode is already locked here), we need to be very
    * careful and only do that in debug mode. */
   #ifdef BEEGFS_DEBUG_RELEASE_DIR
      SafeRWLock safeLock(&dirStore.rwlock);

      const char* logContext = "MetaStore Unlock and Release Dir";
      bool lockRes;

      lockRes = safeLock.timedReadLock(MAX_DEBUG_LOCK_TRY_TIME);
      if (lockRes)
      {
         /* Aquring the lock failed, we assume we would deadlock, but that also means the DirInode
          * is referenced */
         dir->rwlock.unlock();
         releaseDirUnlocked(dir->getID() );
      }
      else
      {
         bool isInStore = dirStore.dirInodeInStoreUnlocked(dirID);
         safeLock.unlock(); // release as early as possible

         if (isInStore)
         {
            dir->rwlock.unlock();
            releaseDirUnlocked(dirID);
         }
         else
         {
            LogContext(logContext).logErr(
               "Bug: Trying to release unreferenced DirInode: " + dirID);
            LogContext(logContext).logBacktrace();

            /* We are still going to unlock it here as we have the object. But it might not be valid
             * anymore. But then by not unlocking it we might keep a lock and deadlock later on. */
            dir->rwlock.unlock();
         }
      }

   #else // non-debug mode
      dir->rwlock.unlock();
      releaseDirUnlocked(dir->getID() );
   #endif

}

/**
 * Only unlock a directory here.
 */
void MetaStore::unlockDirUnlocked(DirInode* dir)
{
   dir->rwlock.unlock();
}


/**
 * Reference a file. It is unknown if this file is already referenced in memory or needs to be
 * loaded. Therefore a complete entryInfo is required.
 */
FileInode* MetaStore::referenceFile(EntryInfo* entryInfo)
{
   FileInode* retVal;

   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   retVal = referenceFileUnlocked(entryInfo);

   safeLock.unlock(); // U N L O C K

   return retVal;
}

/**
 * See referenceFileInode() for details. We already have the lock here.
 */
FileInode* MetaStore::referenceFileUnlocked(EntryInfo* entryInfo)
{
   FileInode *inode = this->fileStore.referenceFileInode(entryInfo, false);
   if (!inode)
   {
      // not in global map, now per directory and also try to load from disk

      std::string parentEntryID = entryInfo->getParentEntryID();
      DirInode* subDir = referenceAndReadLockDirUnlocked(parentEntryID);
      if (likely(subDir) )
      {
         inode = subDir->fileStore.referenceFileInode(entryInfo, true);

         if (!inode)
            unlockAndReleaseDirUnlocked(entryInfo->getParentEntryID(), subDir);
         else
         {
            // we are not going to release the directory here, as we are using its FileStore
            inode->incParentRef(parentEntryID);
            unlockDirUnlocked(subDir);
         }
      }
   }

   return inode;
}

/**
 * See referenceFileInode() for details. DirInode is already known here and will not be futher
 * referenced due to the write-lock hold by the caller. Getting further references might cause
 * dead locks due to locking order problems (DirStore needs to be locked while holding the DirInode
 * lock).
 *
 * Locking:
 *    MetaStore read locked
 *    DirInode write locked.
 *
 * Note: Callers must release FileInode before releasing DirInode! Use this method with care!
 *
 */
FileInode* MetaStore::referenceFileUnlocked(DirInode* subDir, EntryInfo* entryInfo)
{
   FileInode *inode = this->fileStore.referenceFileInode(entryInfo, false);
   if (!inode)
   {
      // not in global map, now per directory and also try to load from disk
      inode = subDir->fileStore.referenceFileInode(entryInfo, true);
   }

   return inode;
}

/**
 * Reference a file known to be already referenced. So disk-access is not required and we don't
 * need the complete EntryInfo, but parentEntryID and entryID are sufficient.
 * Another name for this function also could be "referenceReferencedFiles".
 */
FileInode* MetaStore::referenceLoadedFile(std::string parentEntryID, std::string entryID)
{
   FileInode* retVal;

   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   retVal = referenceLoadedFileUnlocked(parentEntryID, entryID);

   safeLock.unlock(); // U N L O C K

   return retVal;
}

/**
 * See referenceLoadedFile() for details. We already have the lock here.
 */
FileInode* MetaStore::referenceLoadedFileUnlocked(std::string parentEntryID, std::string entryID)
{
   FileInode *inode = this->fileStore.referenceLoadedFile(entryID);
   if (!inode)
   {
      // not in global map, now per directory and also try to load from disk
      DirInode* subDir = referenceAndReadLockDirUnlocked(parentEntryID);
      if (likely(subDir) )
      {
         inode = subDir->fileStore.referenceLoadedFile(entryID);

         if (!inode)
            unlockAndReleaseDirUnlocked(parentEntryID, subDir);
         else
         {
            // we are not going to release the directory here, as we are using its FileStore
            inode->incParentRef(parentEntryID);
            unlockDirUnlocked(subDir);
         }
      }
   }

   return inode;
}

/**
 * See referenceFileInode() for details. DirInode is already known here and will not be futher
 * referenced due to the write-lock hold by the caller. Getting further references might cause
 * dead locks due to locking order problems (DirStore needs to be locked while holding the DirInode
 * lock).
 *
 * Locking:
 *    MetaStore read locked
 *    DirInode write locked.
 *
 * Note: Callers must release FileInode before releasing DirInode! Use this method with care!
 *
 */
FileInode* MetaStore::referenceLoadedFileUnlocked(DirInode* subDir, std::string entryID)
{
   FileInode *inode = this->fileStore.referenceLoadedFile(entryID);
   if (!inode)
   {
      // not in global map, now per directory and also try to load from disk
      inode = subDir->fileStore.referenceLoadedFile(entryID);
   }

   return inode;
}

bool MetaStore::releaseFile(std::string parentEntryID, FileInode* inode)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   bool releaseRes = releaseFileUnlocked(parentEntryID, inode);

   safeLock.unlock(); // U N L O C K

   return releaseRes;
}

/**
 * Release a file inode.
 *
 * Note: If the inode belongs to the per-directory file store also this directory will be
 *       released.
 */
bool MetaStore::releaseFileUnlocked(std::string parentEntryID, FileInode* inode)
{
   bool releaseRes = fileStore.releaseFileInode(inode);
   if (!releaseRes)
   { // Not in global map, now per directory and also try to load from disk.
      DirInode* subDir = referenceAndReadLockDirUnlocked(parentEntryID );
      if (likely(subDir) )
      {
         inode->decParentRef();
         releaseRes = subDir->fileStore.releaseFileInode(inode);

         unlockAndReleaseDirUnlocked(parentEntryID, subDir); // this is the current lock and reference

         // when we referenced the inode we did not release the directory yet, so do that here
         releaseDirUnlocked(parentEntryID);
      }
   }

   return releaseRes;
}

/**
 * Release a file inode. DirInode is known by the caller. Only call this after using
 * referenceFileUnlocked(DirInode* subDir, EntryInfo* entryInfo)
 * or
 * referenceLoadedFileUnlocked(DirInode* subDir, std::string entryID)
 * to reference a FileInode.
 *
 * Locking:
 *    MetaStore is locked
 *    DirInode  is locked
 *
 * Note: No extra DirInode releases here, as the above mentioned referenceFileUnlocked() method also
 *       does not get additional references.
 */
bool MetaStore::releaseFileUnlocked(DirInode* subDir, FileInode* inode)
{
   bool releaseRes = fileStore.releaseFileInode(inode);
   if (!releaseRes)
   { // Not in global map, now per directory.
      releaseRes = subDir->fileStore.releaseFileInode(inode);
   }

   return releaseRes;
}


/*
 * Can be used to reference an inode, if it is unknown whether it is a file or directory. While
 * one of the out.... pointers will hold a reference to the corresponidng inode (which must be
 * released), the other pointer will be set to NULL.
 *
 * Note: This funtion is mainly used by fsck and the fullRefresher component.
 * Note: dirInodes will always be loaded from disk with this function
 * Note: This will NOT work with inlined file inodes, as we only pass the ID
 * Note: This is not very efficient, but OK for now. Can definitely be optimized.
 *
 * @param entryID
 * @param outDirInode
 * @param outFilenode
 *
 * @return false if entryID could neither be loaded as dir nor as file inode
 */
bool MetaStore::referenceInode(std::string entryID, FileInode** outFileInode,
   DirInode** outDirInode)
{
   *outFileInode = NULL;
   *outDirInode  = NULL;

   // trying dir first, because we assume there are more non-inlined dir inodes than file inodes
   *outDirInode = referenceDir(entryID, true);

   if (*outDirInode)
      return true;

   // opening as dir failed => try as file

   /* we do not set full entryInfo (we do not have most of the info), but only entryID. That's why
      it does not work with inlined inodes */
   EntryInfo entryInfo(0, "", entryID, "", DirEntryType_REGULARFILE,0);

   *outFileInode = referenceFile(&entryInfo);

   if (*outFileInode)
      return true;

   // neither worked as dir nor as file
   return false;
}


FhgfsOpsErr MetaStore::isFileUnlinkable(DirInode* subDir, EntryInfo *entryInfo)
{
   FhgfsOpsErr isUnlinkable = this->fileStore.isUnlinkable(entryInfo);
   if (isUnlinkable == FhgfsOpsErr_SUCCESS)
      isUnlinkable = subDir->fileStore.isUnlinkable(entryInfo);

   return isUnlinkable;
}


/**
 * Move a fileInode reference from subDir->fileStore to (MetaStore) this->fileStore
 *
 * @param subDir can be NULL, the dir then must not be locked at all. If subDir is not NULL,
 *               it must be write-locked already.
 * @return true if an existing reference was moved
 *
 * Note: MetaStore needs to be write locked!
 */
bool MetaStore::moveReferenceToMetaFileStoreUnlocked(std::string parentEntryID, std::string entryID)
{
   bool retVal = false;
   const char* logContext = "MetaStore (Move reference from per-Dir fileStore to global Store)";

   DirInode* subDir = referenceAndWriteLockDirUnlocked(parentEntryID);
   if (unlikely(!subDir) )
       return false;

   FileInodeReferencer* inodeRefer = subDir->fileStore.getReferencerAndDeleteFromMap(entryID);
   if (likely(inodeRefer) )
   {
      // The inode is referenced in the per-directory fileStore. Move it to the global store.

      FileInode* inode = inodeRefer->reference();
      ssize_t numParentRefs = inode->getNumParentRefs();

      retVal = this->fileStore.insertReferencer(entryID, inodeRefer);
      if (unlikely(retVal == false) )
      {
         std::string msg = "Bug: Failed to move to MetaStore FileStore - already exists in map"
            " ParentID: " + parentEntryID + " EntryID: " + entryID;
         LogContext(logContext).logErr(msg);
         /* NOTE: We are leaking memory here, but as there is a general bug, this is better
          *       than trying to free an object possibly in-use. */
      }
      else
      {
         // LOG_DEBUG(logContext, Log_SPAM, std::string("Releasing dir references entryID: ") +
         //   entryID + " dirID: " + subDir->getID() );
         while (numParentRefs > 0)
         {
            releaseDirUnlocked(parentEntryID); /* inode references also always keep a dir reference,
                                                * so we need to release the dir as well */
            numParentRefs--;
            inode->decParentRef();
         }

         inodeRefer->release();

         }
   }
   else
   {
      /* possible race of unlinkInodeLaterUnlocked (we has to give up all locks before calling it)
       * with close() */
      // LOG_DEBUG(logContext, Log_SPAM, "FileInodeReferencer does not exist in map. "
      //   " ParentID: " + parentEntryID + " EntryID: " + entryID);

      retVal = true; /* it is probably a valid race, so do not return an error,
                      * unlinkInodeLaterUnlocked() also can handle it as it does fine this inode
                      * in the global FileStore then */
   }

   unlockAndReleaseDirUnlocked(parentEntryID, subDir);

   return retVal;
}

/**
 * @param accessFlags OPENFILE_ACCESS_... flags
 */
FhgfsOpsErr MetaStore::openFile(EntryInfo* entryInfo, unsigned accessFlags, FileInode** outInode)
{
   FhgfsOpsErr retVal;

   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   /* check the MetaStore fileStore map here, but most likely the file will not be in this map,
    * but is either in the per-directory-map or has to be loaded from disk */
   if (this->fileStore.isInStore(entryInfo->getEntryID() ) )
   {
      retVal = this->fileStore.openFile(entryInfo, accessFlags, outInode, false);
   }
   else
   {
      // not in global map, now per directory and also try to load from disk
      std::string parentEntryID = entryInfo->getParentEntryID();

      DirInode* subDir = referenceAndReadLockDirUnlocked(parentEntryID);
      if (likely(subDir) )
      {
         retVal = subDir->fileStore.openFile(entryInfo, accessFlags, outInode, true);

         if (*outInode)
         {
            (*outInode)->incParentRef(parentEntryID);
            unlockDirUnlocked(subDir); /* Do not release dir here, we are returning the inode
                                        * referenced in subDirs fileStore! */
         }
         else
            unlockAndReleaseDirUnlocked(entryInfo->getParentEntryID(), subDir);
      }
      else
         retVal = FhgfsOpsErr_INTERNAL;
   }

   safeLock.unlock(); // U N L O C K

   return retVal;
}

/**
 * @param accessFlags OPENFILE_ACCESS_... flags
 * @param outNumHardlinks for quick on-close unlink check
 * @param outNumRef also for on-close unlink check
 */
void MetaStore::closeFile(EntryInfo* entryInfo, FileInode* inode, unsigned accessFlags,
   unsigned* outNumHardlinks, unsigned* outNumRefs)
{
   const char* logContext = "Close file";
   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   // now release (and possible free (delete) the inode if not further referenced)

   // first try global metaStore map
   bool closeRes = this->fileStore.closeFile(entryInfo, inode, accessFlags,
      outNumHardlinks, outNumRefs);
   if (!closeRes)
   {
      // not in global /store/map, now per directory
      DirInode* subDir = referenceAndReadLockDirUnlocked(entryInfo->getParentEntryID() );
      if (likely(subDir) )
      {
         inode->decParentRef(); /* Already decrease it here, as the inode might get destroyed
                                 * in closeFile(). The counter is very important if there is
                                 * another open reference on this file and if the file is unlinked
                                 * while being open */

         closeRes = subDir->fileStore.closeFile(entryInfo, inode, accessFlags,
            outNumHardlinks, outNumRefs);
         if (!closeRes)
         {
            LOG_DEBUG(logContext, Log_SPAM, "File not open: " + entryInfo->getEntryID() );
            IGNORE_UNUSED_VARIABLE(logContext);
         }

         unlockAndReleaseDirUnlocked(entryInfo->getParentEntryID(), subDir);

         // we kept another dir reference in openFile(), so release it here
         releaseDirUnlocked(entryInfo->getParentEntryID() );
      }
   }

   safeLock.unlock(); // U N L O C K
}

/**
 * get statData of a DirInode or FileInode.
 *
 * @param outParentNodeID maybe NULL (default). Its value for FileInodes is always 0, as
 *    the value is undefined for files due to possible hard links.
 * @param outParentEntryID, as with outParentNodeID it is undefined for files
 */
FhgfsOpsErr MetaStore::stat(EntryInfo* entryInfo, bool loadFromDisk, StatData& outStatData,
   uint16_t* outParentNodeID, std::string* outParentEntryID)
{
   FhgfsOpsErr statRes = FhgfsOpsErr_PATHNOTEXISTS;

   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   if (entryInfo->getEntryType() == DirEntryType_DIRECTORY)
   { // entry is a dir
      statRes = dirStore.stat(entryInfo->getEntryID(), outStatData,
         outParentNodeID, outParentEntryID);
   }
   else
   { // entry is any type, but a directory, e.g. a regular file

      if (outParentNodeID)
      {  // no need to set these for a regular file
         *outParentNodeID = 0;
      }

      // first check if the inode is referenced in the global store
      statRes = fileStore.stat(entryInfo, false, outStatData); // do not try to load from disk!

      if (statRes == FhgfsOpsErr_PATHNOTEXISTS) // not in global store, now per directory
      {
         // not in global /store/map, now per directory
         DirInode* subDir = referenceAndReadLockDirUnlocked(entryInfo->getParentEntryID() );
         if (likely(subDir))
         {
            statRes = subDir->fileStore.stat(entryInfo, loadFromDisk, outStatData);

            unlockAndReleaseDirUnlocked(entryInfo->getParentEntryID(), subDir);
         }
      }
   }

   safeLock.unlock(); // U N L O C K

   return statRes;
}

FhgfsOpsErr MetaStore::setAttr(EntryInfo* entryInfo, int validAttribs, SettableFileAttribs* attribs)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   FhgfsOpsErr setAttrRes = setAttrUnlocked(entryInfo, validAttribs, attribs);

   safeLock.unlock(); // U N L O C K

   return setAttrRes;
}

/**
 * @param validAttribs SETATTR_CHANGE_...-Flags or no flags to only update attribChangeTimeSecs
 * @param attribs  new attribs, may be NULL if validAttribs==0
 */
FhgfsOpsErr MetaStore::setAttrUnlocked(EntryInfo* entryInfo, int validAttribs,
   SettableFileAttribs* attribs)
{
   FhgfsOpsErr setAttrRes = FhgfsOpsErr_PATHNOTEXISTS;

   std::string localNodeID = Program::getApp()->getLocalNode()->getID();

   if (DirEntryType_ISDIR(entryInfo->getEntryType()) )
      setAttrRes = dirStore.setAttr(entryInfo->getEntryID(), validAttribs, attribs);
   else
   {
      if (this->fileStore.isInStore(entryInfo->getEntryID() ) )
         setAttrRes = this->fileStore.setAttr(entryInfo, validAttribs, attribs, 0);
      else
      {
         // not in global /store/map, now per directory
         DirInode* subDir = referenceAndReadLockDirUnlocked(entryInfo->getParentEntryID(), true);
         if (likely(subDir) )
         {
            uint16_t mirrorNodeID = (subDir->getFeatureFlags() & DIRINODE_FEATURE_MIRRORED) ?
               subDir->getMirrorNodeID() : 0;
            setAttrRes = subDir->fileStore.setAttr(entryInfo, validAttribs, attribs, mirrorNodeID);

            unlockAndReleaseDirUnlocked(entryInfo->getParentEntryID(), subDir);
         }
      }
   }

   return setAttrRes;
}

/**
 * Set / update the parent information of a dir inode
 */
FhgfsOpsErr MetaStore::setDirParent(EntryInfo* entryInfo, uint16_t parentNodeID)
{
   if (unlikely(!DirEntryType_ISDIR(entryInfo->getEntryType() ) ) )
       return FhgfsOpsErr_INTERNAL;

    std::string dirID = entryInfo->getEntryID();

   DirInode* dir = referenceDir(dirID, true);
   if (!dir)
      return FhgfsOpsErr_PATHNOTEXISTS;

   // also update the time stamps
   FhgfsOpsErr setRes = dir->setDirParentAndChangeTime(entryInfo, parentNodeID);

   releaseDir(dirID);

   return setRes;
}

/**
 * Create a File (dentry + inlined-inode) from an existing inode.
 *
 * @param dir         Already needs to be locked by the caller.
 * @param inode       Take values from this inode to create the new file. Object will be destroyed
 *                    here.
 */
FhgfsOpsErr MetaStore::mkMetaFileUnlocked(DirInode* dir, std::string entryName,
   DirEntryType entryType, FileInode* inode)
{
   App* app = Program::getApp();
   Node* localNode = app->getLocalNode();

   std::string entryID = inode->getEntryID();
   FileInodeStoreData inodeDiskData(entryID, inode->getInodeDiskData() );

   inodeDiskData.setInodeFeatureFlags(inode->getFeatureFlags() );

   uint16_t ownerNodeID = localNode->getNumID();
   DirEntry newDentry (entryType, entryName, entryID, ownerNodeID);

   newDentry.setFileInodeData(inodeDiskData);

   inodeDiskData.setPattern(NULL); /* pattern now owned by newDentry, so make sure it won't be
                                    * deleted on inodeMetadata object destruction */

   // create a dir-entry with inlined inodes
   FhgfsOpsErr makeRes = dir->makeDirEntryUnlocked(&newDentry, false);

   delete inode;

   return makeRes;
}


/**
 * Create a new File (directory-entry with inlined inode)
 *
 * @param stripePattern     must be provided; will be destructed internally (even in case of error)
 *                          or will be assigned to given outInodeData.
 * @param outEntryInfo      will not be set if NULL (caller not interested)
 * @param outInodeData      will not be set if NULL (caller not interested)
 */
FhgfsOpsErr MetaStore::mkNewMetaFile(DirInode* dir, MkFileDetails* mkDetails,
   StripePattern* stripePattern, EntryInfo* outEntryInfo, FileInodeStoreData* outInodeData)
{
   SafeRWLock metaLock(&this->rwlock, SafeRWLock_READ);
   SafeRWLock dirLock(&dir->rwlock, SafeRWLock_WRITE); // L O C K ( dir )

   FhgfsOpsErr retVal = mkNewMetaFileUnlocked(dir, mkDetails, stripePattern,
      outEntryInfo, outInodeData);

   dirLock.unlock();
   metaLock.unlock();

   return retVal;
}

/**
 * Create a new File (directory-entry with inlined inode), unlocked version
 *
 * @param dir             already needs to be locked by the caller.
 * @param stripePattern   must be provided; will be destructed internally (even in case of error)
 *                        or will be assigned to given outInodeData.
 * @param outEntryInfo    will not be set if NULL (caller not interested)
 * @param outInodeData    will not be set if NULL (caller not interested)
 */
FhgfsOpsErr MetaStore::mkNewMetaFileUnlocked(DirInode* dir, MkFileDetails* mkDetails,
   StripePattern* stripePattern, EntryInfo* outEntryInfo, FileInodeStoreData* outInodeData)
{
   const char* logContext = "Make New Meta File";
   App* app = Program::getApp();
   Config* config = app->getConfig();
   Node* localNode = app->getLocalNode();

   std::string newEntryID = StorageTk::generateFileID(localNode->getNumID() );
   uint16_t ownerNodeID = localNode->getNumID();
   std::string parentEntryID = dir->getID();

   EntryInfo tmpInfo;

   if (!outEntryInfo)
      outEntryInfo = &tmpInfo;

   // load DirInode on demand if required, we need it now
   if (dir->loadIfNotLoadedUnlocked() == false)
   {
      delete stripePattern;
      return FhgfsOpsErr_PATHNOTEXISTS;
   }

   CharVector aclXAttr;
   bool needsACL;
   if (config->getStoreClientACLs() )
   {
      // Find out if parent dir has an ACL.
      FhgfsOpsErr aclXAttrRes = getDirXAttr(dir, PosixACL::defaultACLXAttrName, aclXAttr);

      if (aclXAttrRes == FhgfsOpsErr_SUCCESS)
      {
         // dir has a default acl.
         PosixACL defaultACL;
         if (!defaultACL.deserializeXAttr(aclXAttr) )
         {
            LogContext(logContext).log(Log_ERR, "Error deserializing directory default ACL.");
            return FhgfsOpsErr_INTERNAL;
         }
         else
         {
            // Note: This modifies mkDetails->mode as well as the ACL.
            FhgfsOpsErr modeRes = defaultACL.modifyModeBits(mkDetails->mode, needsACL);

            if (modeRes != FhgfsOpsErr_SUCCESS)
               return modeRes;

            if (needsACL)
            {
               aclXAttr.resize(defaultACL.serialLenXAttr() );
               defaultACL.serializeXAttr(&aclXAttr.front() );
            }
         }
      }
      else
      if (aclXAttrRes == FhgfsOpsErr_NODATA)
      {
         // Directory does not have a default ACL - subtract umask from mode bits.
         mkDetails->mode &= ~mkDetails->umask;
         needsACL = false;
      }
      else
      {
         LogContext(logContext).log(Log_ERR, "Error loading directory default ACL.");
         return FhgfsOpsErr_INTERNAL;
      }
   }
   else
      needsACL = false;

   DirEntryType entryType = MetadataTk::posixFileTypeToDirEntryType(mkDetails->mode);
   StatData statData(mkDetails->mode, mkDetails->userID, mkDetails->groupID,
      stripePattern->getAssignedNumTargets() );

   unsigned flags = ENTRYINFO_FEATURE_INLINED; // new files always have inlined inodes
   unsigned origParentUID = dir->getUserIDUnlocked(); // new file, we use the parent UID
   std::string origParentEntryID = dir->getID(); // new file, so current dir

   // (note: inodeMetaData constructor clones stripePattern)
   FileInodeStoreData inodeMetaData(newEntryID, &statData, stripePattern, flags,
      origParentUID, origParentEntryID, FileInodeOrigFeature_TRUE);

#ifdef BEEGFS_HSM_DEPRECATED
   HsmFileMetaData hsmFileMetaData(dir->getHsmCollocationIDUnlocked());
   inodeMetaData.setHsmFileMetaData(hsmFileMetaData);
#endif

   DirEntry* newDentry = new DirEntry(entryType, mkDetails->newName, newEntryID, ownerNodeID);

   newDentry->setFileInodeData(inodeMetaData);
   inodeMetaData.setPattern(NULL); /* cloned pattern now belongs to newDentry, so make sure it won't
                                      be deleted in inodeMetaData destructor */

   // create a dir-entry with inlined inodes
   FhgfsOpsErr makeRes = dir->makeDirEntryUnlocked(newDentry, false);

   if(makeRes != FhgfsOpsErr_SUCCESS)
   { // addding the dir-entry failed
      delete(stripePattern);
   }
   else
   { // new entry successfully created

      if (!outInodeData)
         delete stripePattern;
      else
      { // set caller's outInodeData
         outInodeData->setInodeStatData(statData);
         outInodeData->setPattern(stripePattern); // (will be deleted by outInodeData destructor)
         outInodeData->setEntryID(newEntryID);

         #ifdef BEEGFS_HSM_DEPRECATED
            outInodeData->setHsmFileMetaData(hsmFileMetaData);
         #endif
      }

   }

   outEntryInfo->set(ownerNodeID, parentEntryID, newEntryID, mkDetails->newName, entryType, flags);

   // apply access ACL calculated from default ACL
   if (needsACL)
   {
      FhgfsOpsErr setXAttrRes = dir->setXAttr(outEntryInfo, mkDetails->newName,
         PosixACL::accessACLXAttrName, aclXAttr, 0, false);

      if (setXAttrRes != FhgfsOpsErr_SUCCESS)
      {
         LogContext(logContext).log(Log_ERR, "Error setting file ACL.");
         makeRes = FhgfsOpsErr_INTERNAL;
      }
   }

   delete newDentry;

   return makeRes;
}


FhgfsOpsErr MetaStore::makeDirInode(DirInode* inode)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   FhgfsOpsErr mkRes = dirStore.makeDirInode(inode);

   safeLock.unlock(); // U N L O C K

   return mkRes;
}

FhgfsOpsErr MetaStore::makeDirInode(DirInode* inode, const CharVector& defaultACLXAttr,
   const CharVector& accessACLXAttr)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   FhgfsOpsErr mkRes = dirStore.makeDirInode(inode, defaultACLXAttr, accessACLXAttr);

   safeLock.unlock(); // U N L O C K

   return mkRes;
}


FhgfsOpsErr MetaStore::removeDirInode(std::string entryID)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   FhgfsOpsErr rmRes = dirStore.removeDirInode(entryID);

   safeLock.unlock(); // U N L O C K

   return rmRes;
}

/**
 * Unlink a non-inlined file inode
 *
 * Note: Specialy case without an entryInfo, for fsck only!
 */
FhgfsOpsErr MetaStore::fsckUnlinkFileInode(std::string entryID)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   // generic code needs an entryInfo, but most values can be empty for non-inlined inodes
   uint16_t ownerNodeID = 0;
   std::string parentEntryID;
   std::string fileName;
   DirEntryType entryType = DirEntryType_REGULARFILE;
   int flags = 0;

   EntryInfo entryInfo(ownerNodeID, parentEntryID, entryID, fileName, entryType, flags);

   FhgfsOpsErr unlinkRes = this->fileStore.unlinkFileInode(&entryInfo, NULL);

   safeLock.unlock();

   return unlinkRes;
}

/**
 * @param subDir may be NULL and then needs to be referenced
 */
FhgfsOpsErr MetaStore::unlinkInodeUnlocked(EntryInfo* entryInfo, DirInode* subDir,
   FileInode** outInode)
{
   FhgfsOpsErr unlinkRes;

   if (this->fileStore.isInStore(entryInfo->getEntryID() ) )
      unlinkRes = fileStore.unlinkFileInode(entryInfo, outInode);
   else
   {
      if (!subDir)
      {
         // not in global /store/map, now per directory
         subDir = referenceAndReadLockDirUnlocked(entryInfo->getParentEntryID() );
         if (likely(subDir) )
         {
            unlinkRes = subDir->fileStore.unlinkFileInode(entryInfo, outInode);

            unlockAndReleaseDirUnlocked(entryInfo->getParentEntryID(), subDir); /* we can release the DirInode here, as the
                                                  * FileInode is not supposed to be in the
                                                  * DirInodes FileStore anymore */
         }
         else
            unlinkRes = FhgfsOpsErr_PATHNOTEXISTS;
      }
      else // subDir already referenced and locked
         unlinkRes = subDir->fileStore.unlinkFileInode(entryInfo, outInode);
   }

   return unlinkRes;
}


/**
 * @param outFile will be set to the unlinked file and the object must then be deleted by the caller
 * (can be NULL if the caller is not interested in the file)
 */
FhgfsOpsErr MetaStore::unlinkInode(EntryInfo* entryInfo, FileInode** outInode)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   FhgfsOpsErr unlinkRes = this->unlinkInodeUnlocked(entryInfo, NULL, outInode);

   safeLock.unlock(); // U N L O C K

   return unlinkRes;
}

/**
 * need the following locks:
 *   this->rwlock: SafeRWLock_WRITE
 *   subDir: reference
 *   subdir->rwlock: SafeRWLock_WRITE
 *
 *   note: caller needs to delete storage chunk files. E.g. via MsgHelperUnlink::unlinkLocalFile()
 */
FhgfsOpsErr MetaStore::unlinkFileUnlocked(DirInode* subdir, std::string fileName,
   FileInode** outInode, EntryInfo* outEntryInfo, bool& outWasInlined)
{
   FhgfsOpsErr retVal;

   DirEntry* dirEntry = subdir->dirEntryCreateFromFileUnlocked(fileName);
   if (!dirEntry)
      return FhgfsOpsErr_PATHNOTEXISTS;

   // dirEntry exists time to make sure we have loaded subdir
   bool loadRes = subdir->loadIfNotLoadedUnlocked();
   if (unlikely(!loadRes) )
      return FhgfsOpsErr_INTERNAL; // if dirEntry exists, subDir also has to exist!

   // set the outEntryInfo
   int additionalFlags = 0;
   std::string parentEntryID = subdir->getID();
   dirEntry->getEntryInfo(parentEntryID, additionalFlags, outEntryInfo);

   if (dirEntry->getIsInodeInlined() )
   {  // inode is inlined into the dir-entry
      retVal = unlinkDirEntryWithInlinedInodeUnlocked(fileName, subdir, dirEntry,
        DirEntry_UNLINK_ID_AND_FILENAME, outInode);

      outWasInlined = true;
   }
   else
   {  // inode and dir-entry are separated fileStore
      retVal = unlinkDentryAndInodeUnlocked(fileName, subdir, dirEntry,
         DirEntry_UNLINK_ID_AND_FILENAME, outInode);

      outWasInlined = false;
   }

   delete dirEntry; // we explicitly checked above if dirEntry != NULL, so no SAFE required

   return retVal;
}

/**
 * Unlinks the entire file, so dir-entry and inode.
 *
 * @param fileName friendly name
 * @param outEntryInfo contains the entryInfo of the unlinked file
 * @param outInode will be set to the unlinked (owned) file and the object and
 * storage server fileStore must then be deleted by the caller; even if success is returned,
 * this might be NULL (e.g. because the file is in use and was added to the disposal directory);
 * can be NULL if the caller is not interested in the file
 * @return normal fhgfs error code, normally succeeds even if a file was open; special case is
 * when this is called to unlink a file with the disposalDir dirID, then an open file will
 * result in a inuse-error (required for online_cfg mode=dispose)
 *
 * note: caller needs to delete storage chunk files. E.g. via MsgHelperUnlink::unlinkLocalFile()
 */
FhgfsOpsErr MetaStore::unlinkFile(std::string dirID, std::string fileName, EntryInfo* outEntryInfo,
   FileInode** outInode)
{
   const char* logContext = "Unlink File";
   FhgfsOpsErr retVal = FhgfsOpsErr_PATHNOTEXISTS;

   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   DirInode* subdir = referenceAndWriteLockDirUnlocked(dirID);
   if(!subdir)
   {
      safeLock.unlock();
      return retVal;
   }

   bool wasInlined;
   retVal = unlinkFileUnlocked(subdir, fileName, outInode, outEntryInfo, wasInlined);

   /* We may release the dir here, as the FileInode is not supposed to be in the DirInodes
    * FileStore anymore. */
   unlockAndReleaseDirUnlocked(dirID, subdir);

   /* Give up the read-lock here, unlinkInodeLater() will aquire a write lock. We already did
    * most of the work, just possible back linking of the inode to the disposal dir is missing.
    * As our important work is done, we can also risk to give up the read lock. */
   safeLock.unlock(); // U N L O C K

   if (retVal == FhgfsOpsErr_INUSE)
   {
      if (dirID != META_DISPOSALDIR_ID_STR)
      {  // we already successfully deleted the dentry, so all fine for the user
         retVal = FhgfsOpsErr_SUCCESS;
      }

      FhgfsOpsErr laterRes = unlinkInodeLater(outEntryInfo, wasInlined);
      if (laterRes == FhgfsOpsErr_AGAIN)
      {
         /* So the inode was not referenced in memory anymore and probably close() already deleted
          * it. Just make sure here it really does not exist anymore */
         FhgfsOpsErr inodeUnlinkRes = unlinkInode(outEntryInfo, outInode);

         if (unlikely((inodeUnlinkRes != FhgfsOpsErr_PATHNOTEXISTS) &&
            (inodeUnlinkRes != FhgfsOpsErr_SUCCESS) ) )
         {
            LogContext(logContext).logErr(std::string("Failed to unlink inode. Error: ") +
               FhgfsOpsErrTk::toErrString(inodeUnlinkRes) );

            retVal = inodeUnlinkRes;
         }
      }

   }

   return retVal;
}

/**
 * Unlink a dirEntry with an inlined inode
 */
FhgfsOpsErr MetaStore::unlinkDirEntryWithInlinedInodeUnlocked(std::string entryName,
   DirInode* subDir, DirEntry* dirEntry, unsigned unlinkTypeFlags, FileInode** outInode)
{
   const char* logContext = "Unlink DirEntry with inlined inode";
   FhgfsOpsErr retVal;

   if (outInode)
      *outInode = NULL; // initialize with NULL first and overwrite later if appropriate

   std::string parentEntryID = subDir->getID();

   // when we are here, we no the inode is inlined into the dirEntry
   int flags = ENTRYINFO_FEATURE_INLINED;

   EntryInfo entryInfo;
   dirEntry->getEntryInfo(parentEntryID, flags, &entryInfo);

   FhgfsOpsErr isUnlinkable = isFileUnlinkable(subDir, &entryInfo);

   uint16_t mirrorNodeID = subDir->getMirrorNodeID();

   // note: FhgfsOpsErr_PATHNOTEXISTS cannot happen with isUnlinkable(id, false)

   if (isUnlinkable == FhgfsOpsErr_INUSE)
   {
      // for some reasons we cannot unlink the inode, probably the file is opened. De-inline it.

      // *outInode = NULL; // the caller must not do anything with this inode

      FileInode* inode = referenceLoadedFileUnlocked(subDir, dirEntry->getEntryID() );
      if (likely(inode) ) // likely as MetaStore is locked and called isFileUnlinkable() before
      {
         unsigned numHardlinks = inode->getNumHardlinks();

         if (numHardlinks > 1)
            unlinkTypeFlags &= ~DirEntry_UNLINK_ID;

         bool unlinkError = subDir->unlinkBusyFileUnlocked(entryName, dirEntry, unlinkTypeFlags);
         if (unlinkError)
            retVal = FhgfsOpsErr_INTERNAL;
         else
         {  // unlink success
            if (numHardlinks < 2)
            {
               retVal = FhgfsOpsErr_INUSE;

               // The inode is not inlined anymore, update the in-memory objects

               dirEntry->unsetInodeInlined();
               inode->setIsInlined(false);
            }
            else
            {  // hard link exists, the inode is still inlined
               retVal = FhgfsOpsErr_SUCCESS;
            }

            /* Decrease the link count, but as dentry and inode are still hard linked objects,
             * it also unsets the DENTRY_FEATURE_INODE_INLINE on disk */
            FhgfsOpsErr decRes = subDir->fileStore.decLinkCount(inode, &entryInfo, mirrorNodeID);
            if (decRes != FhgfsOpsErr_SUCCESS)
            {
               LogContext(logContext).logErr("Failed to decrease the link count!"
                  " parentID: "      + entryInfo.getParentEntryID() +
                  " entryID: "       + entryInfo.getEntryID()       +
                  " entryNameName: " + entryName);
            }
         }

         releaseFileUnlocked(subDir, inode);
      }
      else
      {
         LogContext(logContext).logErr("Bug: Busy inode found, but failed to reference it");
         retVal = FhgfsOpsErr_INTERNAL;
      }

      /* We are done here, but the file still might be referenced in the per-dir file store.
       * However, we cannot move it, as we do not have a MetaStore write-lock, but only a
       * read-lock. Therefore that has to be done later on, once we have given up the read-lock. */
   }
   else
   if (isUnlinkable == FhgfsOpsErr_SUCCESS)
   {
      // dir-entry and inode are inlined. The file is also not opened anymore, so delete it.

      if (unlinkTypeFlags & DirEntry_UNLINK_ID)
      {
         FileInode* inode = referenceFileUnlocked(subDir, &entryInfo);
         if (inode)
         {
            unsigned numHardlinks = inode->getNumHardlinks();

            if (numHardlinks > 1)
               unlinkTypeFlags &= ~DirEntry_UNLINK_ID;

            retVal = subDir->unlinkDirEntryUnlocked(entryName, dirEntry, unlinkTypeFlags);

            if (retVal == FhgfsOpsErr_SUCCESS && numHardlinks > 1)
            {
               FhgfsOpsErr decRes = subDir->fileStore.decLinkCount(inode, &entryInfo, mirrorNodeID);
               if (decRes != FhgfsOpsErr_SUCCESS)
               {
                  LogContext(logContext).logErr("Failed to decrease the link count!"
                     " parentID: "      + entryInfo.getParentEntryID() +
                     " entryID: "       + entryInfo.getEntryID()       +
                     " entryNameName: " + entryName);
               }
            }

            if (outInode && numHardlinks < 2)
            {
               inode->setIsInlined(false); // last dirEntry gone, so not inlined anymore
               *outInode = inode->clone();
            }

            releaseFileUnlocked(subDir, inode);
         }
         else
            retVal = FhgfsOpsErr_PATHNOTEXISTS;

      }
      else
      {
         // only the dentry-by-name is left, so no need to care about the inode
         retVal = subDir->unlinkDirEntryUnlocked(entryName, dirEntry, unlinkTypeFlags);
      }
   }
   else
      retVal = isUnlinkable;

   return retVal;
}

/**
 * Unlink seperated dirEntry and Inode
 */
FhgfsOpsErr MetaStore::unlinkDentryAndInodeUnlocked(std::string fileName,
   DirInode* subdir, DirEntry* dirEntry, unsigned unlinkTypeFlags, FileInode** outInode)
{
   // unlink dirEntry first
   FhgfsOpsErr retVal = subdir->unlinkDirEntryUnlocked(fileName, dirEntry, unlinkTypeFlags);

   if(retVal == FhgfsOpsErr_SUCCESS)
   { // directory-entry was removed => unlink inode

      // if we are here we know the dir-entry does not inline the inode

      int addionalEntryInfoFlags = 0;
      std::string parentEntryID = subdir->getID();
      EntryInfo entryInfo;

      dirEntry->getEntryInfo(parentEntryID, addionalEntryInfoFlags, &entryInfo);

      FhgfsOpsErr unlinkFileRes = this->unlinkInodeUnlocked(&entryInfo, subdir, outInode);
      if(unlinkFileRes != FhgfsOpsErr_SUCCESS && unlinkFileRes == FhgfsOpsErr_INUSE)
         retVal = FhgfsOpsErr_INUSE;
   }

   return retVal;
}

/**
 * Adds the entry to the disposal directory for later (asynchronous) disposal.
 */
FhgfsOpsErr MetaStore::unlinkInodeLater(EntryInfo* entryInfo, bool wasInlined)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE); // L O C K

   FhgfsOpsErr retVal = unlinkInodeLaterUnlocked(entryInfo, wasInlined);

   safeLock.unlock(); // U N L O C K

   return retVal;
}

/**
 * Adds the inode (with a new dirEntry) to the disposal directory for later
 * (on-close or asynchronous) disposal.
 *
 * Note: We are going to set outInode if we determine that the file was closed in the mean time
 *       and all references to this inode shall be deleted.
 *
 */
FhgfsOpsErr MetaStore::unlinkInodeLaterUnlocked(EntryInfo* entryInfo, bool wasInlined)
{
   // Note: We must not try to unlink the inode here immediately, because then the local versions
   //       of the data-object (on the storage nodes) would never be deleted.

   App* app = Program::getApp();
   FhgfsOpsErr retVal = FhgfsOpsErr_SUCCESS;

   std::string parentEntryID = entryInfo->getParentEntryID();
   std::string entryID = entryInfo->getEntryID();

   DirInode* disposalDir = app->getDisposalDir();
   std::string disposalID = disposalDir->getID();

   /* This requires a MetaStore write lock, therefore only can be done here and not in common
    * unlink code, as the common unlink code only has a MetaStore read-lock. */
   if (!this->fileStore.isInStore(entryID) )
   {
      bool moveRes = moveReferenceToMetaFileStoreUnlocked(parentEntryID, entryID);
      if (moveRes == false)
         return FhgfsOpsErr_INTERNAL; /* a critical error happened, better don't do
                                       * anything with this inode anymore */
   }

   entryInfo->setParentEntryID(disposalID); // update the dirID to disposalDir

   // set inode nlink count to 0.
   // we assume the inode is typically already referenced by someone (otherwise we wouldn't need to
   // unlink later) and this allows for faster checking on-close than the disposal dir check.

   // do not load the inode from disk first.
   FileInode* inode = this->fileStore.referenceLoadedFile(entryID);
   if(likely(inode) )
   {
      int linkCount = -1;

      if (!wasInlined) // if the inode was inlined, it was already updated before
      {
         inode->setIsInlined(false); // it's now definitely not inlined anymore
         linkCount = inode->getNumHardlinks();
         if (likely (linkCount > 0) )
            inode->setNumHardlinksUnpersistent(--linkCount);
         inode->updateInodeOnDisk(entryInfo);
      }

      this->fileStore.releaseFileInode(inode);

      /* Now link to the disposal-dir if required. If the inode was inlined into the dentry,
       * the dentry/inode unlink code already links to the disposal dir and we do not need to do
       * this work. */
      if (!wasInlined && linkCount == 0)
      {
         DirInode* disposalDir = app->getDisposalDir();

         std::string inodePath  = MetaStorageTk::getMetaInodePath(
            app->getInodesPath()->getPathAsStrConst(), entryID);

         /* NOTE: If we are going to have another inode-format, than the current
          *       inode-inlined into the dentry, we need to add code for that here. */

         disposalDir->linkFileInodeToDir(inodePath, entryID); // use entryID as file name
         // ignore the return code here, as we cannot do anything about it anyway.
      }
   }
   else
   {
      /* If we cannot reference the inode from memory, we raced with close().
       * This is possible as we gave up all locks in unlinkFile() and it means the inode/file shall
       * be deleted entirely on disk now. */

      entryInfo->setInodeInlinedFlag(false);

      retVal = FhgfsOpsErr_AGAIN;
   }

   return retVal;
}

/**
 * Reads all inodes from the given inodes storage hash subdir.
 *
 * Note: This is intended for use by Fsck only.
 *
 * Note: Offset is an internal value and should not be assumed to be just 0, 1, 2, 3, ...;
 * so make sure you use either 0 (at the beginning) or something that has been returned by this
 * method as outNewOffset.
 * Note: You have reached the end of the directory when "outEntryIDs.size() != maxOutNames".
 *
 *
 * @param hashDirNum number of a hash dir in the "entries" storage subdir
 * @param lastOffset zero-based offset; represents the native local fs offset;
 * @param outDirInodes the read directory inodes in the format fsck saves them
 * @param outFileInodes the read file inodes in the format fsck saves them
 * @param outNewOffset is only valid if return value indicates success.
 */
FhgfsOpsErr MetaStore::getAllInodesIncremental(unsigned hashDirNum, int64_t lastOffset,
   unsigned maxOutInodes, FsckDirInodeList* outDirInodes, FsckFileInodeList* outFileInodes,
   int64_t* outNewOffset)
{
   const char* logContext = "MetaStore (get all inodes inc)";

   App* app = Program::getApp();

   uint16_t rootNodeNumID = app->getMetaNodes()->getRootNodeNumID();
   uint16_t localNodeNumID = app->getLocalNode()->getNumID();

   StringList entryIDs;
   unsigned firstLevelHashDir;
   unsigned secondLevelHashDir;
   StorageTk::splitHashDirs(hashDirNum, &firstLevelHashDir, &secondLevelHashDir);

   FhgfsOpsErr readRes = getAllEntryIDFilesIncremental(firstLevelHashDir, secondLevelHashDir,
      lastOffset, maxOutInodes, &entryIDs, outNewOffset);

   if (readRes != FhgfsOpsErr_SUCCESS)
   {
      LogContext(logContext).logErr(
         "Failed to read inodes from hash dirs; "
         "HashDir Level 1: " + StringTk::uintToStr(firstLevelHashDir) + "; "
         "HashDir Level 2: " + StringTk::uintToStr(firstLevelHashDir) );
      return readRes;
   }

   // the actual entry processing
   for ( StringListIter entryIDIter = entryIDs.begin(); entryIDIter != entryIDs.end();
      entryIDIter++ )
   {
      std::string entryID = *entryIDIter;

      // now try to reference the file and see what we got
      FileInode* fileInode = NULL;
      DirInode* dirInode = NULL;

      referenceInode(entryID, &fileInode, &dirInode);

      if (dirInode)
      {
         // entry is a directory
         std::string parentDirID;
         uint16_t parentNodeID = 0;
         dirInode->getParentInfo(&parentDirID, &parentNodeID);

         uint16_t ownerNodeID = dirInode->getOwnerNodeID();

         // in the unlikely case, that this is the root directory and this MDS is not the owner of
         // root ignore the entry
         if (unlikely( (entryID.compare(META_ROOTDIR_ID_STR) == 0)
            && rootNodeNumID != localNodeNumID) )
            continue;

         // not root => get stat data and create a FsckDirInode with data
         StatData statData;
         dirInode->getStatData(statData);

         UInt16Vector stripeTargets;
         FsckStripePatternType stripePatternType = FsckTk::stripePatternToFsckStripePattern(
            dirInode->getStripePattern(), NULL, &stripeTargets);

         FsckDirInode fsckDirInode(entryID, parentDirID, parentNodeID, ownerNodeID,
            statData.getFileSize(), statData.getNumHardlinks(), stripeTargets,
            stripePatternType, localNodeNumID);

         outDirInodes->push_back(fsckDirInode);

         this->releaseDir(entryID);
      }
      else
      if (fileInode)
      {
         // directory not successful => must be a file-like object

         // create a FsckFileInode with data
         std::string parentDirID;
         uint16_t parentNodeID = 0;
         UInt16Vector stripeTargets;

         PathInfo pathInfo;

         int mode;
         unsigned userID;
         unsigned groupID;

         int64_t fileSize;
         int64_t creationTime;
         int64_t modificationTime;
         int64_t lastAccessTime;
         unsigned numHardLinks;
         uint64_t numBlocks;

         StatData statData;

         fileInode->getPathInfo(&pathInfo);

         FhgfsOpsErr statRes = fileInode->getStatData(statData);

         if ( statRes == FhgfsOpsErr_SUCCESS )
         {
            mode = statData.getMode();
            userID = statData.getUserID();
            groupID = statData.getGroupID();
            fileSize = statData.getFileSize();
            creationTime = statData.getCreationTimeSecs();
            modificationTime = statData.getModificationTimeSecs();
            lastAccessTime = statData.getLastAccessTimeSecs();
            numHardLinks = statData.getNumHardlinks();
            numBlocks = statData.getNumBlocks();
         }
         else
         { // couldn't get the stat data
            LogContext(logContext).logErr(std::string("Unable to stat file inode: ") +
               entryID + std::string(". SysErr: ") + FhgfsOpsErrTk::toErrString(statRes));

            mode = 0;
            userID = 0;
            groupID = 0;
            fileSize = 0;
            creationTime = 0;
            modificationTime = 0;
            lastAccessTime = 0;
            numHardLinks = 0;
            numBlocks = 0;
         }

         StripePattern* stripePattern = fileInode->getStripePattern();
         unsigned chunkSize;
         FsckStripePatternType stripePatternType = FsckTk::stripePatternToFsckStripePattern(
            stripePattern, &chunkSize, &stripeTargets);

         FsckFileInode fsckFileInode(entryID, parentDirID, parentNodeID, pathInfo, mode, userID,
               groupID, fileSize, creationTime, modificationTime, lastAccessTime, numHardLinks,
               numBlocks, stripeTargets, stripePatternType, chunkSize, localNodeNumID, false);
            outFileInodes->push_back(fsckFileInode);

         // parentID is absolutely irrelevant here, because we know that this inode is not inlined
         this->releaseFile("", fileInode);
      }
      else
      { // something went wrong with inode loading

         // create a file inode as dummy (could also be a dir inode, but we chose fileinode)
         UInt16Vector stripeTargets;
         FsckDirInode fsckDirInode(entryID, "", 0, 0, 0, 0, stripeTargets,
            FsckStripePatternType_INVALID, localNodeNumID, false);
      }

   } // end of for loop

   return FhgfsOpsErr_SUCCESS;
}

/**
 * Reads all raw entryID filenames from the given "inodes" storage hash subdirs.
 *
 * Note: This is intended for use by the FullRefresher component and Fsck.
 *
 * Note: Offset is an internal value and should not be assumed to be just 0, 1, 2, 3, ...;
 * so make sure you use either 0 (at the beginning) or something that has been returned by this
 * method as outNewOffset.
 * Note: You have reached the end of the directory when "outEntryIDs.size() != maxOutNames".
 *
 * @param hashDirNum number of a hash dir in the "entries" storage subdir
 * @param lastOffset zero-based offset; represents the native local fs offset;
 * @param outEntryIDFiles the raw filenames of the entries in the given hash dir (so you will need
 * to remove filename suffixes to use these as entryIDs).
 * @param outNewOffset is only valid if return value indicates success.
 *
 */
FhgfsOpsErr MetaStore::getAllEntryIDFilesIncremental(unsigned firstLevelhashDirNum,
   unsigned secondLevelhashDirNum, int64_t lastOffset, unsigned maxOutEntries,
   StringList* outEntryIDFiles, int64_t* outNewOffset)
{
   const char* logContext = "Inode (get entry files inc)";
   App* app = Program::getApp();

   FhgfsOpsErr retVal = FhgfsOpsErr_INTERNAL;
   uint64_t numEntries = 0;
   struct dirent* dirEntry = NULL;

   std::string path = StorageTkEx::getMetaInodeHashDir(
      app->getInodesPath()->getPathAsStrConst(), firstLevelhashDirNum, secondLevelhashDirNum);


   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   DIR* dirHandle = opendir(path.c_str() );
   if(!dirHandle)
   {
      LogContext(logContext).logErr(std::string("Unable to open entries directory: ") +
         path + ". SysErr: " + System::getErrString() );

      goto err_unlock;
   }


   errno = 0; // recommended by posix (readdir(3p) )

   // seek to offset
   if(lastOffset)
      seekdir(dirHandle, lastOffset); // (seekdir has no return value)

   // the actual entry reading
   for( ; (numEntries < maxOutEntries) && (dirEntry = StorageTk::readdirFiltered(dirHandle) );
       numEntries++)
   {
      outEntryIDFiles->push_back(dirEntry->d_name);
      *outNewOffset = dirEntry->d_off;
   }

   if(!dirEntry && errno)
   {
      LogContext(logContext).logErr(std::string("Unable to fetch entries directory entry from: ") +
         path + ". SysErr: " + System::getErrString() );
   }
   else
   { // all entries read
      retVal = FhgfsOpsErr_SUCCESS;
   }


   closedir(dirHandle);

err_unlock:
   safeLock.unlock(); // U N L O C K

   return retVal;
}

void MetaStore::getReferenceStats(size_t* numReferencedDirs, size_t* numReferencedFiles)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   *numReferencedDirs = dirStore.getSize();
   *numReferencedFiles = fileStore.getSize();

   // TODO: walk through dirStore and get for all dirs the fileStore size

   safeLock.unlock(); // U N L O C K
}

void MetaStore::getCacheStats(size_t* numCachedDirs)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   *numCachedDirs = dirStore.getCacheSize();

   safeLock.unlock(); // U N L O C K
}

/**
 * Asynchronous cache sweep.
 *
 * @return true if a cache flush was triggered, false otherwise
 */
bool MetaStore::cacheSweepAsync()
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   bool retVal = dirStore.cacheSweepAsync();

   safeLock.unlock(); // U N L O C K

   return retVal;
}

/**
 * So we failed to delete chunk files and need to create a new disposal file for later cleanup.
 *
 * @param inode will be deleted (or owned by another object) no matter whether this succeeds or not
 *
 * Note: No MetaStore lock required, as the disposal dir cannot be removed.
 */
FhgfsOpsErr MetaStore::insertDisposableFile(FileInode* inode)
{
   LogContext log("MetaStore (insert disposable file)");

   DirInode* disposalDir = Program::getApp()->getDisposalDir();

   SafeRWLock metaLock(&this->rwlock, SafeRWLock_READ);
   SafeRWLock dirLock(&disposalDir->rwlock, SafeRWLock_WRITE); // L O C K ( dir )

   std::string fileName = inode->getEntryID(); // ID is also the file name
   DirEntryType entryType = MetadataTk::posixFileTypeToDirEntryType(inode->getMode() );

   FhgfsOpsErr retVal = mkMetaFileUnlocked(disposalDir, fileName, entryType, inode);
   if (retVal != FhgfsOpsErr_SUCCESS)
   {
      log.log(Log_WARNING,
         std::string("Failed to create disposal file for id: ") + fileName + "; "
         "Storage chunks will not be entirely deleted!");
   }

   dirLock.unlock();
   metaLock.unlock();

   return retVal;
}

/**
 * @param outInodeMetaData might be NULL
 * @return FhgfsOpsErr_SUCCESS if the entry was found and is not referenced,
 *         FhgfsOpsErr_DYNAMICATTRIBSOUTDATED is the entry was found, but is a referenced FileInode
 *         FhgfsOpsErr_PATHNOTEXISTS if the entry does not exist
 *
 * Locking: No lock must be taken already.
 */
FhgfsOpsErr MetaStore::getEntryData(DirInode *dirInode, std::string entryName,
   EntryInfo* outInfo, FileInodeStoreData* outInodeMetaData)
{
   FhgfsOpsErr retVal = dirInode->getEntryData(entryName, outInfo, outInodeMetaData);

   if (retVal == FhgfsOpsErr_SUCCESS && DirEntryType_ISREGULARFILE(outInfo->getEntryType() ) )
   {  /* Hint for the caller not to rely on outInodeMetaData, properly handling close-races
       * is too difficult and in the end probably slower than to just re-get the EAs from the
       * inode (fsIDs/entryID). */
      retVal = FhgfsOpsErr_DYNAMICATTRIBSOUTDATED;
   }

   return retVal;
}


/**
 * Create a hard-link within a directory.
 */
FhgfsOpsErr MetaStore::linkInSameDir(
   EntryInfo* parentInfo, EntryInfo* fromFileInfo, std::string fromName, std::string toName)
{
   const char* logContext = "link in same dir";
   FhgfsOpsErr retVal = FhgfsOpsErr_INTERNAL;
   FileInode *fromFileInode;

   // uint16_t localNodeID = app->getLocalNode()->getNumID();

   SafeRWLock metaLock(&this->rwlock, SafeRWLock_READ);

   std::string dirID = parentInfo->getEntryID();
   DirInode* parentDir = referenceAndWriteLockDirUnlocked(dirID);
   if (!parentDir || !parentDir->loadIfNotLoadedUnlocked() )
   {
      retVal = FhgfsOpsErr_PATHNOTEXISTS;
      goto outMetaUnlock;
   }

   fromFileInode = referenceFileUnlocked(parentDir, fromFileInfo);
   if (!fromFileInode)
   {
      retVal = FhgfsOpsErr_PATHNOTEXISTS;
      goto outUnlock;
   }

   if (!fromFileInode->getIsInlined() )
   {  // not supported yet, TODO
      retVal = FhgfsOpsErr_INTERNAL;
      goto outReleaseInode;
   }

   if (this->fileStore.isInStore(fromFileInfo->getEntryID() ) )
   {  // not supported yet, TODO
      retVal = FhgfsOpsErr_INTERNAL;
      goto outReleaseInode;
   }
   else
   {
      // not in global /store/map, now per directory

      /* Note:  Set to zero on purpose to avoid mirror messages. The inode will be updated anyway
       *        before we create the hard-link on the remote side, so no need to mirror the local
       *        inode attr update (linkCount) again. */
      uint16_t mirrorNodeID = 0;

      FhgfsOpsErr incRes = parentDir->fileStore.incLinkCount(fromFileInode, fromFileInfo,
         mirrorNodeID);
      if (incRes != FhgfsOpsErr_SUCCESS)
      {
         retVal = FhgfsOpsErr_INTERNAL;
         goto outReleaseInode;
      }

      retVal = parentDir->linkFilesInDirUnlocked(fromName, fromFileInode, toName);
      if (retVal != FhgfsOpsErr_SUCCESS)
      {
         FhgfsOpsErr decRes = parentDir->fileStore.decLinkCount(fromFileInode, fromFileInfo,
            mirrorNodeID);
         if (decRes != FhgfsOpsErr_SUCCESS)
            LogContext(logContext).logErr("Warning: Creating the link failed and decreasing "
               "the inode link count again now also failed!"
               " parentDir : " + parentInfo->getEntryID()   +
               " entryID: "    + fromFileInfo->getEntryID() +
               " fileName: "   + fromName);
      }
   }

outReleaseInode:
   releaseFileUnlocked(parentDir, fromFileInode);

outUnlock:
   unlockAndReleaseDirUnlocked(dirID, parentDir);

outMetaUnlock:
   metaLock.unlock();

   return retVal;
}

FhgfsOpsErr MetaStore::getDirXAttr(DirInode* dir, const std::string& xAttrName,
   CharVector& outValue)
{
   FhgfsOpsErr res;
   ssize_t size = 0;

   if (unlikely(!dir) )
      return FhgfsOpsErr_INTERNAL;

   res = dir->getXAttr(MsgHelperXAttr::CURRENT_DIR_FILENAME, xAttrName, outValue, size);

   if (res != FhgfsOpsErr_SUCCESS)
   {
      return res;
   }

   // Now the outValue has been resized to the size.
   outValue.resize(size);

   res = dir->getXAttr(MsgHelperXAttr::CURRENT_DIR_FILENAME, xAttrName, outValue, size);

   return res;
}


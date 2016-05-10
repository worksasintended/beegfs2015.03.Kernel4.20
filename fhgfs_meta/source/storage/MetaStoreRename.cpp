/*
 * MetaStoreRenameHelper.cpp
 *
 * These methods belong to class MetaStore, but are all related to rename()
 */

#include <common/storage/striping/Raid0Pattern.h>
#include <common/toolkit/FsckTk.h>
#include <net/msghelpers/MsgHelperStat.h>
#include <net/msghelpers/MsgHelperMkFile.h>
#include <program/Program.h>
#include "MetaStore.h"

#include <sys/types.h>
#include <dirent.h>
#include "MetaStore.h"

/**
 * Simple rename on the same server in the same directory.
 *
 * @param outUnlinkInode is the inode of a dirEntry being possibly overwritten (toName already
 *    existed).
 */
FhgfsOpsErr MetaStore::renameInSameDir(DirInode* parentDir, std::string fromName,
   std::string toName, FileInode** outUnlinkInode)
{
   const char* logContext = "Rename in dir";

   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K
   SafeRWLock fromMutexLock(&parentDir->rwlock, SafeRWLock_WRITE); // L O C K ( F R O M )

   FhgfsOpsErr retVal;
   FhgfsOpsErr unlinkRes;

   DirEntry* overWrittenEntry = NULL;

   retVal = performRenameEntryInSameDir(parentDir, fromName, toName, &overWrittenEntry);

   if (retVal != FhgfsOpsErr_SUCCESS)
   {
      fromMutexLock.unlock();
      safeLock.unlock();

      SAFE_DELETE(overWrittenEntry);

      return retVal;
   }

   EntryInfo unlinkEntryInfo;
   bool unlinkedWasInlined;

   if (overWrittenEntry)
   {
      std::string parentDirID = parentDir->getID();
      overWrittenEntry->getEntryInfo(parentDirID, 0, &unlinkEntryInfo);
      unlinkedWasInlined = overWrittenEntry->getIsInodeInlined();

      unlinkRes = unlinkOverwrittenEntryUnlocked(parentDir, overWrittenEntry, outUnlinkInode);
   }
   else
   {
      *outUnlinkInode = NULL;

      // irrelevant values, just to please the compiler
      unlinkRes = FhgfsOpsErr_SUCCESS;
      unlinkedWasInlined = true;
   }

   /* Now update the ctime (attribChangeTime) of the renamed entry.
    * Only do that for Directory dentry after giving up the DirInodes (fromMutex) lock
    * as dirStore.setAttr() will aquire the InodeDirStore:: lock
    * and the lock order is InodeDirStore:: and then DirInode::  (risk of deadlock) */

   DirEntry* entry = parentDir->dirEntryCreateFromFileUnlocked(toName);
   if (likely(entry) ) // entry was just renamed to, so very likely it exists
   {
      EntryInfo entryInfo;
      std::string parentID = parentDir->getID();
      entry->getEntryInfo(parentID, 0, &entryInfo);

      fromMutexLock.unlock();
      setAttrUnlocked(&entryInfo, 0, NULL); /* This will fail if the DirInode is on another
                                             * meta server, but as updating the ctime is not
                                             * a real posix requirement (but filesystems usually
                                             * do it) we simply ignore this issue for now. */

      SAFE_DELETE(entry);
   }
   else
      fromMutexLock.unlock();

   safeLock.unlock();

   // unlink later must be called after releasing all locks

   if (overWrittenEntry)
   {
      if (unlinkRes == FhgfsOpsErr_INUSE)
      {
         unlinkRes = unlinkInodeLater(&unlinkEntryInfo, unlinkedWasInlined );
         if (unlinkRes == FhgfsOpsErr_AGAIN)
         {
            unlinkRes = unlinkOverwrittenEntry(parentDir, overWrittenEntry, outUnlinkInode);
         }
      }

      if (unlinkRes != FhgfsOpsErr_SUCCESS && unlinkRes != FhgfsOpsErr_PATHNOTEXISTS)
      {
         LogContext(logContext).logErr("Failed to unlink overwritten entry:"
            " FileName: "      + toName                         +
            " ParentEntryID: " + parentDir->getID()             +
            " entryID: "       + overWrittenEntry->getEntryID() +
            " Error: "         + FhgfsOpsErrTk::toErrString(unlinkRes) );


         // TODO: Restore the dentry
      }
   }

   SAFE_DELETE(overWrittenEntry);

   return retVal;
}

/**
 * Unlink an overwritten dentry. From this dentry either the #fsid# entry or its inode is left.
 *
 * Locking:
 *   We lock everything ourself
 */
FhgfsOpsErr MetaStore::unlinkOverwrittenEntry(DirInode* parentDir,
   DirEntry* overWrittenEntry, FileInode** outInode)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K
   SafeRWLock parentLock(&parentDir->rwlock, SafeRWLock_WRITE);

   FhgfsOpsErr unlinkRes = unlinkOverwrittenEntryUnlocked(parentDir, overWrittenEntry, outInode);

   parentLock.unlock();
   safeLock.unlock();

   return unlinkRes;
}

/**
 * See unlinkOverwrittenEntry() for details
 *
 * Locking:
 *    MetaStore rwlock: Read-lock
 *    parentDir       : Write-lock
 */
FhgfsOpsErr MetaStore::unlinkOverwrittenEntryUnlocked(DirInode* parentDir,
   DirEntry* overWrittenEntry, FileInode** outInode)
{
   FhgfsOpsErr unlinkRes;

   if (overWrittenEntry->getIsInodeInlined() )
   {
      /* We advise the calling code not to try to delete the entryName dentry,
       * as renameEntryUnlocked() already did that */
      unlinkRes = unlinkDirEntryWithInlinedInodeUnlocked("", parentDir, overWrittenEntry,
         DirEntry_UNLINK_ID, outInode);
   }
   else
   {
      // And also do not try to delete the dir-entry-by-name here.
      unlinkRes = unlinkDentryAndInodeUnlocked("", parentDir, overWrittenEntry,
         DirEntry_UNLINK_ID, outInode);
   }

   return unlinkRes;
}

/**
 * Perform the rename action here.
 *
 * In constrast to the moving...()-methods, this method performs a simple rename of an entry,
 * where no moving is involved.
 *
 * Rules: Files can overwrite existing files, but not existing dirs. Dirs cannot overwrite any
 * existing entry.
 *
 * @param dir needs to write-locked already
 * @param outOverwrittenEntry the caller is responsible for the deletion of the local file;
 *    accoring to the rules, this can only be an overwritten file, not a dir; may not be NULL.
 *    Also, we only overwrite the entryName dentry, but not the ID dentry.
 *
 * Note: MetaStore is ReadLocked, dir is WriteLocked
 */
FhgfsOpsErr MetaStore::performRenameEntryInSameDir(DirInode* dir, std::string fromName,
   std::string toName, DirEntry** outOverwrittenEntry)
{
   *outOverwrittenEntry = NULL;

   FhgfsOpsErr retVal;

   // load DirInode on demand if required, we need it now
   bool loadSuccess = dir->loadIfNotLoadedUnlocked();
   if (!loadSuccess)
      return FhgfsOpsErr_PATHNOTEXISTS;

   // of the file being renamed
   DirEntry* fromEntry  = dir->dirEntryCreateFromFileUnlocked(fromName);
   if (!fromEntry)
   {
      return FhgfsOpsErr_PATHNOTEXISTS;
   }

   EntryInfo fromEntryInfo;
   std::string parentEntryID = dir->getID();
   fromEntry->getEntryInfo(parentEntryID, 0, &fromEntryInfo);

   // reference the inode
   FileInode* fromFileInode = NULL;
   // DirInode*  fromDirInode  = NULL;
   if (DirEntryType_ISDIR(fromEntryInfo.getEntryType() ) )
   {
      // TODO, exclusive lock
   }
   else
   {
      fromFileInode = referenceFileUnlocked(dir, &fromEntryInfo);
      if (!fromFileInode)
      {
         /* Note: The inode might be exclusively locked and a remote rename op might be in progress.
          *       If that fails we should actually continue with our rename. That will be solved
          *       in the future by using hardlinks for remote renaming. */
         return FhgfsOpsErr_PATHNOTEXISTS;
      }
   }

   DirEntry* overWriteEntry = dir->dirEntryCreateFromFileUnlocked(toName);
   if (overWriteEntry)
   {
      // sanity checks if we really shall overwrite the entry

      std::string parentID = dir->getID();

      EntryInfo fromEntryInfo;
      fromEntry->getEntryInfo(parentID , 0, &fromEntryInfo);

      EntryInfo overWriteEntryInfo;
      overWriteEntry->getEntryInfo(parentID, 0, &overWriteEntryInfo);

      bool isSameInode;
      retVal = checkRenameOverwrite(&fromEntryInfo, &overWriteEntryInfo, isSameInode);

      if (isSameInode)
      {
         delete(overWriteEntry);
         overWriteEntry = NULL;
         goto out; // nothing to do then, rename request will be silently ignored
      }

      if (retVal != FhgfsOpsErr_SUCCESS)
         goto out; // not allowed for some reasons, return it to the user
   }

   // eventually rename here
   retVal = dir->renameDirEntryUnlocked(fromName, toName, overWriteEntry);

   /* Note: If rename faild and and an existing toName was to be overwritten, we don't need to care
    *       about it, the underlying file system has to handle it. */

   /* Note2: Do not decrease directory link count here, even if we overwrote an entry. We will do
    *        that later on in common unlink code, when we going to unlink the entry from
    *        the #fsIDs# dir.
    */

   if (fromFileInode)
      releaseFileUnlocked(dir, fromFileInode);
   else
   {
      // TODO dir
   }


out:

   if (retVal == FhgfsOpsErr_SUCCESS)
      *outOverwrittenEntry = overWriteEntry;
   else
      SAFE_DELETE(overWriteEntry);

   SAFE_DELETE(fromEntry); // always exists when we are here

   return retVal;
}

/**
 * Check if overwriting an entry on rename is allowed.
 */
FhgfsOpsErr MetaStore::checkRenameOverwrite(EntryInfo* fromEntry, EntryInfo* overWriteEntry,
   bool& outIsSameInode)
{
   outIsSameInode = false;

   // check if we are going to rename to a dentry with the same inode
  if (fromEntry->getEntryID() == overWriteEntry->getEntryID() )
   { // According to posix we must not do anything and return success.

     outIsSameInode = true;
     return FhgfsOpsErr_SUCCESS;
   }

  if (overWriteEntry->getEntryType() == DirEntryType_DIRECTORY)
  {
     return FhgfsOpsErr_EXISTS;
  }

  /* TODO: We should allow this if overWriteEntry->getEntryType() == DirEntryType_DIRECTORY
   *       and overWriteEntry is empty.
   */
  if (fromEntry->getEntryType() == DirEntryType_DIRECTORY)
  {
     return FhgfsOpsErr_EXISTS;
  }

  return FhgfsOpsErr_SUCCESS;
}

/**
 * Create a new file on this (remote) meta-server. This is the 'toFile' on a rename() client call.
 *
 * Note: Replaces existing entry.
 *
 * @param buf serialized inode object
 * @param outUnlinkedInode the unlinked (owned) file (in case a file was overwritten
 * by the move operation); the caller is responsible for the deletion of the local file and the
 * corresponding object; may not be NULL
 */
FhgfsOpsErr MetaStore::moveRemoteFileInsert(EntryInfo* fromFileInfo, std::string toParentID,
   std::string newEntryName, const char* buf, FileInode** outUnlinkedInode, EntryInfo& newFileInfo)
{
   // note: we do not allow newEntry to be a file if the old entry was a directory (and vice versa)
   const char* logContext = "rename(): Insert remote entry";

   FhgfsOpsErr retVal = FhgfsOpsErr_INTERNAL;

   *outUnlinkedInode = NULL;

   SafeRWLock safeMetaStoreLock(&rwlock, SafeRWLock_READ); // L O C K

   DirInode* toParent = referenceDirUnlocked(toParentID, true);
   if(!toParent)
   {
      retVal = FhgfsOpsErr_PATHNOTEXISTS;
      safeMetaStoreLock.unlock(); // U N L O C K
      return retVal;
   }

   // toParent exists
   SafeRWLock toParentMutexLock(&toParent->rwlock, SafeRWLock_WRITE); // L O C K ( T O )


   DirEntry* overWrittenEntry = toParent->dirEntryCreateFromFileUnlocked(newEntryName);
   if (overWrittenEntry)
   {
      EntryInfo overWriteInfo;
      std::string parentID = overWrittenEntry->getID();
      overWrittenEntry->getEntryInfo(parentID, 0, &overWriteInfo);
      bool isSameInode;

      FhgfsOpsErr checkRes = checkRenameOverwrite(fromFileInfo, &overWriteInfo, isSameInode);

      if ((checkRes != FhgfsOpsErr_SUCCESS)  || ((checkRes == FhgfsOpsErr_SUCCESS) && isSameInode) )
      {
         retVal = checkRes;
         goto outUnlock;
      }

      // only unlink the dir-entry-name here, so we can still restore it from dir-entry-id
      FhgfsOpsErr unlinkRes = toParent->unlinkDirEntryUnlocked(newEntryName, overWrittenEntry,
         DirEntry_UNLINK_FILENAME);
      if (unlikely(unlinkRes != FhgfsOpsErr_SUCCESS) )
      {
         if (unlikely (unlinkRes == FhgfsOpsErr_PATHNOTEXISTS) )
            LogContext(logContext).log(Log_WARNING, "Unexpectedly failed to unlink file: " +
               toParent->entries.getDirEntryPathUnlocked() + newEntryName + ". ");
         else
         {
            LogContext(logContext).logErr("Failed to unlink existing file. Aborting rename().");
            retVal = unlinkRes;
            goto outUnlock;
         }
      }
   }

   { // create new dirEntry with inlined inode
      FileInode* inode = new FileInode(); // the deserialized inode
      bool deserializeRes = inode->deserializeMetaData(buf);
      if (deserializeRes == false)
      {
         LogContext("File rename").logErr("Bug: Deserialization of remote buffer failed. Are all "
            "meta servers running with the same version?" );
         retVal = FhgfsOpsErr_INTERNAL;

         delete inode;
         goto outUnlock;
      }

      // destructs inode
      retVal = mkMetaFileUnlocked(toParent, newEntryName, fromFileInfo->getEntryType(), inode);
   }

   if (retVal == FhgfsOpsErr_SUCCESS)
   {
      if (!toParent->entries.getFileEntryInfo(newEntryName, newFileInfo))
         retVal = FhgfsOpsErr_INTERNAL;
   }

   if (overWrittenEntry && retVal == FhgfsOpsErr_SUCCESS)
   { // unlink the overwritten entry, will unlock, release and return
      bool unlinkedWasInlined = overWrittenEntry->getIsInodeInlined();

      FhgfsOpsErr unlinkRes = unlinkOverwrittenEntryUnlocked(toParent, overWrittenEntry,
         outUnlinkedInode);

      EntryInfo unlinkEntryInfo;
      overWrittenEntry->getEntryInfo(toParentID, 0, &unlinkEntryInfo);

      // unlock everything here, but do not release toParent yet.
      toParentMutexLock.unlock(); // U N L O C K ( T O )
      safeMetaStoreLock.unlock();

      // unlinkInodeLater() requires that everything was unlocked!
      if (unlinkRes == FhgfsOpsErr_INUSE)
      {
         unlinkRes = unlinkInodeLater(&unlinkEntryInfo, unlinkedWasInlined );
         if (unlinkRes == FhgfsOpsErr_AGAIN)
            unlinkRes = unlinkOverwrittenEntry(toParent, overWrittenEntry, outUnlinkedInode);

         if (unlinkRes != FhgfsOpsErr_SUCCESS && unlinkRes != FhgfsOpsErr_PATHNOTEXISTS)
            LogContext(logContext).logErr("Failed to unlink overwritten entry:"
               " FileName: "      + newEntryName                   +
               " ParentEntryID: " + toParent->getID()              +
               " entryID: "       + overWrittenEntry->getEntryID() +
               " Error: "         + FhgfsOpsErrTk::toErrString(unlinkRes) );
      }

      delete overWrittenEntry;

      releaseDir(toParentID);

      return retVal;
   }
   else
   if (overWrittenEntry)
   {
      // TODO: Restore the overwritten entry
   }

outUnlock:

   toParentMutexLock.unlock(); // U N L O C K ( T O )
   dirStore.releaseDir(toParent->getID() );

   safeMetaStoreLock.unlock();

   SAFE_DELETE(overWrittenEntry);

   return retVal;
}


/**
 * Copies (serializes) the original file object to a buffer.
 *
 * Note: This works by inserting a temporary placeholder and returning the original, so remember to
 * call movingComplete()
 *
 * @param buf target buffer for serialization
 * @param bufLen must be at least META_SERBUF_SIZE
 */
FhgfsOpsErr MetaStore::moveRemoteFileBegin(DirInode* dir, EntryInfo* entryInfo,
   char* buf, size_t bufLen, size_t* outUsedBufLen)
{
   FhgfsOpsErr retVal = FhgfsOpsErr_INTERNAL;

   SafeRWLock safeLock(&this->rwlock, SafeRWLock_READ); // L O C K

   // lock the dir to make sure no renameInSameDir is going on
   SafeRWLock safeDirLock(&dir->rwlock, SafeRWLock_READ);

   if (this->fileStore.isInStore(entryInfo->getEntryID() ) )
      retVal = this->fileStore.moveRemoteBegin(entryInfo, buf, bufLen, outUsedBufLen);
   else
   {
      retVal = dir->fileStore.moveRemoteBegin(entryInfo, buf, bufLen, outUsedBufLen);
   }

   safeDirLock.unlock();
   safeLock.unlock(); // U N L O C K

   return retVal;
}

void MetaStore::moveRemoteFileComplete(DirInode* dir, std::string entryID)
{
   SafeRWLock safeLock(&this->rwlock, SafeRWLock_WRITE); // L O C K

   if (this->fileStore.isInStore(entryID) )
      this->fileStore.moveRemoteComplete(entryID);
   else
   {
      SafeRWLock safeDirLock(&dir->rwlock, SafeRWLock_READ);

      dir->fileStore.moveRemoteComplete(entryID);

      safeDirLock.unlock();

   }

   safeLock.unlock(); // U N L O C K
}



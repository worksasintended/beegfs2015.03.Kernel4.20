#include <app/App.h>
#include <app/log/Logger.h>
#include <common/threading/Mutex.h>
#include <common/Common.h>
#include <common/toolkit/Time.h>
#include <net/filesystem/FhgfsOpsRemoting.h>
#include <filesystem/FsDirInfo.h>
#include <filesystem/FsFileInfo.h>
#include <toolkit/NoAllocBufferStore.h>
#include <common/toolkit/MetadataTk.h>
#include <common/Common.h>
#include "FhgfsOps_versions.h"
#include "FhgfsOpsInode.h"
#include "FhgfsOpsDir.h"
#include "FhgfsOpsSuper.h"
#include "FhgfsOpsHelper.h"


#include <linux/namei.h>

static int __FhgfsOps_revalidateIntent(struct dentry* parentDentry, struct dentry* dentry);

struct dentry_operations fhgfs_dentry_ops =
{
   .d_revalidate   = FhgfsOps_revalidateIntent,
   .d_delete       = FhgfsOps_deleteDentry,
};

/**
 * Called when the dcache has a lookup hit (and wants to know whether the cache data
 * is still valid).
 *
 * @return value is quasi-boolean: 0 if entry invalid, 1 if still valid (no other return values
 * allowed).
 */
#ifndef KERNEL_HAS_ATOMIC_OPEN
int FhgfsOps_revalidateIntent(struct dentry* dentry, struct nameidata* nameidata)
#else
int FhgfsOps_revalidateIntent(struct dentry* dentry, unsigned flags)
#endif // LINUX_VERSION_CODE
{
   App* app;
   Logger* log;
   const char* logContext;

   int isValid = 0; // quasi-boolean (return value)
   struct dentry* parentDentry;
   struct inode* parentInode;
   struct inode* inode;

   #ifndef KERNEL_HAS_ATOMIC_OPEN
      unsigned flags = nameidata ? nameidata->flags : 0;

      IGNORE_UNUSED_VARIABLE(flags);
   #endif // LINUX_VERSION_CODE


   #ifdef LOOKUP_RCU
      /* note: 2.6.38 introduced rcu-walk mode, which is inappropriate for us, because we need the
         parentDentry and need to sleep for communication. ECHILD below tells vfs to call this again
         in old ref-walk mode. (see Documentation/filesystems/vfs.txt:d_revalidate) */

      if(flags & LOOKUP_RCU)
         return -ECHILD;
   #endif // LINUX_VERSION_CODE

   inode = dentry->d_inode;

   // access to parentDentry and inode needs to live below the rcu check.

   app = FhgfsOps_getApp(dentry->d_sb);
   log = App_getLogger(app);
   logContext = "FhgfsOps_revalidateIntent";

   parentDentry = dget_parent(dentry);
   parentInode = parentDentry->d_inode;

   if(unlikely(Logger_getLogLevel(log) >= 5) )
      FhgfsOpsHelper_logOp(Log_SPAM, app, dentry, inode, logContext);

   if(!inode || !parentInode || is_bad_inode(inode) )
   {
      if(inode && S_ISDIR(inode->i_mode) )
      {
         if(have_submounts(dentry) )
            goto cleanup_put_parent;

         shrink_dcache_parent(dentry);
      }

      d_drop(dentry);
      goto cleanup_put_parent;
   }

   // active dentry => remote-stat and local-compare

   isValid = __FhgfsOps_revalidateIntent(parentDentry, dentry );

cleanup_put_parent:
   // clean-up
   dput(parentDentry);

#ifdef BEEGFS_DEBUG
   {
      const char* isValidStr = StringTk_boolToStr(isValid);
      LOG_DEBUG_FORMATTED(log, 5, logContext, "'%s': isValid: %s",
         dentry->d_name.name, isValidStr );
      os_kfree(isValidStr);
   }
#endif // BEEGFS_DEBUG

   return isValid;
}

/*
 * sub function of FhgfsOps_revalidateIntent(), supposed to be inlined, as we resolve several
 * pointers two times, in this function and also already in the caller
 *
 * @return value is quasi-boolean: 0 if entry invalid, 1 if still valid (no other return values
 * allowed).
 */
int __FhgfsOps_revalidateIntent(struct dentry* parentDentry, struct dentry* dentry)
{
   const char* logContext = "__FhgfsOps_revalidateIntent";
   App* app = FhgfsOps_getApp(dentry->d_sb);
   Logger* log = App_getLogger(app);
   Config* cfg = App_getConfig(app);

   const char* entryName = dentry->d_name.name;

   fhgfs_stat fhgfsStat;
   fhgfs_stat* fhgfsStatPtr;

   struct inode* parentInode = parentDentry->d_inode;
   FhgfsInode* parentFhgfsInode = BEEGFS_INODE(parentInode);

   struct inode* inode = dentry->d_inode;
   FhgfsInode* fhgfsInode = BEEGFS_INODE(inode);

   fhgfs_bool cacheValid = FhgfsInode_isCacheValid(fhgfsInode, inode->i_mode, cfg);
   int isValid = 0; // quasi-boolean (return value)
   fhgfs_bool needDrop = fhgfs_false;

   FhgfsIsizeHints iSizeHints;


   FhgfsOpsHelper_logOp(Log_SPAM, app, dentry, inode, logContext);


   if (cacheValid)
   {
      isValid = 1;
      return isValid;
   }

   if(IS_ROOT(dentry) )
      fhgfsStatPtr = NULL;
   else
   { // any file or directory except our mount root
      EntryInfo* parentInfo;
      EntryInfo* entryInfo;

      FhgfsOpsErr remotingRes;
      LookupIntentInfoIn inInfo; // input data for combo-request
      LookupIntentInfoOut outInfo; // result data of combo-request

      FhgfsInode_initIsizeHints(fhgfsInode, &iSizeHints);

      FhgfsInode_entryInfoReadLock(parentFhgfsInode); // LOCK parentInfo
      FhgfsInode_entryInfoReadLock(fhgfsInode);       // LOCK EntryInfo

      parentInfo = FhgfsInode_getEntryInfo(parentFhgfsInode);
      entryInfo  = FhgfsInode_getEntryInfo(fhgfsInode);

      LookupIntentInfoIn_init(&inInfo, parentInfo, entryName);
      LookupIntentInfoIn_addEntryInfo(&inInfo, entryInfo);

      LookupIntentInfoOut_prepare(&outInfo, NULL, &fhgfsStat);

      remotingRes = FhgfsOpsRemoting_lookupIntent(app, &inInfo, &outInfo);

      FhgfsInode_entryInfoReadUnlock(fhgfsInode);       // UNLOCK EntryInfo
      FhgfsInode_entryInfoReadUnlock(parentFhgfsInode); // UNLOCK parentInfo

      if (unlikely(remotingRes != FhgfsOpsErr_SUCCESS) )
      {
         needDrop = fhgfs_true;
         goto out;
      }

      if (outInfo.revalidateRes != FhgfsOpsErr_SUCCESS)
      {
         if(unlikely(!(outInfo.responseFlags & LOOKUPINTENTRESPMSG_FLAG_REVALIDATE) ) )
            Logger_logErrFormatted(log, logContext, "Unexpected revalidate info missing: %s",
               entryInfo->fileName);

         needDrop = fhgfs_true;
         goto out;
      }

      // we only need to update entryInfo flags
      EntryInfo_setFlags(entryInfo, outInfo.revalidateUpdatedFlags);

      // check the stat result here and set fhgfsStatPtr accordingly
      if(outInfo.statRes == FhgfsOpsErr_SUCCESS)
         fhgfsStatPtr = &fhgfsStat; // successful, so we can use existing stat values
      else
      if(outInfo.statRes == FhgfsOpsErr_NOTOWNER)
         fhgfsStatPtr = NULL; // stat values not available
      else
      {
         if(unlikely(!(outInfo.responseFlags & LOOKUPINTENTRESPMSG_FLAG_STAT) ) )
            Logger_logErrFormatted(log, logContext, "Unexpected stat info missing: %s",
               entryInfo->fileName);

         // now its getting difficult as there is an unexpected error
         needDrop = fhgfs_true;
         goto out;
      }
   }

   if (!__FhgfsOps_refreshInode(app, inode, fhgfsStatPtr, &iSizeHints) )
      isValid = 1;
   else
      isValid = 0;

out:
   if (needDrop)
      d_drop(dentry);

   return isValid;
}

/**
 * This is called from dput() when d_count is going to 0 and dput() wants to know from us whether
 * not it should delete the dentry.
 *
 * @return !=0 to delete dentry, 0 to keep it
 */
#ifndef KERNEL_HAS_D_DELETE_CONST_ARG
   int FhgfsOps_deleteDentry(struct dentry* dentry)
#else
   int FhgfsOps_deleteDentry(const struct dentry* dentry)
#endif // LINUX_VERSION_CODE
{
   int shouldBeDeleted = 0; // quasi-boolean (return value)
   struct inode* inode = dentry->d_inode;

   if(!inode)
      shouldBeDeleted = 1; // no inode attached => no need to keep dentry
   else
   {
      if(is_bad_inode(inode) )
         shouldBeDeleted = 1; // inode marked as bad => no need to keep dentry
   }

   return shouldBeDeleted;
}

/*
 * Constructs the path from the root dentry (of the mount-point) to an arbitrary hashed dentry.
 *
 * Note: Acquires a buf from the pathBufStore that must be released by the caller.
 * Note: This is safe for paths larger than bufSize, but returns -ENAMETOOLONG in that case
 * NOTE: Do NOT call two times in the same thread, as it might deadlock if multiple threads
 *       try to aquire a buffer. The thread that takes the buffer two times might never finish,
 *       if not sufficient buffers are available. If multiple threads take multiple buffers in 
 *       parallel an infinity deadlock of the filesystem will happen.
 *
 * @outBuf the buf that must be returned to the pathBufStore of the app
 * @return some offset within the outStoreBuf or the linux ERR_PTR(errorCode) (already negative) and
 * *outStoreBuf will be NULL then
 */
char* __FhgfsOps_pathResolveToStoreBuf(NoAllocBufferStore* bufStore, struct dentry* dentry,
   char** outStoreBuf)
{
   char * path;
   const ssize_t storeBufLen = NoAllocBufferStore_getBufSize(bufStore);
   *outStoreBuf = NoAllocBufferStore_waitForBuf(bufStore);

   path = dentry_path_raw(dentry, *outStoreBuf, storeBufLen);
   if (unlikely (IS_ERR(path) ) )
   {
      NoAllocBufferStore_addBuf(bufStore, *outStoreBuf);
      *outStoreBuf = NULL;
   }

   return path;
}

// currently not needed (and typically hard to use, because we don't have the vfsmount for a
// dentry at hand to pass it as mnt/dentry pair to this method)
///**
// * Retrieve the full path of a dentry from the top root (including the path of the mount point).
// *
// * Note: Allocates a page that must be free_page'd by the caller.
// * Note: unlike the other path resolve method, this version retrieves the full path (including
// * the path to the mount point)
// *
// * @param mnt must be the mount for the dentry (so don't try to just pass root mount or so here)
// * @param outPageBuf the page that must be free_page'd
// * @return points to some location inside the pageBuf
// */
//char* __FhgfsOps_fullpathResolveToPage(struct dentry* dentry, struct vfsmount* mnt,
//   char** outPageBuf)
//{
//   char* resolvedPath;
//
//   *outPageBuf = (char*)__get_free_page(GFP_NOFS);
//
//#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25)
//   {
//      resolvedPath = d_path(dentry, mnt, *outPageBuf, PAGE_SIZE);
//   }
//#else
//   {
//      struct path path =
//      {
//         .mnt = mnt,
//         .dentry = dentry,
//      };
//
//      resolvedPath = d_path(&path, *outPageBuf, PAGE_SIZE);
//   }
//#endif // LINUX_VERSION_CODE
//
//
//   return resolvedPath;
//}


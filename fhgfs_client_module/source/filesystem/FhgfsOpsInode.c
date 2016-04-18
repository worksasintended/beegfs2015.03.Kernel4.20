#include <app/log/Logger.h>
#include <app/App.h>
#include <app/config/Config.h>
#include <common/net/message/storage/lookup/LookupIntentRespMsg.h> // response flags
#include <common/toolkit/vector/StrCpyVec.h>
#include <common/storage/StorageErrors.h>
#include <common/toolkit/StringTk.h>
#include <common/toolkit/MetadataTk.h>
#include <os/OsCompat.h>
#include <os/OsTypeConversion.h>
#include "FhgfsOpsSuper.h"
#include "FhgfsOpsDir.h"
#include "FhgfsOpsInode.h"
#include "FhgfsOpsFile.h"
#include "FhgfsOpsFileNative.h"
#include "FhgfsOpsHelper.h"

#include <linux/namei.h>
#include <linux/backing-dev.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mount.h>
#include <linux/posix_acl.h>
#include <linux/xattr.h>


static struct kmem_cache* FhgfsInodeCache = NULL;

#define FhgfsOpsInode_CACHE_NAME    BEEGFS_MODULE_NAME_STR "_inode_cache" 


static void FhgfsOps_newAttrToInode(struct iattr* iAttr, struct inode* outInode);


/**
 * Find out whether a directory entry exists. Add the dentry and create/associate the
 * corresponding inode in case it does exist.
 *
 * Note: Newer version, superseding the old _lookup() with more efficient messaging based on
 * combined intent requests.
 *
 * @param dentry the thing that we are looking for.
 * @param flags LOOKUP_...
 * @return NULL if we're using the given new dentry, or pointer to another dentry if we
 * find out that it already existed (e.g. from a NFS handle), or ERR_PTR(x) with x being a negative
 * linux error code.
 */
#ifndef KERNEL_HAS_ATOMIC_OPEN
struct dentry* FhgfsOps_lookupIntent(struct inode* parentDir, struct dentry* dentry,
   struct nameidata* nameidata)
#else
struct dentry* FhgfsOps_lookupIntent(struct inode* parentDir, struct dentry* dentry,
   unsigned flags)
#endif // KERNEL_HAS_ATOMIC_OPEN
{
   App* app = FhgfsOps_getApp(dentry->d_sb);
   Logger* log = App_getLogger(app);
   const char* logContext = "FhgfsOps_lookupIntent";

   struct dentry* returnDentry = NULL; // can be NULL or existing dentry or ERR_PTR
   FhgfsOpsErr statRes;
   fhgfs_stat fhgfsStat;
   FhgfsInode* fhgfsParentInode = BEEGFS_INODE(parentDir);
   EntryInfo newEntryInfo;
   const char* entryName = dentry->d_name.name;
   fhgfs_bool freeNewEntryInfo = fhgfs_false;
   struct inode* inode = dentry->d_inode;

   FhgfsIsizeHints iSizeHints;

   if(unlikely(Logger_getLogLevel(log) >= Log_SPAM) )
      FhgfsOpsHelper_logOp(Log_SPAM, app, dentry, inode, logContext);

   if(unlikely(dentry->d_name.len > NAME_MAX) )
      return ERR_PTR(-ENAMETOOLONG);

   /*
    * O_EXCL: optimize away the lookup, but don't hash the dentry. Let
    * the VFS handle the create (also optimizes hard-link calls)
    */
   #ifdef LOOKUP_EXCL
      #if !defined KERNEL_HAS_ATOMIC_OPEN
         if (nameidata && (nameidata->flags & LOOKUP_EXCL))
      #else
         if (flags & LOOKUP_EXCL)
      #endif // !defined KERNEL_HAS_ATOMIC_OPEN
         {
            /* Make it a negative dentry and let lookup-create handle it. It is important to return
             * NULL here and not ERR_PTR(-ENOENT) as with ENOENT dangling symlinks will cause
             * issues. */
            d_instantiate(dentry, NULL);
            return NULL;
         }
   #endif // LOOKUP_EXCL

   /* check if root inode attribs have been fetched already
      (because the kernel doesn't do lookup/revalidate for the root inode) */

   if(unlikely(!FhgfsOps_getIsRootInited(dentry->d_sb) &&
      parentDir->i_ino == BEEGFS_INODE_ROOT_INO) )
   {
      int initRootRes;
      int permRes;

      initRootRes = __FhgfsOps_refreshInode(app, parentDir, NULL, &iSizeHints);
      if(initRootRes)
         return ERR_PTR(initRootRes);

      FhgfsOps_setIsRootInited(dentry->d_sb, fhgfs_true);

      // root permissions might have changed now => recheck permissions

      permRes = os_generic_permission(parentDir, MAY_EXEC);
      if(permRes)
         return ERR_PTR(permRes);
   }


   // retrieve remote stat info for given entry...

   if(unlikely(IS_ROOT(dentry) ) )
   { // root inode is special (though the kernel never actually does a root lookup => unlikely)
      fhgfs_bool isGetSuccess = MetadataTk_getRootEntryInfoCopy(app, &newEntryInfo);

      freeNewEntryInfo = fhgfs_true;

      if (isGetSuccess)
         statRes = FhgfsOpsRemoting_statDirect(app, &newEntryInfo, &fhgfsStat);
      else
         statRes = FhgfsOpsErr_INTERNAL;
   }
   else
   { // just a normal subentry of some dir => lookup entryInfoPtr and stat it
      FhgfsOpsErr remotingRes;
      LookupIntentInfoIn inInfo; // input data for combo-request
      LookupIntentInfoOut outInfo; // result data of combo-request

      FhgfsInode_initIsizeHints(NULL, &iSizeHints);

      FhgfsInode_entryInfoReadLock(fhgfsParentInode); // LOCK EntryInfo

      LookupIntentInfoIn_init(&inInfo, FhgfsInode_getEntryInfo(fhgfsParentInode), entryName);

      LookupIntentInfoOut_prepare(&outInfo, &newEntryInfo, &fhgfsStat);

      remotingRes = FhgfsOpsRemoting_lookupIntent(app, &inInfo, &outInfo);

      FhgfsInode_entryInfoReadUnlock(fhgfsParentInode); // UNLOCK EntryInfo

      if(remotingRes != FhgfsOpsErr_SUCCESS || outInfo.lookupRes != FhgfsOpsErr_SUCCESS)
         statRes = outInfo.lookupRes; // entry not found
      else
      { // lookup successful (entry exists)
         statRes = outInfo.statRes;

         if(unlikely(!(outInfo.responseFlags & LOOKUPINTENTRESPMSG_FLAG_STAT) ) )
         {
            Logger_logErrFormatted(log, logContext, "Unexpected stat info missing: %s",
               entryName);
         }

         freeNewEntryInfo = fhgfs_true;

         // EntryInfo uninitialize already handled
         LookupIntentInfoOut_setEntryInfoPtr(&outInfo, NULL);

         if(statRes == FhgfsOpsErr_NOTOWNER)
         { // metadata not owned by parent/lookup server => need separate stat remoting
            statRes = FhgfsOpsRemoting_statDirect(app, &newEntryInfo, &fhgfsStat);
         }
      }

      LookupIntentInfoOut_uninit(&outInfo);

   }

   // handle result of stat retrieval (e.g. create new inode)...

   if(statRes != FhgfsOpsErr_SUCCESS)
   { // stat error (e.g. entry doesn't exist)

      if(statRes == FhgfsOpsErr_PATHNOTEXISTS)
      {
         /* note: "not exists" does not mean that we return a lookup error. the kernel will check
            after lookup() whether we attached an inode to this dentry and handle it accordingly. */

         #ifndef KERNEL_HAS_S_D_OP
            dentry->d_op = &fhgfs_dentry_ops; // (see below for KERNEL_HAS_S_D_OP comments)
         #endif // KERNEL_HAS_S_D_OP

         d_add(dentry, NULL);
      }
      else
         returnDentry = ERR_PTR(FhgfsOpsErr_toSysErr(statRes) );

   }
   else
   { // entry exists => create inode

      struct kstat kstat;
      struct inode* newInode;

      OsTypeConv_kstatFhgfsToOs(&fhgfsStat, &kstat);

      kstat.ino = FhgfsInode_generateInodeID(dentry->d_sb,
         newEntryInfo.entryID, strlen(newEntryInfo.entryID) );

      newInode = __FhgfsOps_newInode(parentDir->i_sb, &kstat, 0, &newEntryInfo, &iSizeHints);

      freeNewEntryInfo = fhgfs_false; // newEntryInfo now owned or freed by _newInode()

      if(unlikely(!newInode || IS_ERR(newInode) ) )
         returnDentry = IS_ERR(newInode) ? ERR_PTR(PTR_ERR(newInode) ) : ERR_PTR(-EACCES);
      else
      { // new inode created

         #ifndef KERNEL_HAS_S_D_OP
            /* per-dentry d_ops are deprecated as of linux 2.6.38 (commit c8aebb0c9f8c7471643d5f).
               they are now set on the superblock. (individual per-dentry d_ops could still be set
               with d_set_d_op(), but are not required for us.) */

            dentry->d_op = &fhgfs_dentry_ops;
         #endif // KERNEL_HAS_S_D_OP

         if (S_ISDIR(newInode->i_mode) )
            returnDentry = d_materialise_unique(dentry, newInode);
         else
         {
            returnDentry = d_splice_alias(newInode, dentry); /* (d_splice_alias() also replaces a
               disconnected dentry that was created from a nfs handle) */
         }
      }

   }

   // clean-up
   if(freeNewEntryInfo)
      EntryInfo_uninit(&newEntryInfo);

   return returnDentry;
}


int FhgfsOps_getattr(struct vfsmount* mnt, struct dentry* dentry, struct kstat* kstat)
{
   App* app = FhgfsOps_getApp(dentry->d_sb);
   Config* cfg = App_getConfig(app);
   const char* logContext = "FhgfsOps_getattr";

   /* Note: Do not use IS_ROOT(dentry) as this does not work for NFS file dentries
    *       (disconnected dentries), i.e. after a server reboot */
   fhgfs_bool isRoot = (dentry == mnt->mnt_root) ? fhgfs_true : fhgfs_false;

   int retVal = 0;
   struct inode* inode = dentry->d_inode;
   FhgfsInode* fhgfsInode = BEEGFS_INODE(inode);

   fhgfs_bool cacheValid = FhgfsInode_isCacheValid(fhgfsInode, inode->i_mode, cfg);

   fhgfs_bool refreshOnGetAttr = Config_getTuneRefreshOnGetAttr(cfg);

   FhgfsIsizeHints iSizeHints;


   FhgfsOpsHelper_logOp(5, app, dentry, inode, logContext);

   /* note: we assumed that we could live without refreshInode() here, because either lookup() or
      revalidate() was called before. But that assumption is wrong when a open file is fstat'ed
      (which is relevant for e.g. "tail -f"), so we only use this optimization for closed files.
      The root dir inode is also an important exception here, because it is not being revalidated
      by the kernel.

      note on dirs: we could always refresh dirs here because of caching and tools like "find"
      relying on an updated link count, but while a user is creating subdirs, the stat from "find"
      can never be up-to-date, so that would also be quite useless. */

   if(!cacheValid &&
      (isRoot || FhgfsInode_getIsFileOpen(fhgfsInode) || refreshOnGetAttr) )
   {
      retVal = __FhgfsOps_refreshInode(app, inode, NULL, &iSizeHints);
   }

   if(!retVal)
   {
      generic_fillattr(inode, kstat);

      kstat->blksize = Config_getTuneInodeBlockSize(cfg);

      if(isRoot)
         kstat->ino = BEEGFS_INODE_ROOT_INO; // root => assign the constant root inode number
      else
      if (is_32bit_api() && kstat->ino > UINT_MAX)
      {  // 32-bit applications cannot handle 64bit inodes and would fail with EOVERFLOW
         kstat->ino = kstat->ino >> 32;
      }
   }

   // clean-up

   // printk_fhgfs_debug(KERN_INFO, "%s: retVal: %d\n", __func__, retVal);

   return retVal;
}

/**
 * Get a list of extended attributes of a file, copy the list into a buffer as null-separated string
 * list; or compute the size of the buffer required.
 * @param value Pointer to the buffer. NULL to compute the size of the buffer required.
 * @param size Size of the buffer.
 * @return Negative error number on failure, number of bytes used / required on success.
 */
ssize_t FhgfsOps_listxattr(struct dentry* dentry, char* value, size_t size)
{
   App* app = FhgfsOps_getApp(dentry->d_sb);
   Config* cfg = App_getConfig(app);
   FhgfsOpsErr remotingRes;
   ssize_t resSize;

   FhgfsInode* fhgfsInode = BEEGFS_INODE(dentry->d_inode);

   fhgfs_bool cacheValid = FhgfsInode_isCacheValid(fhgfsInode, dentry->d_inode->i_mode, cfg);
   if (!cacheValid)
   {
      FhgfsIsizeHints iSizeHints;
      remotingRes = __FhgfsOps_refreshInode(app, dentry->d_inode, NULL, &iSizeHints);

      if (remotingRes != FhgfsOpsErr_SUCCESS)
         return remotingRes;
   }

   FhgfsOpsHelper_logOpDebug(app, dentry, NULL, __func__, "(size: %u)", size);

   // "Just get the size, don't return any value" is signaled by value=NULL. Since we only send
   // the "size" parameter to the server, we make sure that size is set to 0 in that case.
   if(!value)
      size = 0;

   remotingRes = FhgfsOpsRemoting_listXAttr(app, FhgfsInode_getEntryInfo(fhgfsInode), value, size,
         &resSize);

   if(remotingRes != FhgfsOpsErr_SUCCESS)
      resSize = FhgfsOpsErr_toSysErr(remotingRes);

   return resSize;
}

/**
 * Get an extended attribute of a file, copy it into a buffer; or compute the size of the buffer
 * required.
 * @param value Pointer to the buffer. NULL to compute the size of the buffer required.
 * @param size Size of the buffer.
 * @return Negative error number on failure, or the number of bytes used / required on success.
 */
#ifdef KERNEL_HAS_DENTRY_XATTR_HANDLER
ssize_t FhgfsOps_getxattr(struct dentry* dentry, const char* name, void* value, size_t size)
{
   App* app = FhgfsOps_getApp(dentry->d_sb);
   Config* cfg = App_getConfig(app);
   FhgfsInode* fhgfsInode = BEEGFS_INODE(dentry->d_inode);
   fhgfs_bool cacheValid = FhgfsInode_isCacheValid(fhgfsInode, dentry->d_inode->i_mode, cfg);
   struct inode* inode = dentry->d_inode;
#else
ssize_t FhgfsOps_getxattr(struct inode* inode, const char* name, void* value, size_t size)
{
   App* app = FhgfsOps_getApp(inode->i_sb);
   Config* cfg = App_getConfig(app);
   FhgfsInode* fhgfsInode = BEEGFS_INODE(inode);
   fhgfs_bool cacheValid = FhgfsInode_isCacheValid(fhgfsInode, inode->i_mode, cfg);
#endif // KERNEL_HAS_DENTRY_XATTR_HANDLER

   FhgfsOpsErr remotingRes;
   ssize_t resSize;

   if (!cacheValid)
   {
      FhgfsIsizeHints iSizeHints;
      remotingRes = __FhgfsOps_refreshInode(app, inode, NULL, &iSizeHints);

      if (remotingRes != FhgfsOpsErr_SUCCESS)
         return remotingRes;
   }

   // "Just get the size, don't return any value" is signaled by value=NULL. Since we only send
   // the "size" parameter to the server, we make sure that size is set to 0 in that case.
   if(!value)
      size = 0;

   remotingRes = FhgfsOpsRemoting_getXAttr(app, FhgfsInode_getEntryInfo(fhgfsInode), name, value,
      size, &resSize);

   if(remotingRes != FhgfsOpsErr_SUCCESS)
      resSize = FhgfsOpsErr_toSysErr(remotingRes);

   return resSize;
}

/**
 * Remove an extended attribute from a file.
 */
int FhgfsOps_removexattr(struct dentry* dentry, const char* name)
{
   App* app = FhgfsOps_getApp(dentry->d_sb);
   Config* cfg = App_getConfig(app);
   FhgfsOpsErr remotingRes;

   FhgfsInode* fhgfsInode = BEEGFS_INODE(dentry->d_inode);

   fhgfs_bool cacheValid = FhgfsInode_isCacheValid(fhgfsInode, dentry->d_inode->i_mode, cfg);
   if (!cacheValid)
   {
      FhgfsIsizeHints iSizeHints;
      remotingRes = __FhgfsOps_refreshInode(app, dentry->d_inode, NULL, &iSizeHints);

      if (remotingRes != FhgfsOpsErr_SUCCESS)
         return remotingRes;
   }

   FhgfsOpsHelper_logOpDebug(app, dentry, NULL, __func__, "(name: %s)", name);

   remotingRes = FhgfsOpsRemoting_removeXAttr(app, FhgfsInode_getEntryInfo(fhgfsInode), name);

   if(remotingRes != FhgfsOpsErr_SUCCESS)
      return FhgfsOpsErr_toSysErr(remotingRes);

   return 0;
}

/**
 * Set an extended attribute for a file.
 */
#ifdef KERNEL_HAS_DENTRY_XATTR_HANDLER
int FhgfsOps_setxattr(struct dentry* dentry, const char* name, const void* value, size_t size,
      int flags)
{
   App* app = FhgfsOps_getApp(dentry->d_sb);
   Config* cfg = App_getConfig(app);
   FhgfsInode* fhgfsInode = BEEGFS_INODE(dentry->d_inode);
   fhgfs_bool cacheValid = FhgfsInode_isCacheValid(fhgfsInode, dentry->d_inode->i_mode, cfg);
   struct inode* inode = dentry->d_inode;
#else
int FhgfsOps_setxattr(struct inode* inode, const char* name, const void* value, size_t size,
      int flags)
{
   App* app = FhgfsOps_getApp(inode->i_sb);
   Config* cfg = App_getConfig(app);
   FhgfsInode* fhgfsInode = BEEGFS_INODE(inode);
   fhgfs_bool cacheValid = FhgfsInode_isCacheValid(fhgfsInode, inode->i_mode, cfg);
#endif // KERNEL_HAS_DENTRY_XATTR_HANDLER

   FhgfsOpsErr remotingRes;

   if (!cacheValid)
   {
      FhgfsIsizeHints iSizeHints;
      remotingRes = __FhgfsOps_refreshInode(app, inode, NULL, &iSizeHints);

      if (remotingRes != FhgfsOpsErr_SUCCESS)
         return remotingRes;
   }

   remotingRes = FhgfsOpsRemoting_setXAttr(app, FhgfsInode_getEntryInfo(fhgfsInode),
         name, value, size, flags);

   if(remotingRes != FhgfsOpsErr_SUCCESS)
      return FhgfsOpsErr_toSysErr(remotingRes);

   return 0;
}

#ifdef KERNEL_HAS_POSIX_GET_ACL
struct posix_acl* FhgfsOps_get_acl(struct inode* inode, int type)
{
   App* app = FhgfsOps_getApp(inode->i_sb);
   Config* cfg = App_getConfig(app);
   struct posix_acl* res = NULL;
   char* xAttrName;
   FhgfsOpsErr remotingRes;
   size_t remotingResSize;
   size_t xAttrSize;
   char* xAttrBuf;

   FhgfsInode* fhgfsInode = BEEGFS_INODE(inode);
   EntryInfo* entryInfo = FhgfsInode_getEntryInfo(fhgfsInode);

   fhgfs_bool cacheValid = FhgfsInode_isCacheValid(fhgfsInode, inode->i_mode, cfg);
   if (!cacheValid)
   {
      FhgfsIsizeHints iSizeHints;
      remotingRes = __FhgfsOps_refreshInode(app, inode, NULL, &iSizeHints);

      if (remotingRes != FhgfsOpsErr_SUCCESS)
         return ERR_PTR(remotingRes);
   }

   if(type == ACL_TYPE_ACCESS)
      xAttrName = POSIX_ACL_XATTR_ACCESS;
   else if(type == ACL_TYPE_DEFAULT)
      xAttrName = POSIX_ACL_XATTR_DEFAULT;
   else
      return ERR_PTR(-EOPNOTSUPP);

   // read extended attributes from the file
   remotingRes = FhgfsOpsRemoting_getXAttr(app, entryInfo, xAttrName, NULL, 0, &remotingResSize);

   if(remotingRes != FhgfsOpsErr_SUCCESS)
   {
      int sysErr = FhgfsOpsErr_toSysErr(remotingRes);

      if (sysErr == -ENODATA || sysErr == -ENOSYS)
         res = NULL;
      else
         res = ERR_PTR(sysErr);

      return res;
   }

   xAttrBuf = os_kmalloc(remotingResSize);
   if(!xAttrBuf)
      return ERR_PTR(-ENOMEM);

   xAttrSize = remotingResSize;

   remotingRes = FhgfsOpsRemoting_getXAttr(app, entryInfo, xAttrName, xAttrBuf, xAttrSize,
         &remotingResSize);

   if(remotingRes != FhgfsOpsErr_SUCCESS)
   {
      res = ERR_PTR(FhgfsOpsErr_toSysErr(remotingRes) );
      goto cleanup;
   }

   // determine posix_acl from extended attributes
   res = os_posix_acl_from_xattr(xAttrBuf, remotingResSize);

cleanup:
   os_kfree(xAttrBuf);
   return res;
}
#endif // KERNEL_HAS_POSIX_GET_ACL

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0)
int FhgfsOps_set_acl(struct inode* inode, struct posix_acl* acl, int type)
{
   App* app = FhgfsOps_getApp(inode->i_sb);
   Config* cfg = App_getConfig(app);
   int res;
   FhgfsOpsErr remotingRes;
   char* xAttrName;
   int xAttrBufLen;
   void* xAttrBuf;

   FhgfsInode* fhgfsInode = BEEGFS_INODE(inode);
   EntryInfo* entryInfo = FhgfsInode_getEntryInfo(fhgfsInode);

   fhgfs_bool cacheValid = FhgfsInode_isCacheValid(fhgfsInode, inode->i_mode, cfg);
   if (!cacheValid)
   {
      FhgfsIsizeHints iSizeHints;
      res = __FhgfsOps_refreshInode(app, inode, NULL, &iSizeHints);

      if (res != FhgfsOpsErr_SUCCESS)
         return FhgfsOpsErr_toSysErr(res);
   }

   if (type == ACL_TYPE_ACCESS)
      xAttrName = POSIX_ACL_XATTR_ACCESS;
   else if (type == ACL_TYPE_DEFAULT)
      xAttrName = POSIX_ACL_XATTR_DEFAULT;
   else
      return -EOPNOTSUPP;

   // prepare extended attribute - determine size needed for buffer.
   xAttrBufLen = os_posix_acl_to_xattr(acl, NULL, 0);

   if (xAttrBufLen < 0)
      return xAttrBufLen;

   xAttrBuf = os_kmalloc(xAttrBufLen);
   if (!xAttrBuf)
      return -ENOMEM;

   res = os_posix_acl_to_xattr(acl, xAttrBuf, xAttrBufLen);
   if (res != xAttrBufLen)
      goto cleanup;

   remotingRes = FhgfsOpsRemoting_setXAttr(app, entryInfo, xAttrName, xAttrBuf, xAttrBufLen, 0);
   if (remotingRes != FhgfsOpsErr_SUCCESS)
      res = FhgfsOpsErr_toSysErr(remotingRes);
   else
      res = 0;

cleanup:
   os_kfree(xAttrBuf);

   return res;
}
#endif // LINUX_KERNEL_VERSION_CODE

#ifdef KERNEL_HAS_POSIX_GET_ACL
/**
 * Update the ACL of an inode after a chmod
 */
int FhgfsOps_aclChmod(struct iattr* iattr, struct dentry* dentry)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0)
   if (iattr->ia_valid & ATTR_MODE)
      return posix_acl_chmod(dentry->d_inode, iattr->ia_mode);
   else
      return 0;
#else
   struct posix_acl* acl;
   int res;

   void* xAttrBuf;
   int xAttrBufLen;

   if( !(iattr->ia_valid & ATTR_MODE) ) // don't have to do anything if the mode hasn't changed
      return 0;

   acl = FhgfsOps_get_acl(dentry->d_inode, ACL_TYPE_ACCESS);

   if(PTR_ERR(acl) == -ENODATA) // entry doesn't have an ACL. We don't have to do anything.
      return 0;
   else if(IS_ERR(acl) ) // some error occured - pass the error code on to the caller.
      return PTR_ERR(acl);

   // We have an actual ACL for that entry, so we need to update it.
   res = posix_acl_chmod(&acl, GFP_KERNEL, iattr->ia_mode);

   if(res != 0)
      goto cleanup;

   // Set the ACL Xattr.
   xAttrBufLen = os_posix_acl_to_xattr(acl, NULL, 0); // Determine size needed for buffer.

   if(xAttrBufLen < 0)
   {
      res = xAttrBufLen;
      goto cleanup;
   }

   xAttrBuf = os_kmalloc(xAttrBufLen);
   if(!xAttrBuf)
   {
      res = -ENOMEM;
      goto cleanup;
   }

   res = os_posix_acl_to_xattr(acl, xAttrBuf, xAttrBufLen);
   if(res != xAttrBufLen)
      goto buf_cleanup; // if it's not the same as xAttrBufLen, it is -ERANGE - so no need to modify res

   // We call FhgfsOps_setxattr directly instead of using the XAttr handler
   // because the handler would try to chmod again.
   res = FhgfsOps_setxattr(dentry, POSIX_ACL_XATTR_ACCESS, xAttrBuf, xAttrBufLen, 0);

buf_cleanup:
   os_kfree(xAttrBuf);

cleanup:
   posix_acl_release(acl);

   return res;
#endif //  LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0)
}
#endif // KERNEL_HAS_POSIX_GET_ACL

/**
 * @return 0 on success, negative linux error code otherwise
 */
int FhgfsOps_setattr(struct dentry* dentry, struct iattr* iattr)
{
   // note: called for chmod(), chown(), utime(), truncate()
   // note: changed fields can be determined by the iattr->ia_valid flags (ATTR_...)

   App* app = FhgfsOps_getApp(dentry->d_sb);
   Config* cfg = App_getConfig(app);
   Logger* log = App_getLogger(app);
   const char* logContext = "FhgfsOps_setattr";

   int retVal = 0;
   int inodeChangeRes;
   SettableFileAttribs fhgfsAttr;
   int validFhgfsAttribs;
   FhgfsOpsErr setAttrRes = FhgfsOpsErr_SUCCESS;
   struct inode* inode = dentry->d_inode;
   FhgfsInode* fhgfsInode = BEEGFS_INODE(inode);

   if(unlikely(Logger_getLogLevel(log) >= 5) )
   {
      FhgfsOpsHelper_logOpMsg(Log_SPAM, app, dentry, inode, logContext,
         (iattr->ia_valid & ATTR_SIZE) ? "(with trunc)" : "");
   }

   inodeChangeRes = inode_change_ok(inode, iattr);
   if(inodeChangeRes < 0)
      return inodeChangeRes;

   #ifdef KERNEL_HAS_ATTR_OPEN
      /* we do trunc during open message on meta server, so we don't want this redundant trunc
         (and ctime/mtime update) from the kernel */
      if(iattr->ia_valid & ATTR_OPEN)
      {
         if (iattr->ia_valid & ATTR_SIZE)
         {  // the file was already remotely truncate by open, now also truncate it locally
            FhgfsOps_vmtruncate(inode, iattr->ia_size);
         }

         return 0;
      }
   #endif

   if(iattr->ia_valid & ATTR_SIZE)
   { // make sure we only update size of regular files
      if(!S_ISREG(inode->i_mode) )
         iattr->ia_valid &= ~ATTR_SIZE;
   }

   if(S_ISREG(inode->i_mode) )
   { // flush all dirty file data
      filemap_write_and_wait(inode->i_mapping);

      if(Config_getTuneFileCacheTypeNum(cfg) == FILECACHETYPE_Buffered)
         FhgfsOpsHelper_flushCache(app, fhgfsInode, false);
   }

   OsTypeConv_iattrOsToFhgfs(iattr, &fhgfsAttr, &validFhgfsAttribs);

   if(validFhgfsAttribs)
   {
      FhgfsInode_entryInfoReadLock(fhgfsInode); // LOCK EntryInfo

      setAttrRes = FhgfsOpsRemoting_setAttr(app, FhgfsInode_getEntryInfo(fhgfsInode), &fhgfsAttr,
         validFhgfsAttribs);

      FhgfsInode_entryInfoReadUnlock(fhgfsInode); // UNLOCK EntryInfo
   }

   /* todo: truncation should be integrated into Remoting_setAttr to save the time to send an
      extra msg */

   if(setAttrRes == FhgfsOpsErr_SUCCESS)
   {
      if (validFhgfsAttribs)
      {
         FhgfsOps_newAttrToInode(iattr, inode);

#ifdef KERNEL_HAS_POSIX_GET_ACL
         if (Config_getSysACLsEnabled(cfg) )
            FhgfsOps_aclChmod(iattr, dentry);
#endif // KERNEL_HAS_POSIX_GET_ACL
      }

      // all right so far => handle truncation
      if(iattr->ia_valid & ATTR_SIZE)
      {
         FhgfsInode_entryInfoReadLock(fhgfsInode); // LOCK EntryInfo

         setAttrRes = FhgfsOpsRemoting_truncfile(app, FhgfsInode_getEntryInfo(fhgfsInode),
            iattr->ia_size);

         FhgfsInode_entryInfoReadUnlock(fhgfsInode); // UNLOCK EntryInfo

         if(setAttrRes == FhgfsOpsErr_SUCCESS)
         {
            FhgfsOps_vmtruncate(inode, iattr->ia_size);
         }
      }
   }

   if(setAttrRes != FhgfsOpsErr_SUCCESS)
      retVal = FhgfsOpsErr_toSysErr(setAttrRes);
   else
      FhgfsInode_invalidateCache(fhgfsInode);

   return retVal;
}

/**
 * Set inode attributes after a successful setattr
 */
void FhgfsOps_newAttrToInode(struct iattr* iAttr, struct inode* outInode)
{
   spin_lock(&outInode->i_lock);

   if(iAttr->ia_valid & ATTR_MODE)
      outInode->i_mode = iAttr->ia_mode;

   if(iAttr->ia_valid & ATTR_UID)
      outInode->i_uid   = iAttr->ia_uid;

   if(iAttr->ia_valid & ATTR_GID)
      outInode->i_gid    = iAttr->ia_gid;

   if(iAttr->ia_valid & ATTR_MTIME_SET)
   {
      outInode->i_mtime  = iAttr->ia_mtime;
   }
   else
   if(iAttr->ia_valid & ATTR_MTIME)
   { // set mtime to "now"
      TimeAbs now;
      TimeAbs_init(&now);

      outInode->i_mtime.tv_sec = TimeAbs_getTimeval(&now)->tv_sec;

      TimeAbs_uninit(&now);
   }

   if(iAttr->ia_valid & ATTR_ATIME_SET)
   {
      outInode->i_atime             = iAttr->ia_atime;
   }
   else
   if(iAttr->ia_valid & ATTR_ATIME)
   { // set atime to "now"
      TimeAbs now;
      TimeAbs_init(&now);

      outInode->i_atime.tv_sec = TimeAbs_getTimeval(&now)->tv_sec;

      TimeAbs_uninit(&now);
   }

   if(iAttr->ia_valid & ATTR_CTIME)
   {
      TimeAbs now;
      TimeAbs_init(&now);

      outInode->i_ctime.tv_sec = TimeAbs_getTimeval(&now)->tv_sec;

      TimeAbs_uninit(&now);
   }

   spin_unlock(&outInode->i_lock);
}



/**
 * Create directory.
 */
#ifdef KERNEL_HAS_UMODE_T
int FhgfsOps_mkdir(struct inode* dir, struct dentry* dentry, umode_t mode)
#else
int FhgfsOps_mkdir(struct inode* dir, struct dentry* dentry, int mode)
#endif // KERNEL_HAS_UMODE_T
{
   struct super_block* sb = dentry->d_sb;
   App* app = FhgfsOps_getApp(sb);
   Logger* log = App_getLogger(app);
   const char* logContext = "FhgfsOps_mkdir";

   int retVal = 0;
   FhgfsOpsErr mkRes;
   FhgfsInode* fhgfsParentInode = BEEGFS_INODE(dir);
   EntryInfo newEntryInfo;
   const char* entryName = dentry->d_name.name;
   struct CreateInfo createInfo;

   struct inode* inode = dentry->d_inode;

   FhgfsIsizeHints iSizeHints;

   if(unlikely(Logger_getLogLevel(log) >= 5) )
      FhgfsOpsHelper_logOp(5, app, dentry, inode, logContext);

   mode |= S_IFDIR; // just make sure this is a dir

   CreateInfo_init(app, dir, entryName, mode, fhgfs_false, &createInfo);

   FhgfsInode_entryInfoReadLock(fhgfsParentInode); // LOCK EntryInfo

   mkRes = FhgfsOpsRemoting_mkdir(app, FhgfsInode_getEntryInfo(fhgfsParentInode), &createInfo,
      &newEntryInfo);

   FhgfsInode_entryInfoReadUnlock(fhgfsParentInode); // UNLOCK EntryInfo

   if(mkRes != FhgfsOpsErr_SUCCESS)
   {
      retVal = FhgfsOpsErr_toSysErr(mkRes);
   }
   else
   { // remote success => create the local inode
      retVal = __FhgfsOps_instantiateInode(dentry, &newEntryInfo, NULL, &iSizeHints);

      dir->i_ctime = dir->i_mtime = current_fs_time(sb);
   }

   return retVal;
}

int FhgfsOps_rmdir(struct inode* dir, struct dentry* dentry)
{
   struct super_block* sb = dentry->d_sb;
   App* app = FhgfsOps_getApp(sb);
   Logger* log = App_getLogger(app);
   const char* logContext = "FhgfsOps_rmdir";

   int retVal = 0;
   FhgfsOpsErr rmRes;
   FhgfsInode* fhgfsParentInode = BEEGFS_INODE(dir);
   const char* entryName = dentry->d_name.name;
   struct inode* inode = dentry->d_inode;

   if(unlikely(Logger_getLogLevel(log) >= 5) )
      FhgfsOpsHelper_logOp(5, app, dentry, inode, logContext);

   FhgfsInode_entryInfoReadLock(fhgfsParentInode); // LOCK EntryInfo

   rmRes = FhgfsOpsRemoting_rmdir(app, FhgfsInode_getEntryInfo(fhgfsParentInode), entryName);

   FhgfsInode_entryInfoReadUnlock(fhgfsParentInode); // UNLOCK EntryInfo

   if(rmRes != FhgfsOpsErr_SUCCESS)
   {
      retVal = FhgfsOpsErr_toSysErr(rmRes);
   }
   else
   { // remote success
      clear_nlink(dentry->d_inode);

      dir->i_ctime = dir->i_mtime = current_fs_time(sb);
   }


   return retVal;
}

/**
 * Create file based on combined intent message.
 *
 * @param isExclusiveCreate true if this is an exclusive (O_EXCL) create
 */
#if defined KERNEL_HAS_ATOMIC_OPEN
int FhgfsOps_createIntent(struct inode* dir, struct dentry* dentry, umode_t createMode,
   bool isExclusiveCreate)
#elif defined KERNEL_HAS_UMODE_T
int FhgfsOps_createIntent(struct inode* dir, struct dentry* dentry, umode_t createMode,
   struct nameidata* nameidata)
#else
int FhgfsOps_createIntent(struct inode* dir, struct dentry* dentry, int createMode,
   struct nameidata* nameidata)
#endif // KERNEL_HAS_ATOMIC_OPEN
{
   struct super_block* sb = dentry->d_sb;
   App* app = FhgfsOps_getApp(sb);
   Logger* log = App_getLogger(app);
   const char* logContext = "FhgfsOps_createIntent";

   int retVal = 0;
   FhgfsOpsErr remotingRes;
   FhgfsInode* fhgfsParentInode = BEEGFS_INODE(dir);
   EntryInfo newEntryInfo;
   const char* entryName = dentry->d_name.name;
   fhgfs_stat statData;
   CreateInfo createInfo;
   OpenInfo openInfo;
   LookupIntentInfoIn inInfo; // input data for combo-request
   LookupIntentInfoOut lookupOutInfo; // result data of combo-request
   struct inode* inode = dentry->d_inode;

   FhgfsIsizeHints iSizeHints;

   #ifndef KERNEL_HAS_ATOMIC_OPEN
      fhgfs_bool isExclusiveCreate = (nameidata && (nameidata->intent.open.flags & O_EXCL) );
      int openFlags = (nameidata && nameidata->flags & LOOKUP_OPEN) ?
         nameidata->intent.open.flags : 0;
   #else
      int openFlags = 0;
   #endif // LINUX_VERSION_CODE

   FhgfsOpsHelper_logOp(4, app, dentry, inode, logContext);

   if(unlikely(dentry->d_name.len > NAME_MAX) )
      return -ENAMETOOLONG;

   FhgfsInode_entryInfoReadLock(fhgfsParentInode); // LOCK EntryInfo

   if(unlikely(!S_ISREG(createMode) ) )
      return -EINVAL; // just make sure we create a regular file here

   FhgfsInode_initIsizeHints(NULL, &iSizeHints);

   LookupIntentInfoIn_init(&inInfo, FhgfsInode_getEntryInfo(fhgfsParentInode), entryName);

   CreateInfo_init(app, dir, entryName, createMode, isExclusiveCreate, &createInfo);
   LookupIntentInfoIn_addCreateInfo(&inInfo, &createInfo);

   if (openFlags)
   {
      OpenInfo_init(&openInfo, openFlags, __FhgfsOps_isPagedMode(sb) );
      LookupIntentInfoIn_addOpenInfo(&inInfo, &openInfo);
   }

   LookupIntentInfoOut_prepare(&lookupOutInfo, &newEntryInfo, &statData);

   remotingRes = FhgfsOpsRemoting_lookupIntent(app, &inInfo, &lookupOutInfo);

   FhgfsInode_entryInfoReadUnlock(fhgfsParentInode); // UNLOCK EntryInfo

   if (unlikely(remotingRes != FhgfsOpsErr_SUCCESS) )
   {
      d_drop(dentry); // avoid leaving a negative dentry behind
      retVal = FhgfsOpsErr_toSysErr(remotingRes);
      goto outErr;
   }

   if (unlikely(lookupOutInfo.createRes != FhgfsOpsErr_SUCCESS) )
   {
      d_drop(dentry); // avoid leaving a negative dentry behind
      retVal = FhgfsOpsErr_toSysErr(lookupOutInfo.createRes);
      goto outErr;
   }

   if (unlikely(lookupOutInfo.statRes != FhgfsOpsErr_SUCCESS) )
   { // something went wrong before the server could stat the file

      if (!(lookupOutInfo.responseFlags & LOOKUPINTENTRESPMSG_FLAG_STAT) )
         Logger_logErrFormatted(log, logContext, "Unexpected stat info missing: %s",
            createInfo.entryName);


      d_drop(dentry); // avoid leaving a negative dentry behind
      retVal = FhgfsOpsErr_toSysErr(lookupOutInfo.statRes);
      goto outErr;
   }

   if(unlikely(
      lookupOutInfo.stripePattern &&
      (StripePattern_getPatternType(lookupOutInfo.stripePattern) == STRIPEPATTERN_Invalid) ) )
   { // unknown/invalid stripe pattern
      Logger_logErrFormatted(log, logContext, "Entry has invalid/unknown stripe pattern type: %s",
         createInfo.entryName);

      d_drop(dentry); // avoid leaving a negative dentry behind
      retVal = FhgfsOpsErr_toSysErr(FhgfsOpsErr_INTERNAL);
      goto outErr;
   }

   // remote success => create local inode
   retVal = __FhgfsOps_instantiateInode(dentry, lookupOutInfo.entryInfoPtr,
      lookupOutInfo.fhgfsStat, &iSizeHints);
   LookupIntentInfoOut_setEntryInfoPtr(&lookupOutInfo, NULL); /* Make sure entryInfo will not be
                                                         *  de-initialized */
   dir->i_ctime = dir->i_mtime = current_fs_time(sb);

#ifndef KERNEL_HAS_ATOMIC_OPEN
   if (lookupOutInfo.openRes == FhgfsOpsErr_SUCCESS)
   {
      struct file* file;
      struct inode* newInode;
      int openHandleRes;

      file = lookup_instantiate_filp(nameidata, dentry, generic_file_open);
      if (IS_ERR(file) )
      {
         retVal = PTR_ERR(file);
         goto outErr;
      }

      newInode = dentry->d_inode;

      openHandleRes = FhgfsOps_openReferenceHandle(app, newInode, file, openFlags,
         &lookupOutInfo);
      if (unlikely(openHandleRes) )
      {   // failed to get an fhgfs internal handle
         // printk_fhgfs_debug(KERN_INFO, "Reference handle failed\n");

         fput(file);

         retVal = openHandleRes;
         goto outErr;
      }

      LookupIntentInfoOut_setStripePattern(&lookupOutInfo, NULL);

   }
#endif // KERNEL_HAS_ATOMIC_OPEN

   // clean-up

   LookupIntentInfoOut_uninit(&lookupOutInfo);

   retVal = 0; // success

   LOG_DEBUG_FORMATTED(log, Log_SPAM, logContext, "complete. result: %d", retVal);

   return retVal;



outErr:

   // clean-up on error

   if (unlikely(lookupOutInfo.openRes == FhgfsOpsErr_SUCCESS) )
   {
      RemotingIOInfo closeInfo;
      FhgfsOpsErr closeRes;
      EntryInfo* parentEntryInfo = FhgfsInode_getEntryInfo(fhgfsParentInode);
      AtomicInt maxUsedTargetIndex;

      AtomicInt_init(&maxUsedTargetIndex, 0); // file was not open to user space yet, so 0

      RemotingIOInfo_initSpecialClose(app, lookupOutInfo.fileHandleID,
         &maxUsedTargetIndex, openFlags, &closeInfo);

      closeRes = FhgfsOpsHelper_closefileWithAsyncRetry(lookupOutInfo.entryInfoPtr, &closeInfo);
      if (closeRes != FhgfsOpsErr_SUCCESS)
         Logger_logErrFormatted(log, logContext, "Close on error: Failed to close file "
            "parentID: %s fileName: %s", EntryInfo_getEntryID(parentEntryInfo), entryName);
   }

   LookupIntentInfoOut_uninit(&lookupOutInfo);

   LOG_DEBUG_FORMATTED(log, Log_SPAM, logContext, "complete. result: %d", retVal);

   return retVal;
}


#ifdef KERNEL_HAS_ATOMIC_OPEN
/**
 * Lookup/create/open file
 *
 * Note: O_CREAT is not required to be set, so we must also handle a lookup-open.
 * Note2: Inode permissions are handled in vfs' atomic_open(), once we have successfully opened
 *        the file. If userspace is not allowed to open the file, vfs will immediately send a close
 *        request.
 * Note3: As this is a general lookup, it can also be called for directories, symlinks etc.
 *
 * Warning: This is currently only enabled when define BEEGFS_ENABLE_ATOMIC_OPEN is set, because
 *    a) dentries come out as d_unhashed() here, which should not happen.
 *    b) users reported hangs with 3.10 lt elrepo kernel when doing fcntl locking stresstests with
 *       global locks enabled.
 */
int FhgfsOps_atomicOpen(struct inode* dir, struct dentry* dentry, struct file* file,
   unsigned openFlags, umode_t createMode,  int* outOpenedFlags)
{
   struct super_block* sb = dentry->d_sb;
   App* app = FhgfsOps_getApp(sb);
   Logger* log = App_getLogger(app);
   const char* logContext = "FhgfsOps_atomicOpen";

   int retVal = -EINVAL; // any error is fine by default
   FhgfsOpsErr remotingRes;
   FhgfsInode* fhgfsParentInode = BEEGFS_INODE(dir);
   EntryInfo newEntryInfo;
   struct inode* newInode;
   const char* entryName = dentry->d_name.name;
   struct inode* inode = dentry->d_inode;
   FhgfsInode* fhgfsInode;


   CreateInfo createInfo;
   OpenInfo openInfo;

   LookupIntentInfoIn inInfo; // input data for combo-request
   LookupIntentInfoOut lookupOutInfo; // result data of combo-request

   fhgfs_stat statData; // passed to lookupOutInfo
   fhgfs_stat* statDataPtr;

   int openHandleRes;
   int instantiateRes;

   FhgfsIsizeHints iSizeHints;

   fhgfs_bool isCreate = !!(openFlags & O_CREAT);

   FhgfsOpsHelper_logOp(4, app, dentry, inode, logContext);

   if(unlikely(dentry->d_name.len > NAME_MAX) )
      return -ENAMETOOLONG;

   if (inode)
      fhgfsInode = BEEGFS_INODE(inode);
   else
      fhgfsInode = NULL;

   FhgfsInode_initIsizeHints(fhgfsInode, &iSizeHints);

   FhgfsInode_entryInfoReadLock(fhgfsParentInode); // LOCK EntryInfo

   LookupIntentInfoIn_init(&inInfo, FhgfsInode_getEntryInfo(fhgfsParentInode), entryName);

   if (isCreate)
   {
      fhgfs_bool isExclusiveCreate = !!(openFlags & O_EXCL);

      if (!(createMode & S_IFREG) )
      {
         FhgfsInode_entryInfoReadUnlock(fhgfsParentInode); // UNLOCK EntryInfo
         return -EINVAL; // we can still easily return here, no need to free something yet
      }

      CreateInfo_init(app, dir, entryName, createMode, isExclusiveCreate, &createInfo);
      LookupIntentInfoIn_addCreateInfo(&inInfo, &createInfo);
   }

   OpenInfo_init(&openInfo, openFlags, __FhgfsOps_isPagedMode(sb) );
   LookupIntentInfoIn_addOpenInfo(&inInfo, &openInfo);

   LookupIntentInfoOut_prepare(&lookupOutInfo, &newEntryInfo, &statData);

   remotingRes = FhgfsOpsRemoting_lookupIntent(app, &inInfo, &lookupOutInfo); // remote call

   FhgfsInode_entryInfoReadUnlock(fhgfsParentInode); // UNLOCK EntryInfo

   if (unlikely(remotingRes != FhgfsOpsErr_SUCCESS) )
   {
      // printk_fhgfs_debug(KERN_INFO, "%s: Remoting error\n", __func__);
      // d_drop(dentry); // avoid leaving a negative dentry behind. Don't drop for atomic_open!
      retVal = FhgfsOpsErr_toSysErr(remotingRes);
      goto outErr;
   }

   if (!isCreate && lookupOutInfo.lookupRes != FhgfsOpsErr_SUCCESS)
   {
      // printk_fhgfs_debug(KERN_INFO, "%s: lookup error\n", __func__);
      // d_drop(dentry); // avoid leaving a negative dentry behind. Don't for atomic_open!

      retVal = FhgfsOpsErr_toSysErr(lookupOutInfo.lookupRes);
      goto outErr;
   }
   else
   if (lookupOutInfo.statRes != FhgfsOpsErr_SUCCESS)
   { // something went wrong before the server could stat the file

      // printk_fhgfs_debug(KERN_INFO, "%s: stat error \n", __func__);
      // d_drop(dentry); // avoid leaving a negative dentry behind. Don't for atomic_open!

      if (lookupOutInfo.statRes == FhgfsOpsErr_NOTOWNER)
      {  // directory or inter-dir hardlink
         statDataPtr = NULL; // force a stat call to the real owner
      }
      else
      {
         retVal = FhgfsOpsErr_toSysErr(lookupOutInfo.statRes);
         goto outErr;
      }
   }
   else
   { // stat success
      statDataPtr = lookupOutInfo.fhgfsStat;
   }

   if (isCreate)
   {
      if (lookupOutInfo.createRes == FhgfsOpsErr_SUCCESS) // implies isCreate == true
      {
         *outOpenedFlags |= FILE_CREATED;

         if (lookupOutInfo.lookupRes != FhgfsOpsErr_SUCCESS)
         {  // only update directory time stamps if the file did not exist yet
            dir->i_ctime = dir->i_mtime = current_fs_time(sb);
         }
      }
      else
      {
         // printk_fhgfs_debug(KERN_INFO, "%s: create error\n", __func__);

         retVal = FhgfsOpsErr_toSysErr(lookupOutInfo.createRes);
         goto outErr;
      }
   }


   // remote success => create local inode
   instantiateRes = __FhgfsOps_instantiateInode(dentry, lookupOutInfo.entryInfoPtr, statDataPtr,
      &iSizeHints);
   LookupIntentInfoOut_setEntryInfoPtr(&lookupOutInfo, NULL); /* Make sure entryInfo will not be
                                                               * de-initialized */

   if (unlikely(instantiateRes) )
   {  // instantiate error
      retVal = instantiateRes;
      goto outErr;
   }

   // instantiate success

   if (lookupOutInfo.openRes != FhgfsOpsErr_SUCCESS)
   {
      // printk_fhgfs_debug(KERN_INFO, "Failed to open file, openRes: %d\n",
      //                    lookupOutInfo.openRes);

      retVal = FhgfsOpsErr_toSysErr(lookupOutInfo.openRes);
      goto outLookupSuccessOpenFailure;
   }

   // remote open success

   file->f_path.dentry = dentry; /* Assign the dentry, finish open does that, but we
                                  * already need it in openReferenceHandle() */

   newInode = dentry->d_inode;

   openHandleRes = FhgfsOps_openReferenceHandle(app, newInode, file, openFlags,
      &lookupOutInfo);
   if (unlikely(openHandleRes) )
   {   // failed to get an fhgfs internal handle
      // printk_fhgfs_debug(KERN_INFO, "Reference handle failed\n");

      retVal = openHandleRes;
      goto outLookupSuccessOpenFailure;
   }

   // internal open handle success

   // stripePattern is assigned to FhgfsInode now, make sure it does not get free'ed
   LookupIntentInfoOut_setStripePattern(&lookupOutInfo, NULL);

   retVal = finish_open(file, dentry, generic_file_open, outOpenedFlags);
   if (unlikely(retVal) )
   {  // finish open failed
      int releaseRes;

      // printk_fhgfs_debug(KERN_INFO, "finish_open failed: %d\n", retVal);

      releaseRes = FhgfsOps_release(dentry->d_inode, file);
      if (unlikely(releaseRes) )
      {
         EntryInfo* parentEntryInfo = FhgfsInode_getEntryInfo(fhgfsParentInode);

         Logger_logErrFormatted(log, logContext, "Close on error: Failed to close file "
            "parentID: %s fileName: %s", EntryInfo_getEntryID(parentEntryInfo), entryName);
      }
   }

   // clean-up

   LookupIntentInfoOut_uninit(&lookupOutInfo);

   retVal = 0; // success

   LOG_DEBUG_FORMATTED(log, Log_SPAM, logContext, "complete. result: %d", retVal);

   return retVal;


outLookupSuccessOpenFailure:

   dget(dentry); /* Get another dentry reference, the caller (atomic_open) will drop it
                  * it again immediately */
   retVal = finish_no_open(file, dentry); // successful lookup/create, but failed open


outErr:

   // clean-up on error

   if (unlikely(lookupOutInfo.openRes == FhgfsOpsErr_SUCCESS) )
   {
      RemotingIOInfo closeInfo;
      FhgfsOpsErr closeRes;
      EntryInfo* parentEntryInfo = FhgfsInode_getEntryInfo(fhgfsParentInode);
      AtomicInt maxUsedTargetIndex;

      AtomicInt_init(&maxUsedTargetIndex, 0); // file was not open to user space yet, so 0

      RemotingIOInfo_initSpecialClose(app, lookupOutInfo.fileHandleID,
         &maxUsedTargetIndex, openFlags, &closeInfo);

      closeRes = FhgfsOpsRemoting_closefile(lookupOutInfo.entryInfoPtr, &closeInfo);
      if (closeRes != FhgfsOpsErr_SUCCESS)
         Logger_logErrFormatted(log, logContext, "Close on error: Failed to close file "
            "parentID: %s fileName: %s", EntryInfo_getEntryID(parentEntryInfo), entryName);
   }

   LookupIntentInfoOut_uninit(&lookupOutInfo);

   LOG_DEBUG_FORMATTED(log, Log_SPAM, logContext, "complete. result: %d", retVal);

   return retVal;
}
#endif // KERNEL_HAS_ATOMIC_OPEN


int FhgfsOps_unlink(struct inode* dir, struct dentry* dentry)
{
   struct super_block* sb = dentry->d_sb;
   App* app = FhgfsOps_getApp(sb);
   Logger* log = App_getLogger(app);
   const char* logContext = "FhgfsOps_unlink";

   int retVal = 0;
   FhgfsOpsErr unlinkRes;
   FhgfsInode* fhgfsParentInode = BEEGFS_INODE(dir);
   const char* entryName = dentry->d_name.name;
   struct inode* inode = dentry->d_inode;


   if(Logger_getLogLevel(log) >= 4)
      FhgfsOpsHelper_logOp(4, app, dentry, inode, logContext);

   FhgfsInode_entryInfoReadLock(fhgfsParentInode); // LOCK EntryInfo

   unlinkRes = FhgfsOpsRemoting_unlinkfile(app, FhgfsInode_getEntryInfo(fhgfsParentInode),
      entryName);

   FhgfsInode_entryInfoReadUnlock(fhgfsParentInode); // UNLOCK EntryInfo

   if(unlinkRes != FhgfsOpsErr_SUCCESS)
   {
      retVal = FhgfsOpsErr_toSysErr(unlinkRes);

      if(retVal == -ENOENT)
         d_drop(dentry);
   }
   else
   { // remote success
      dir->i_ctime = dir->i_mtime = current_fs_time(sb);

      if(dentry->d_inode)
      {
         struct inode* fileInode = dentry->d_inode;
         FhgfsInode* fhgfsInode = BEEGFS_INODE(fileInode);

         FhgfsInode_invalidateCache(fhgfsInode);
         fileInode->i_ctime = dir->i_ctime;
         drop_nlink(fileInode);

      }
   }

   return retVal;
}

/**
 * Create special file.
 */
#ifdef KERNEL_HAS_UMODE_T
int FhgfsOps_mknod(struct inode* dir, struct dentry* dentry, umode_t mode, dev_t dev)
#else
int FhgfsOps_mknod(struct inode* dir, struct dentry* dentry, int mode, dev_t dev)
#endif // KERNEL_HAS_UMODE_T
{
   struct super_block* sb = dentry->d_sb;
   App* app = FhgfsOps_getApp(sb);
   Logger* log = App_getLogger(app);
   const char* logContext = "FhgfsOps_mknod";

   int retVal = 0;
   FhgfsOpsErr mkRes;
   FhgfsInode* fhgfsParentInode = BEEGFS_INODE(dir);
   EntryInfo newEntryInfo;
   const char* entryName = dentry->d_name.name;
   struct CreateInfo createInfo;
   struct inode* inode = dentry->d_inode;

   FhgfsIsizeHints iSizeHints;

   if(Logger_getLogLevel(log) >= 4)
      FhgfsOpsHelper_logOp(4, app, dentry, inode, logContext);

   CreateInfo_init(app, dir, entryName, mode, fhgfs_false, &createInfo);

   FhgfsInode_entryInfoReadLock(fhgfsParentInode); // LOCK EntryInfo

   mkRes = FhgfsOpsRemoting_mkfile(app, FhgfsInode_getEntryInfo(fhgfsParentInode),
      &createInfo, &newEntryInfo);

   FhgfsInode_entryInfoReadUnlock(fhgfsParentInode); // UNLOCK EntryInfo

   if(mkRes != FhgfsOpsErr_SUCCESS)
   {
      d_drop(dentry); // avoid leaving a negative dentry behind
      retVal = FhgfsOpsErr_toSysErr(mkRes);
      goto outErr;
   }

   retVal = __FhgfsOps_instantiateInode(dentry, &newEntryInfo, NULL, &iSizeHints);

   dir->i_ctime = dir->i_mtime = current_fs_time(sb);


outErr:
   // clean-up

   return retVal;
}


/**
 * @param dir directory for the new link
 * @param dentry dentry of the new entry that we want to create
 * @param to where the symlink points to
 */
int FhgfsOps_symlink(struct inode* dir, struct dentry* dentry, const char* to)
{
   struct super_block* sb = dentry->d_sb;
   App* app = FhgfsOps_getApp(sb);
   Logger* log = App_getLogger(app);
   const char* logContext = "FhgfsOps_symlink";

   int retVal;
   int mode = S_IFLNK | S_IRWXU | S_IRWXG | S_IRWXO;
   FhgfsInode* fhgfsParentInode = BEEGFS_INODE(dir);
   EntryInfo newEntryInfo;
   const char* entryName = dentry->d_name.name;
   struct CreateInfo createInfo;
   struct inode* inode = dentry->d_inode;

   FhgfsIsizeHints iSizeHints;


   if(unlikely(Logger_getLogLevel(log) >= 4) )
      FhgfsOpsHelper_logOp(4, app, dentry, inode, logContext);


   CreateInfo_init(app, dir, entryName, mode, fhgfs_false, &createInfo);

   FhgfsInode_entryInfoReadLock(fhgfsParentInode); // LOCK EntryInfo

   retVal = FhgfsOpsHelper_symlink(app, FhgfsInode_getEntryInfo(fhgfsParentInode), to, &createInfo,
      &newEntryInfo);

   FhgfsInode_entryInfoReadUnlock(fhgfsParentInode); // UNLOCK EntryInfo

   if(retVal)
   { // error occurred
      d_drop(dentry); // avoid leaving a negative dentry behind
   }
   else
   { // remote success => create local inode
      retVal = __FhgfsOps_instantiateInode(dentry, &newEntryInfo, NULL, &iSizeHints);

      dir->i_ctime = dir->i_mtime = current_fs_time(sb);
   }


   return retVal;
}

/**
 * @param fromFileDentry the already existing dentry
 * @param toDirInode parent (directory) inode of the new dentry
 * @param toFileDentry the new dentry (link) that we want to create
 */
int FhgfsOps_link(struct dentry* fromFileDentry, struct inode* toDirInode,
   struct dentry* toFileDentry)
{
   // note: we don't support hardlinks yet, but have a config option to create symlinks instead
      // of hardlinks.

   struct super_block* sb = toFileDentry->d_sb;
   App* app = FhgfsOps_getApp(sb);
   Logger* log = App_getLogger(app);
   Config* cfg = App_getConfig(app);
   const char* logContext = "FhgfsOps_link";

   struct inode* fileInode = fromFileDentry->d_inode;
   FhgfsInode* fhgfsFileInode = BEEGFS_INODE(fileInode);

   FhgfsInode* fhgfsToDirInode    = BEEGFS_INODE(toDirInode);

   struct inode* fromDirInode    = fromFileDentry->d_parent->d_inode;
   FhgfsInode* fhgfsFromDirInode  = BEEGFS_INODE(fromDirInode);

   const char* fromFileName = fromFileDentry->d_name.name;
   unsigned    fromFileLen  = fromFileDentry->d_name.len;

   const char* toFileName   = toFileDentry->d_name.name;
   unsigned    toFileLen    = toFileDentry->d_name.len;

   EntryInfo* fromDirInfo;
   EntryInfo* toDirInfo;
   EntryInfo* fromFileInfo;

   int retVal;


   if(unlikely(Logger_getLogLevel(log) >= Log_DEBUG) )
      FhgfsOpsHelper_logOpMsg(Log_SPAM, app, fromFileDentry, fromFileDentry->d_inode, logContext,
         "From: %s; To: %s", fromFileName, toFileName);

   FhgfsInode_entryInfoReadLock(fhgfsFromDirInode); // LOCK EntryInfo
   FhgfsInode_entryInfoReadLock(fhgfsToDirInode);   // LOCK EntryInfo
   FhgfsInode_entryInfoReadLock(fhgfsFileInode);    // LOCK EntryInfo

   fromDirInfo   = FhgfsInode_getEntryInfo(fhgfsFromDirInode);
   toDirInfo     = FhgfsInode_getEntryInfo(fhgfsToDirInode);
   fromFileInfo  = FhgfsInode_getEntryInfo(fhgfsFileInode);


#if 0 // TODO: Re-add here once we support inter directory hard-links
   ihold(fileInode);
   inc_nlink(fileInode);
#endif

   if (strcmp(fromDirInfo->entryID, toDirInfo->entryID) == 0)
   {  // same inode, so hard link within the same directory
      int linkRes;

      // TODO: Remove it here once we support inter directory hard-links
      ihold(fileInode);
      inc_nlink(fileInode);

      linkRes = FhgfsOpsRemoting_hardlink(app, fromFileName, fromFileLen, fromFileInfo,
         fromDirInfo, toFileName, toFileLen, toDirInfo);

      retVal = FhgfsOpsErr_toSysErr(linkRes);

      if (retVal) // TODO: Remove it here once we support inter directory hard-links
      {
         d_drop(toFileDentry);
         iput(fileInode);
         drop_nlink(fileInode);
      }
      else
      {
         d_instantiate(toFileDentry, fileInode);
      }
   }
   else
   {
      if(!Config_getSysCreateHardlinksAsSymlinks(cfg) )
      { // creation of hardlinks not allowed/supported
         FhgfsInode_entryInfoReadUnlock(fhgfsFileInode);    // UNLOCK fromFileInfo

         FhgfsOpsHelper_logOpMsg(Log_DEBUG, app, toFileDentry, fileInode, logContext,
            "Hardlinks between different dirs not supported yet.");

         FhgfsInode_entryInfoReadLock(fhgfsFileInode);    // LOCK fromParentEntryInfo

         retVal = -EPERM; // do not allow creation of hardlink
      }
      else
         retVal = FhgfsOps_hardlinkAsSymlink(fromFileDentry, toDirInode, toFileDentry);
   }

   FhgfsInode_entryInfoReadUnlock(fhgfsFileInode);    // UNLOCK fromFileInfo
   FhgfsInode_entryInfoReadUnlock(fhgfsToDirInode);   // UNLOCK toDirInfo
   FhgfsInode_entryInfoReadUnlock(fhgfsFromDirInode); // UNLOCK fromDirInfo

#if 0 // TODO: Re-add here once we support inter directory hard-links
   if (retVal)
      d_drop(toFileDentry);
      iput(fileInode);
      drop_nlink(fileInode);

   else
   {
      d_instantiate(toFileDentry, fileInode);
   }
#endif

   FhgfsInode_invalidateCache(fhgfsFileInode);

   return retVal;
}

/**
 * create symlink instead of hardlink
 *
 * @param oldDentry the already existing link
 * @param dir directory of the new link
 * @param newDentry the new link that we want to create
 *
 * Note:
 */
int FhgfsOps_hardlinkAsSymlink(struct dentry* oldDentry, struct inode* dir,
   struct dentry* newDentry)
{
   struct super_block* sb = newDentry->d_sb;
   App* app = FhgfsOps_getApp(sb);
   Logger* log = App_getLogger(app);
   const char* logContext = "FhgfsOps_link";

   FhgfsIsizeHints iSizeHints;

   NoAllocBufferStore* bufStore = App_getPathBufStore(app);

   int retVal;
   int mode = S_IFLNK | S_IRWXU | S_IRWXG | S_IRWXO;
   FhgfsInode* fhgfsParentInode = BEEGFS_INODE(dir);
   EntryInfo newEntryInfo;
   const char* entryName = newDentry->d_name.name;

   char *oldPathStr, *newPathStr;
   char *oldPathStoreBuf, *newPathStoreBuf;
   char *oldPathTmp, *newPathTmp;
   char* oldRelativePathStr;

   struct CreateInfo createInfo;

   // resolve oldDentry to full path
   oldPathTmp = __FhgfsOps_pathResolveToStoreBuf(bufStore, oldDentry, &oldPathStoreBuf);
   if (unlikely(IS_ERR(oldPathTmp) ) )
   {
      int error = IS_ERR(oldPathTmp);
      Logger_logFormatted(log, 2, logContext, "Error in link(): %d", error);
      retVal = error;
      goto errorOldPath;
   }

   oldPathStr = StringTk_strDup(oldPathTmp);
   NoAllocBufferStore_addBuf(bufStore, oldPathStoreBuf);

   // resolve newDentry to full path
   newPathTmp = __FhgfsOps_pathResolveToStoreBuf(bufStore, newDentry, &newPathStoreBuf);
   if (unlikely(IS_ERR(newPathTmp) ) )
   {
      int error = IS_ERR(oldPathTmp);
      Logger_logFormatted(log, 2, logContext, "Error in link(): %d", error);
      retVal = error;
      goto errorNewPath;
   }

   newPathStr = StringTk_strDup(newPathTmp);
   NoAllocBufferStore_addBuf(bufStore, newPathStoreBuf);

   // resolve relative path from new to old
   FhgfsOpsHelper_getRelativeLinkStr(newPathStr, oldPathStr, &oldRelativePathStr);

   if(unlikely(Logger_getLogLevel(log) >= 4) )
   {
      Logger_logFormatted(log, 4, logContext, "called (as symlink). Path: %s; To: %s",
         newPathStr, oldRelativePathStr);
   }

   CreateInfo_init(app, dir, entryName, mode, fhgfs_false, &createInfo);

   FhgfsInode_entryInfoReadLock(fhgfsParentInode); // LOCK EntryInfo

   retVal = FhgfsOpsHelper_symlink(app, FhgfsInode_getEntryInfo(fhgfsParentInode),
      oldRelativePathStr, &createInfo, &newEntryInfo);

   FhgfsInode_entryInfoReadUnlock(fhgfsParentInode); // UNLOCK EntryInfo

   if(retVal)
   { // error occurred
      d_drop(newDentry); // avoid leaving a negative dentry behind
   }
   else
   { // remote success => create local inode
      retVal = __FhgfsOps_instantiateInode(newDentry, &newEntryInfo, NULL, &iSizeHints);

      dir->i_ctime = dir->i_mtime = current_fs_time(sb);
   }

   // clean-up

   kfree(oldRelativePathStr);
   kfree(newPathStr);
errorNewPath:
   kfree(oldPathStr);
errorOldPath:
   return retVal;
}


static void __beegfs_follow_link(struct dentry* dentry, char** linkBody, void** cookie)
{
   struct inode* inode = dentry->d_inode;
   App* app = FhgfsOps_getApp(inode->i_sb);
   Logger* log = App_getLogger(app);
   const char* logContext = "FhgfsOps_follow_link";

   int readRes;
   char* bufPage = (char*)__os_get_free_page();
   char* destination = bufPage;

   FhgfsInode* fhgfsParentInode = BEEGFS_INODE(inode);

   if(unlikely(Logger_getLogLevel(log) >= 5) )
      FhgfsOpsHelper_logOp(5, app, dentry, inode, logContext);

   FhgfsInode_entryInfoReadLock(fhgfsParentInode); // LOCK EntryInfo

   readRes = FhgfsOpsHelper_readlink(app, FhgfsInode_getEntryInfo(fhgfsParentInode), bufPage,
      PAGE_SIZE-1);

   FhgfsInode_entryInfoReadUnlock(fhgfsParentInode); // UNLOCK EntryInfo

   if(readRes < 0)
   {
      destination = ERR_PTR(readRes);
   }
   else
      bufPage[readRes] = 0;

   if(readRes == (PAGE_SIZE-1) )
   { // link destination too long
      destination = ERR_PTR(-ENAMETOOLONG);
   }

   // store link destination
   *linkBody = destination;
   *cookie = bufPage;

   if(IS_ERR(destination) )
   {
      free_page( (unsigned long)bufPage);
      *cookie = destination;
   }

   // Note: free_page() is called by the put_link method in the success case
}

static void __beegfs_put_link(struct inode* inode, void* cookie)
{
   free_page( (unsigned long)cookie);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,2,0)
void* FhgfsOps_follow_link(struct dentry* dentry, struct nameidata* nd)
{
   char* destination;
   void* cookie;

   __beegfs_follow_link(dentry, &destination, &cookie);
   nd_set_link(nd, destination);
   return cookie;
}


void FhgfsOps_put_link(struct dentry* dentry, struct nameidata* nd, void* p)
{
   if(!IS_ERR(p) )
      __beegfs_put_link(dentry->d_inode, p);
}
#else
const char* FhgfsOps_follow_link(struct dentry* dentry, void** cookie)
{
   char* destination;

   __beegfs_follow_link(dentry, &destination, cookie);
   return destination;
}


void FhgfsOps_put_link(struct inode* inode, void* cookie)
{
   __beegfs_put_link(inode, cookie);
}
#endif


int FhgfsOps_rename(struct inode* inodeDirFrom, struct dentry* dentryFrom,
   struct inode* inodeDirTo, struct dentry* dentryTo)
{
   struct super_block* sb = dentryFrom->d_sb;
   App* app = FhgfsOps_getApp(sb);
   Logger* log = App_getLogger(app);
   const char* logContext = __func__;
   FhgfsOpsErr renameRes;

   struct inode* fromEntryInode = dentryFrom->d_inode; // no need to lock it, see other in kernel fs
   FhgfsInode* fhgfsFromEntryInode = BEEGFS_INODE(fromEntryInode);
   DirEntryType entryType = FhgfsInode_getDirEntryType(fhgfsFromEntryInode);

   EntryInfo* fromDirInfo; // EntryInfo about the 'from' directory
   EntryInfo* toDirInfo;   // EntryInfo about the 'to' directory

   int retVal = 0;

   FhgfsInode* fhgfsFromDirInode = BEEGFS_INODE(inodeDirFrom);
   FhgfsInode* fhgfsToDirInode   = BEEGFS_INODE(inodeDirTo);

   const char* oldName = dentryFrom->d_name.name;
   unsigned oldLen = dentryFrom->d_name.len;

   const char* newName = dentryTo->d_name.name;
   unsigned newLen = dentryTo->d_name.len;

   if(unlikely(Logger_getLogLevel(log) >= Log_DEBUG) )
      FhgfsOpsHelper_logOpMsg(Log_SPAM, app, dentryFrom, dentryFrom->d_inode, logContext,
         "From: %s; To: %s", oldName, newName);

   FhgfsInode_entryInfoReadLock(fhgfsFromDirInode); // LOCK EntryInfo
   FhgfsInode_entryInfoReadLock(fhgfsToDirInode);   // LOCK EntryInfo

   // note the fileInode also needs to be locked to prevent reference/release during a rename
   FhgfsInode_entryInfoWriteLock(fhgfsFromEntryInode);   // LOCK EntryInfo (renamed file dir)

   fromDirInfo = FhgfsInode_getEntryInfo(fhgfsFromDirInode);
   toDirInfo   = FhgfsInode_getEntryInfo(fhgfsToDirInode);

   renameRes = FhgfsOpsRemoting_rename(app, oldName, oldLen, entryType, fromDirInfo,
      newName, newLen, toDirInfo);
   if(renameRes != FhgfsOpsErr_SUCCESS)
   {
      int logLevel = Log_NOTICE;
      EntryInfo* fromEntryInfo = FhgfsInode_getEntryInfo(fhgfsFromEntryInode);

      if( (renameRes == FhgfsOpsErr_PATHNOTEXISTS) || (renameRes == FhgfsOpsErr_INUSE) )
         logLevel = Log_DEBUG; // don't bother user with non-error messages

      /* We cannot rename onto an existing empty directory, yet. So Log_DEBUG only.
       * TODO: Once we support these renames we further need to restrict to empty
       * (link count = 2) dirs */
      if (renameRes == FhgfsOpsErr_EXISTS &&
          dentryTo->d_inode && S_ISDIR(dentryTo->d_inode->i_mode) )
         logLevel = Log_DEBUG;

      Logger_logFormatted(log, logLevel, logContext,
         "Rename failed: %s fromDirID: %s oldName: %s toDirID: %s newName: %s EntryID: %s",
         FhgfsOpsErr_toErrString(renameRes),
         fromDirInfo->entryID, oldName, toDirInfo->entryID, newName, fromEntryInfo->entryID);

      retVal = FhgfsOpsErr_toSysErr(renameRes);
   }
   else
   { // remote success
      fromEntryInode->i_ctime = inodeDirFrom->i_ctime = inodeDirFrom->i_mtime =
         inodeDirTo->i_ctime = inodeDirTo->i_mtime = current_fs_time(sb);

      /* TODO: Future releases might keep the inode on the original server, so the ownerNodeID
       *       is not going to be updated then and also must not be wrongly set in our inode. */
      FhgfsInode_updateEntryInfoOnRenameUnlocked(fhgfsFromEntryInode, toDirInfo, newName);
   }

   FhgfsInode_entryInfoWriteUnlock(fhgfsFromEntryInode);   // UNLOCK EntryInfo (renamed file dir)
   FhgfsInode_entryInfoReadUnlock(fhgfsToDirInode);   // UNLOCK ToDirEntryInfo
   FhgfsInode_entryInfoReadUnlock(fhgfsFromDirInode); // UNLOCK FromDirEntryInfo

   LOG_DEBUG_FORMATTED(log, Log_DEBUG, logContext, "remoting complete. result: %d", (int)renameRes);


   // clean-up

   return retVal;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,8,0)

/**
 * Note: i_size of the inode is set to the desired size before this is called
 *
 * Note: unused, can later be removed; truncation is done via sys_truncate -> setattr (and this was
 * just called additionally indirectly via sys_truncate -> vmtruncate -> truncate).
 */
void FhgfsOps_truncate(struct inode* inode)
{
   App* app = FhgfsOps_getApp(inode->i_sb);
   Logger* log = App_getLogger(app);
   const char* logContext = "FhgfsOps_truncate";

   FhgfsInode* fhgfsInode = BEEGFS_INODE(inode);

   loff_t size = i_size_read(inode);

   if(unlikely(Logger_getLogLevel(log) >= 5) )
   {
      struct dentry* dentry = d_find_alias(inode); // calls dget_locked (can return NULL)

      FhgfsOpsHelper_logOp(5, app, dentry, inode, logContext);

      if(dentry)
         dput(dentry);
   }

   FhgfsInode_entryInfoReadLock(fhgfsInode); // LOCK EntryInfo

   // (see below why return val is not checked)
   FhgfsOpsRemoting_truncfile(app, FhgfsInode_getEntryInfo(fhgfsInode), size);

   FhgfsInode_entryInfoReadUnlock(fhgfsInode); // UNLOCK EntryInfo


   return; // note: this is an internal function which gives us no way to return a result.
}

#endif // LINUX_VERSION_CODE

/**
 * Note: This is almost a copy of general vmtruncate(), just with inode->i_lock around the i_size
 * updates.
 * Note: This is not called directly, but via setattr
 *
 * @param offset file offset for truncation
 */
int FhgfsOps_vmtruncate(struct inode* inode, loff_t offset)
{
   if(i_size_read(inode) < offset)
   {
      unsigned long limit;

      limit = current->signal->rlim[RLIMIT_FSIZE].rlim_cur;
      if (limit != RLIM_INFINITY && offset > (loff_t)limit)
         goto out_sig;

      if (offset > (loff_t)(inode->i_sb->s_maxbytes) )
         goto out_big;

      spin_lock(&inode->i_lock);
      i_size_write(inode, offset);
      spin_unlock(&inode->i_lock);
   }
   else
   {
      struct address_space *mapping = inode->i_mapping;

      /*
       * truncation of in-use swapfiles is disallowed - it would
       * cause subsequent swapout to scribble on the now-freed
       * blocks.
       */
      if(IS_SWAPFILE(inode) )
         return -ETXTBSY;

      spin_lock(&inode->i_lock);
      i_size_write(inode, offset);
      spin_unlock(&inode->i_lock);

      /*
       * unmap_mapping_range is called twice, first simply for
       * efficiency so that truncate_inode_pages does fewer
       * single-page unmaps.  However after this first call, and
       * before truncate_inode_pages finishes, it is possible for
       * private pages to be COWed, which remain after
       * truncate_inode_pages finishes, hence the second
       * unmap_mapping_range call must be made for correctness.
       */
      unmap_mapping_range(mapping, offset + PAGE_SIZE - 1, 0, 1);
      truncate_inode_pages(mapping, offset);
      unmap_mapping_range(mapping, offset + PAGE_SIZE - 1, 0, 1);
   }

   return 0;

out_sig:
   send_sig(SIGXFSZ, current, 0);
out_big:
   return -EFBIG;
}

/**
 * Note: Call this once during module init (and remember to call _destroyInodeCache() )
 */
fhgfs_bool FhgfsOps_initInodeCache(void)
{
   FhgfsInodeCache =
      OsCompat_initKmemCache(FhgfsOpsInode_CACHE_NAME, sizeof(FhgfsInode), FhgfsOps_initInodeOnce);

   if (!FhgfsInodeCache)
      return fhgfs_false;

   return fhgfs_true;
}

void FhgfsOps_destroyInodeCache(void)
{
   if(FhgfsInodeCache)
      kmem_cache_destroy(FhgfsInodeCache);
}


struct inode* FhgfsOps_alloc_inode(struct super_block *sb)
{
   App* app = FhgfsOps_getApp(sb);
   Config* cfg = App_getConfig(app);

   FhgfsInode* fhgfsInode;

   fhgfsInode = kmem_cache_alloc(FhgfsInodeCache, GFP_KERNEL);
   if(unlikely(!fhgfsInode) )
      return NULL;

   FhgfsInode_allocInit(fhgfsInode);

   fhgfsInode->vfs_inode.i_blkbits = Config_getTuneInodeBlockBits(cfg);

   return (struct inode*)fhgfsInode;
}

void FhgfsOps_destroy_inode(struct inode* inode)
{
   FhgfsInode* fhgfsInode = BEEGFS_INODE(inode);

   FhgfsInode_destroyUninit(fhgfsInode);

   kmem_cache_free(FhgfsInodeCache, inode);
}

/**
 * Creates a new inode, inits it from the kstat, inits the ops (depending on the mode)
 * and hashes it.
 *
 * Note: Make sure everything is set in the kstat _before_ you call this, because we hash
 * the inode in here (so it can be found and accessed by others when this method returns).
 * Note: Consider using the _instantiateInode()-wrapper instead of calling this directly for new
 * files/dirs.
 *
 * @param kstat must have a valid .ino (inode number)
 * @param dev set to 0 if not required (only used for special files)
 * @param entryInfoPtr contained strings will just be moved to the new inode or free'd in case of an
 * error (or cached inode), so don't access the given entryInfoPtr anymore after calling this.
 * @param parentNodeID: usually 0, except for NFS export callers, which needs it to connect dentries
 *    with their parents. By default dentries are connected to their parents, so usually this
 *    is not required (nfs is an exception).
 * @return NULL if not successful
 */
struct inode* __FhgfsOps_newInodeWithParentID(struct super_block* sb, struct kstat* kstat,
   dev_t dev, EntryInfo* entryInfo, uint16_t parentNodeID, FhgfsIsizeHints* iSizeHints)
{
   App* app = FhgfsOps_getApp(sb);
   Config* cfg = App_getConfig(app);

   FhgfsInode* fhgfsInode;

   FhgfsInodeComparisonInfo comparisonInfo =
   {
      .inodeHash = kstat->ino, // pre-set by caller
      .entryID = entryInfo->entryID,
   };


   // check inode cache for an existing inode with this ID (and get it) or allocate a new one

   struct inode* inode = iget5_locked(sb, kstat->ino,
      __FhgfsOps_compareInodeID, __FhgfsOps_initNewInodeDummy, &comparisonInfo);

   if(unlikely(!inode || IS_ERR(inode) ) )
      goto cleanup_entryInfo; // allocation of new inode failed

   fhgfsInode = BEEGFS_INODE(inode);

   if( !(inode->i_state & I_NEW) )
   {  // Found an existing inode, which is possibly actively used. We still need to update it.
      FhgfsInode_entryInfoWriteLock(fhgfsInode); // LOCK EntryInfo
      FhgfsInode_updateEntryInfoUnlocked(fhgfsInode, entryInfo);
      FhgfsInode_entryInfoWriteUnlock(fhgfsInode); // UNLOCK EntryInfo

      spin_lock(&inode->i_lock);

      __FhgfsOps_applyStatDataToInodeUnlocked(kstat, iSizeHints, inode); // already locked
      Time_setToNow(&fhgfsInode->dataCacheTime);
      spin_unlock(&inode->i_lock);

      goto outNoCleanUp; // we found a matching existing inode => no init needed
   }

   fhgfsInode->parentNodeID = parentNodeID;

   /* note: new inodes are protected by the I_NEW flag from access by other threads until we
    *       call unlock_new_inode(). */

   // init this fresh new inode...

   // no one can access inode yet => unlocked
   __FhgfsOps_applyStatDataToInodeUnlocked(kstat, iSizeHints, inode);

   inode->i_ino = kstat->ino; // pre-set by caller

   inode->i_flags |= S_NOATIME | S_NOCMTIME; // timestamps updated by server

   mapping_set_gfp_mask(&inode->i_data, GFP_USER); // avoid highmem for page cache pages

   // move values (no actual string copy)
   EntryInfo_copy(entryInfo, FhgfsInode_getEntryInfo(fhgfsInode) );

   switch (kstat->mode & S_IFMT)
   {
      case S_IFREG: // regular file
      {
         if(Config_getTuneFileCacheTypeNum(cfg) == FILECACHETYPE_Native)
         {
            inode->i_fop = &fhgfs_file_native_ops;
            inode->i_data.a_ops = &fhgfs_addrspace_native_ops;
         }
         else
         if(Config_getTuneFileCacheTypeNum(cfg) == FILECACHETYPE_Paged)
         { // with pagecache
            inode->i_fop = &fhgfs_file_pagecache_ops;
            inode->i_data.a_ops = &fhgfs_address_pagecache_ops;
         }
         else
         { // no pagecache (=> either none or buffered cache)
            inode->i_fop = &fhgfs_file_buffered_ops;
            inode->i_data.a_ops = &fhgfs_address_ops;
         }

         #if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32) ) && \
            (LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0) )
            inode->i_data.backing_dev_info = FhgfsOps_getBdi(sb);
         #endif

         inode->i_op = App_getFileInodeOps(app);
      } break;

      case S_IFDIR: // directory
      {
         inode->i_op = App_getDirInodeOps(app);
         inode->i_fop = &fhgfs_dir_ops;
      } break;

      case S_IFLNK: // symlink
      {
         inode->i_op = App_getSymlinkInodeOps(app);
      } break;

      default: // pipes and other special files
      {
         inode->i_op = App_getSpecialInodeOps(app);
         init_special_inode(inode, kstat->mode, dev);
      } break;
   }


   unlock_new_inode(inode); // remove I_NEW flag, so the inode can be accessed by others

   return inode;


   // error occured
cleanup_entryInfo:
   EntryInfo_uninit(entryInfo);

   // found an existing inode
outNoCleanUp:
   return inode;
}

/**
 * Retrieves the attribs from the metadata node, generates an inode number (ID) and instantiates
 * a local version of the inode for the dentry.
 *
 * Note: This is a wrapper for _newInode() that also retrieves the the entry attributes (so the
 * entry must exist already on the server).
 *
 * @param dentry the new entry
 * @param entryInfo contained values (not the struct itself!) will be owned by the inode
 *    (or kfreed on error). So those values need to be allocated by the caller,
 *    but MUST NOT be free'ed by the caller.
 * @param fhgfsStat may be given to avoid extra remoting, or may be NULL (in which case a remote
 * stat will be done).
 * @return 0 on success, negative linux error code otherwise
 */
int __FhgfsOps_instantiateInode(struct dentry* dentry, EntryInfo* entryInfo, fhgfs_stat* fhgfsStat,
   FhgfsIsizeHints* iSizeHints)
{
   const char* logContext = "__FhgfsOps_instantiateInode";
   App* app = FhgfsOps_getApp(dentry->d_sb);
   int retVal = 0;
   fhgfs_stat fhgfsStatInternal;
   fhgfs_stat* actualStatInfo; // points either external given or internal stat data
   FhgfsOpsErr statRes = FhgfsOpsErr_SUCCESS;

   FhgfsOpsHelper_logOpDebug(app, dentry, NULL, logContext, "(%s)",
      fhgfsStat ? "with stat info" : "without stat info");
   IGNORE_UNUSED_VARIABLE(logContext);

   if(fhgfsStat)
      actualStatInfo = fhgfsStat;
   else
   { // no stat data given by caller => request from server
      actualStatInfo = &fhgfsStatInternal;

      FhgfsInode_initIsizeHints(NULL, iSizeHints);

      statRes = FhgfsOpsRemoting_statDirect(app, entryInfo, &fhgfsStatInternal);
   }


   if(statRes != FhgfsOpsErr_SUCCESS)
   { // error
      EntryInfo_uninit(entryInfo);

      retVal = FhgfsOpsErr_toSysErr(statRes);
   }
   else
   { // success (entry exists on server or was already given by caller)
      struct kstat kstat;
      struct inode* newInode;

      OsTypeConv_kstatFhgfsToOs(actualStatInfo, &kstat);

      kstat.ino = FhgfsInode_generateInodeID(dentry->d_sb, entryInfo->entryID,
         strlen(entryInfo->entryID) );

      newInode = __FhgfsOps_newInode(dentry->d_sb, &kstat, 0, entryInfo, iSizeHints);
      if(unlikely(!newInode || IS_ERR(newInode) ) )
         retVal = IS_ERR(newInode) ? PTR_ERR(newInode) : -EACCES;
      else
      { // new inode created
         d_instantiate(dentry, newInode);
      }
   }

   return retVal;
}

/**
 * Compare ID of given cachedInode with ID from comparison info arg.
 * This is called by iget5_locked() to make sure that we don't have an instance of a given inode
 * already (e.g. due to a hardlink or from an NFS handle) before we allocate a new one.
 *
 * Note: This is called with a spin_lock (inode_hash_lock) held, so we may not sleep.
 *
 * @param voidComparisonInfo opaque data pointer as passed to iget5_locked.
 * @return 0 if IDs don't match, !=0 on match.
 */
int __FhgfsOps_compareInodeID(struct inode* cachedInode, void* voidComparisonInfo)
{
   FhgfsInodeComparisonInfo* comparisonInfo = voidComparisonInfo;

   if(cachedInode->i_ino != comparisonInfo->inodeHash)
   { // entryID string hashes don't match

      return 0;
   }
   else
   { // inode hashes match => compare entryID strings

      FhgfsInode* fhgfsInode = BEEGFS_INODE(cachedInode);
      const char* searchEntryID = comparisonInfo->entryID;

      return FhgfsInode_compareEntryID(fhgfsInode, searchEntryID);
   }

}

/**
 * A dummy (to be passed to iget5_locked() ), which actually does nothing.
 *
 * We prefer to do initialization afterwards on the new inode (which is safe, because the inode
 * is still marked as new after iget5_locked returns). This method is called with the
 * inode_hash_lock spin lock held and we don't want to keep that lock longer than necessary.
 *
 * @return 0 on success, !=0 on error (specific error code is not checked by calling code).
 */
int __FhgfsOps_initNewInodeDummy(struct inode* newInode, void* newInodeInfo)
{
   return 0;
}


/**
 * Flush file content caches of the given inode.
 *
 * @return 0 on success, negative linux error code otherwise.
 */
int __FhgfsOps_flushInodeFileCache(App* app, struct inode* inode)
{
   Config* cfg = App_getConfig(app);
   FhgfsInode* fhgfsInode = BEEGFS_INODE(inode);

   if(inode->i_mapping)
   { // flush out file contents to servers for correct file size
      FhgfsInode* fhgfsInode = BEEGFS_INODE(inode);

      fhgfs_bool hasWriteHandle = FhgfsInode_hasWriteHandle(fhgfsInode);

      if (hasWriteHandle || FhgfsInode_getHasDirtyPages(fhgfsInode) )
      {
          int inodeWriteRes = write_inode_now(inode, 1);
          int filemapWaitRes = filemap_fdatawait(inode->i_mapping);

          if(unlikely(inodeWriteRes < 0 || filemapWaitRes < 0) )
            return (inodeWriteRes < 0) ? inodeWriteRes : filemapWaitRes;
      }
      else
      {
         // no need to flush
      }
   }

   if(S_ISREG(inode->i_mode) && Config_getTuneFileCacheTypeNum(cfg) == FILECACHETYPE_Buffered)
   { // regular file and buffered mode => flush write cache for correct file size
      FhgfsOpsErr flushRes = FhgfsOpsHelper_flushCache(app, fhgfsInode, fhgfs_false);
      if(unlikely(flushRes != FhgfsOpsErr_SUCCESS) )
         return FhgfsOpsErr_toSysErr(flushRes);
   }

   return 0;
}

/**
 * Called when we want to check whether the inode has changed on the server (or to update
 * the inode attribs after we've made changes to a file/dir).
 * (If it has changed, we invalidate the caches.)
 *
 * Note: This method will indirectly aqcuire the i_lock spinlock.
 *
 * @param fhgfsStat if NULL is given, this method will perform a remote stat; if not NULL, no
 * remoting is needed. (note that usually  _flushInodeFileCache() or similar should be called before
 * retrieving stat info).
 * @param iSizeHints is not initialized if fhgfsStat is NULL, but must not be a NULL pointer
 * @param noFlush if (unlikely) true the inode must not be flushed
 * @return 0 on success (validity), negative linux error code if no longer valid
 */
int __FhgfsOps_doRefreshInode(App* app, struct inode* inode, fhgfs_stat* fhgfsStat,
   FhgfsIsizeHints* iSizeHints, fhgfs_bool noFlush)
{
   const char* logContext = "FhgfsOps_refreshInode";
   Config* cfg = App_getConfig(app);

   int retVal = 0;
   struct kstat kstat;
   int flushRes;
   FhgfsOpsErr statRes;
   FhgfsInode* fhgfsInode = BEEGFS_INODE(inode);

   time_t oldMTime;
   loff_t oldSize;
   unsigned cacheElapsedMS;
   fhgfs_bool mtimeSizeInvalidate;
   fhgfs_bool timeoutInvalidate;

   FhgfsOpsHelper_logOpDebug(app, NULL, inode, logContext, "(%s)",
      fhgfsStat ? "with stat info" : "without stat info");

   IGNORE_UNUSED_VARIABLE(logContext);

   if(fhgfsStat)
   { // stat info given by caller => no remoting needed
      OsTypeConv_kstatFhgfsToOs(fhgfsStat, &kstat);
   }
   else
   { // no stat info given by caller => get it from server
      fhgfs_stat fhgfsStatInternal;

      if (likely(!noFlush) )
      {  // flush file contents for correct stat size
      flushRes = __FhgfsOps_flushInodeFileCache(app, inode);
      if(flushRes < 0)
         return flushRes;
      }

      FhgfsInode_initIsizeHints(fhgfsInode, iSizeHints);

      FhgfsInode_entryInfoReadLock(fhgfsInode); // LOCK EntryInfo

      statRes = FhgfsOpsRemoting_statDirect(app, FhgfsInode_getEntryInfo(fhgfsInode),
         &fhgfsStatInternal);

      FhgfsInode_entryInfoReadUnlock(fhgfsInode); // UNLOCK EntryInfo

      if(statRes != FhgfsOpsErr_SUCCESS)
      { // error or entry doesn't exist anymore
         retVal = FhgfsOpsErr_toSysErr(statRes);
         goto cleanup;
      }

      OsTypeConv_kstatFhgfsToOs(&fhgfsStatInternal, &kstat);
   }

   // stat succeeded, so the entry still exists

   if(inode->i_ino == BEEGFS_INODE_ROOT_INO)
   { // root node deserves special handling (less checking)
      __FhgfsOps_applyStatDataToInode(&kstat, NULL, inode);

      goto cleanup;
   }

   // check whether entry is still the same object type

   if(unlikely( (inode->i_mode & S_IFMT) != (kstat.mode & S_IFMT) ) )
   { // object type changed => limit damage by marking it as bad

      /* note: this is quite impossible since we're verifying by entryID and an ID should
         never ever be assigned to another object type. */

      umode_t savedMode = inode->i_mode; // save mode

      make_bad_inode(inode);

      inode->i_mode = savedMode; // restore mode

      printk_fhgfs(KERN_WARNING, "%s: Inode object type changed unexpectedly.\n", logContext);

      // invalidate cached pages
      if(!S_ISDIR(inode->i_mode) )
      {
         invalidate_remote_inode(inode);
      }

      retVal = -ENOENT;
      goto cleanup;
   }

   // apply new stat data

   spin_lock(&inode->i_lock); // I _ L O C K

   oldMTime = inode->i_mtime.tv_sec;
   oldSize = i_size_read(inode);

   __FhgfsOps_applyStatDataToInodeUnlocked(&kstat, iSizeHints, inode);

   // compare previous size/mtime to detect modifications by other clients

   mtimeSizeInvalidate =
      (inode->i_mtime.tv_sec != oldMTime) || (i_size_read(inode) != oldSize);
   cacheElapsedMS = Time_elapsedMS(&fhgfsInode->dataCacheTime);
   timeoutInvalidate = cacheElapsedMS > Config_getTunePageCacheValidityMS(cfg);

   if(S_ISDIR(inode->i_mode) )
   {
      Time_setToNow(&fhgfsInode->dataCacheTime);
   }
   else
   if(mtimeSizeInvalidate || timeoutInvalidate)
   { // file contents changed => invalidate non-dirty pages
      spin_unlock(&inode->i_lock); // I _ U N L O C K
      invalidate_remote_inode(inode); // might sleep => unlocked
      spin_lock(&inode->i_lock); // I _ R E L O C K

      Time_setToNow(&fhgfsInode->dataCacheTime); // after invalidate_... to avoid race condition
   }

   spin_unlock(&inode->i_lock); // I _ U N L O C K


   // clean up
cleanup:

   return retVal;
}


/**
 * Note: Refreshes inode and mapping only if cache validity timeout expired.
 *
 * @return 0 on success (validity), negative linux error code if no longer valid
 */
int __FhgfsOps_revalidateMapping(App* app, struct inode* inode)
{
   const char* logContext = "FhgfsOps_revalidateMapping";
   Config* cfg = App_getConfig(app);

   FhgfsInode* fhgfsInode = BEEGFS_INODE(inode);

   unsigned cacheElapsedMS;
   fhgfs_bool timeoutInvalidate;
   fhgfs_bool needRefresh = fhgfs_false;

   FhgfsIsizeHints iSizeHints;

   FhgfsOpsHelper_logOp(Log_SPAM, app, NULL, inode, logContext);
   IGNORE_UNUSED_VARIABLE(logContext);

   spin_lock(&inode->i_lock); // I _ L O C K

   cacheElapsedMS = Time_elapsedMS(&fhgfsInode->dataCacheTime);
   timeoutInvalidate = cacheElapsedMS > Config_getTunePageCacheValidityMS(cfg);

   if(!S_ISDIR(inode->i_mode) && timeoutInvalidate)
      needRefresh = fhgfs_true;

   spin_unlock(&inode->i_lock); // I _ U N L O C K

   if(needRefresh)
      return __FhgfsOps_refreshInode(app, inode, NULL, &iSizeHints);

   return 0;
}

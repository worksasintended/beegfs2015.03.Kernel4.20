#include <linux/fs.h>
#include <linux/posix_acl_xattr.h>

#include "common/Common.h"
#include "FhgfsOpsInode.h"
#include "FhgfsOpsHelper.h"

#include "FhgfsXAttrHandlers.h"

#define FHGFS_XATTR_USER_PREFIX "user."


#ifdef KERNEL_HAS_POSIX_GET_ACL
/**
 * Called when an ACL Xattr is set. Responsible for setting the mode bits corresponding to the
 * ACL mask.
 */
static int FhgfsXAttrSetACL(struct dentry *dentry, const char *name, const void *value, size_t size,
      int flags, int handler_flags)
{
   struct inode* inode = dentry->d_inode;
   char* attrName;

   FhgfsOpsHelper_logOpDebug(FhgfsOps_getApp(dentry->d_sb), dentry, NULL, __func__, "Called.");

   // Enforce an empty name here (which means the name of the Xattr has to be
   // fully given by the POSIX_ACL_XATTR_... defines)
   if(strcmp(name, "") )
      return -EINVAL;

   if(!inode_owner_or_capable(inode) )
      return -EPERM;

   if(S_ISLNK(inode->i_mode) )
      return -EOPNOTSUPP;

   if(handler_flags == ACL_TYPE_ACCESS)
   {
      struct posix_acl* acl;
      struct iattr attr;
      int aclEquivRes;
      int setAttrRes;

      // if we set the access ACL, we also need to update the file mode permission bits.
      attr.ia_mode = inode->i_mode;
      attr.ia_valid = ATTR_MODE;

      acl = os_posix_acl_from_xattr(value, size);

      if(IS_ERR(acl) )
         return PTR_ERR(acl);

      aclEquivRes = posix_acl_equiv_mode(acl, &attr.ia_mode);
      if(aclEquivRes == 0) // ACL can be exactly represented by file mode permission bits
      {
         value = NULL;
      }
      else if(aclEquivRes < 0)
      {
         posix_acl_release(acl);
         return -EINVAL;
      }

      setAttrRes = FhgfsOps_setattr(dentry, &attr);
      if(setAttrRes < 0)
         return setAttrRes;

      posix_acl_release(acl);

      // Name of the Xattr to be set later
      attrName = POSIX_ACL_XATTR_ACCESS;
   }
   else if(handler_flags == ACL_TYPE_DEFAULT)
   {
      attrName = POSIX_ACL_XATTR_DEFAULT;
      // Note: The default acl is not reflected in any file mode permission bits.
   }
   else
      return -EOPNOTSUPP;

   if(value)
      return FhgfsOps_setxattr(dentry, attrName, value, size, flags);
   else // value == NULL: Remove the ACL extended attribute.
      return FhgfsOps_removexattr(dentry, attrName);
}

/**
 * The get-function of the xattr handler which handles the POSIX_ACL_XATTR_ACCESS and
 * POSIX_ACL_XATTR_DEFAULT xattrs.
 * @param name has to be a pointer to an empty string ("").
 */
int FhgfsXAttrGetACL(struct dentry* dentry, const char* name, void* value, size_t size,
      int handler_flags)
{
   char* attrName;

   FhgfsOpsHelper_logOpDebug(FhgfsOps_getApp(dentry->d_sb), dentry, NULL, __func__, "Called.");

   // For simplicity we enforce an empty name here (which means the name of the Xattr has to be
   // fully given by the POSIX_ACL_XATTR_... defines)
   if(strcmp(name, "") )
      return -EINVAL;

   if(handler_flags == ACL_TYPE_ACCESS)
      attrName = POSIX_ACL_XATTR_ACCESS;
   else if(handler_flags == ACL_TYPE_DEFAULT)
      attrName = POSIX_ACL_XATTR_DEFAULT;
   else
      return -EOPNOTSUPP;

   return FhgfsOps_getxattr(dentry, attrName, value, size);
}
#endif // KERNEL_HAS_POSIX_GET_ACL

/**
 * The get-function which is used for all the user.* xattrs.
 */
#ifdef KERNEL_HAS_DENTRY_XATTR_HANDLER
int FhgfsXAttr_getUser(struct dentry* dentry, const char* name, void* value, size_t size,
      int handler_flags)
#else
int FhgfsXAttr_getUser(struct inode* inode, const char* name, void* value, size_t size)
#endif // KERNEL_HAS_DENTRY_XATTR_HANDLER
{
   FhgfsOpsErr res;
   char* prefixedName = os_kmalloc(strlen(name) + sizeof(FHGFS_XATTR_USER_PREFIX) );
   // Note: strlen does not count the terminating '\0', but sizeof does. So we have space for
   // exactly one '\0' which coincidally is just what we need.

#ifdef KERNEL_HAS_DENTRY_XATTR_HANDLER
   FhgfsOpsHelper_logOpDebug(FhgfsOps_getApp(dentry->d_sb), dentry, NULL, __func__,
      "(name: %s; size: %u)", name, size);
#else
   FhgfsOpsHelper_logOpDebug(FhgfsOps_getApp(inode->i_sb), NULL, inode, __func__,
      "(name: %s; size: %u)", name, size);
#endif // KERNEL_HAS_DENTRY_XATTR_HANDLER

   // add name prefix which has been removed by the generic function
   if(!prefixedName)
      return -ENOMEM;

   strcpy(prefixedName, FHGFS_XATTR_USER_PREFIX);
   strcpy(prefixedName + sizeof(FHGFS_XATTR_USER_PREFIX) - 1, name); // sizeof-1 to remove the '\0'

#ifdef KERNEL_HAS_DENTRY_XATTR_HANDLER
   res = FhgfsOps_getxattr(dentry, prefixedName, value, size);
#else
   res = FhgfsOps_getxattr(inode, prefixedName, value, size);
#endif // KERNEL_HAS_DENTRY_XATTR_HANDLER

   os_kfree(prefixedName);
   return res;
}

/**
 * The set-function which is used for all the user.* xattrs.
 */
#ifdef KERNEL_HAS_DENTRY_XATTR_HANDLER
int FhgfsXAttr_setUser(struct dentry* dentry, const char* name, const void* value, size_t size,
   int flags, int handler_flags)
#else
int FhgfsXAttr_setUser(struct inode* inode, const char* name, const void* value, size_t size,
   int flags)
#endif // KERNEL_HAS_DENTRY_XATTR_HANDLER
{
   FhgfsOpsErr res;
   char* prefixedName = os_kmalloc(strlen(name) + sizeof(FHGFS_XATTR_USER_PREFIX) );


#ifdef KERNEL_HAS_DENTRY_XATTR_HANDLER
   FhgfsOpsHelper_logOpDebug(FhgfsOps_getApp(dentry->d_sb), dentry, NULL, __func__,
      "(name: %s)", name);
#else
   FhgfsOpsHelper_logOpDebug(FhgfsOps_getApp(inode->i_sb), NULL, inode, __func__,
      "(name: %s)", name);
#endif // KERNEL_HAS_DENTRY_XATTR_HANDLER

   // add name prefix which has been removed by the generic function
   if(!prefixedName)
      return -ENOMEM;
   strcpy(prefixedName, FHGFS_XATTR_USER_PREFIX);
   strcpy(prefixedName + sizeof(FHGFS_XATTR_USER_PREFIX) - 1, name); // sizeof-1 to remove the '\0'

#ifdef KERNEL_HAS_DENTRY_XATTR_HANDLER
   res = FhgfsOps_setxattr(dentry, prefixedName, value, size, flags);
#else
   res = FhgfsOps_setxattr(inode, prefixedName, value, size, flags);
#endif // KERNEL_HAS_DENTRY_XATTR_HANDLER

   os_kfree(prefixedName);
   return res;
}

#ifdef KERNEL_HAS_POSIX_GET_ACL
const struct xattr_handler fhgfs_xattr_acl_access_handler =
{
   .prefix = POSIX_ACL_XATTR_ACCESS,
   .flags  = ACL_TYPE_ACCESS,
   .list   = NULL,
   .get    = FhgfsXAttrGetACL,
   .set    = FhgfsXAttrSetACL,
};

const struct xattr_handler fhgfs_xattr_acl_default_handler =
{
   .prefix = POSIX_ACL_XATTR_DEFAULT,
   .flags  = ACL_TYPE_DEFAULT,
   .list   = NULL,
   .get    = FhgfsXAttrGetACL,
   .set    = FhgfsXAttrSetACL,
};
#endif // KERNEL_HAS_POSIX_GET_ACL

#ifdef KERNEL_HAS_CONST_XATTR_HANDLER
const struct xattr_handler fhgfs_xattr_user_handler =
#else
struct xattr_handler fhgfs_xattr_user_handler =
#endif // KERNEL_HAS_CONST_XATTR_HANDLER
{
   .prefix = FHGFS_XATTR_USER_PREFIX,
   .list   = NULL,
   .set    = FhgfsXAttr_setUser,
   .get    = FhgfsXAttr_getUser,
};

#ifdef KERNEL_HAS_POSIX_GET_ACL
#ifdef KERNEL_HAS_CONST_XATTR_HANDLER
const struct xattr_handler* fhgfs_xattr_handlers[] =
#else
struct xattr_handler* fhgfs_xattr_handlers[] =
#endif // KERNEL_HAS_CONST_XATTR_HANDLER
{
   &fhgfs_xattr_acl_access_handler,
   &fhgfs_xattr_acl_default_handler,
   &fhgfs_xattr_user_handler,
   NULL
};
#endif // KERNEL_HAS_POSIX_GET_ACL

#ifdef KERNEL_HAS_CONST_XATTR_HANDLER
const struct xattr_handler* fhgfs_xattr_handlers_noacl[] =
#else
struct xattr_handler* fhgfs_xattr_handlers_noacl[] =
#endif // KERNEL_HAS_CONST_XATTR_HANDLER
{
   &fhgfs_xattr_user_handler,
   NULL
};


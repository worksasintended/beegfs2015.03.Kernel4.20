/*
 * Ioctl helper functions
 */

#include <app/log/Logger.h>
#include <common/toolkit/list/UInt16List.h>
#include "IoctlHelper.h"

/**
 * Copy struct BeegfsIoctl_MkFile_Arg from user to kernel space
 *
 * Note: For simplicity, the calling function needs to free alloced outFileInfo members, even in
 * case of errors. Therefore, *outFileInfo is supposed to be memset() with 0 by the caller.
 *
 * @return 0 on success, negative linux error code otherwise
 */
long IoctlHelper_ioctlCreateFileCopyFromUser(App* app, void __user *argp,
   struct BeegfsIoctl_MkFile_Arg* outFileInfo)
{
   Logger* log = App_getLogger(app);
   const char* logContext = __func__;
   struct BeegfsIoctl_MkFile_Arg userFileInfo; // fileInfo still with pointers into user space

   memset(&userFileInfo, 0, sizeof(userFileInfo) ); // avoid clang warning

   if (copy_from_user(&userFileInfo, argp, sizeof(userFileInfo) ) )
      return -EFAULT;

   /* we cannot simply do: outFileInfo = userFileInfo, as that would overwrite all NULL pointers
    * and we use those NULL pointers to simplify free(), so we copy integers one by one */
   outFileInfo->ownerNodeID            = userFileInfo.ownerNodeID;
   outFileInfo->parentParentEntryIDLen = userFileInfo.parentParentEntryIDLen;
   outFileInfo->parentEntryIDLen       = userFileInfo.parentEntryIDLen;
   outFileInfo->parentNameLen          = userFileInfo.parentNameLen;
   outFileInfo->entryNameLen           = userFileInfo.entryNameLen;
   outFileInfo->symlinkToLen           = userFileInfo.symlinkToLen;
   outFileInfo->mode                   = userFileInfo.mode;
   outFileInfo->uid                    = userFileInfo.uid;
   outFileInfo->gid                    = userFileInfo.gid;
   outFileInfo->numTargets             = userFileInfo.numTargets;
   outFileInfo->prefTargetsLen         = userFileInfo.prefTargetsLen;
   outFileInfo->fileType               = userFileInfo.fileType;

   /* Now copy and alloc all char* to kernel space */

   if (outFileInfo->parentParentEntryIDLen == 0)
      return -EINVAL;
   outFileInfo->parentParentEntryID = strndup_user(
      (const char __user *) userFileInfo.parentParentEntryID,
      outFileInfo->parentParentEntryIDLen);
   if (IS_ERR(outFileInfo->parentParentEntryID) )
   {
      int error = PTR_ERR(outFileInfo->parentParentEntryID);
      outFileInfo->parentParentEntryID = NULL;
      LOG_DEBUG_FORMATTED(log, Log_NOTICE, logContext, "Invalid parentParentEntryID string");
      return error;
   }

   if (outFileInfo->parentEntryIDLen == 0)
      return -EINVAL;
   outFileInfo->parentEntryID = strndup_user(
      (const char __user *) userFileInfo.parentEntryID,
      outFileInfo->parentEntryIDLen);
   if (IS_ERR(outFileInfo->parentEntryID) )
   {
      int error = PTR_ERR(outFileInfo->parentEntryID);
      outFileInfo->parentEntryID = NULL;
      LOG_DEBUG_FORMATTED(log, Log_NOTICE, logContext, "Invalid parentEntryID string");
      return error;
   }

   if (outFileInfo->parentNameLen == 0)
      return -EINVAL;
   outFileInfo->parentName = strndup_user(
      (const char __user *) userFileInfo.parentName,
      outFileInfo->parentNameLen);
   if (IS_ERR(outFileInfo->parentName) )
   {
      int error = PTR_ERR(outFileInfo->parentName);
      outFileInfo->parentName = NULL;
      LOG_DEBUG_FORMATTED(log, Log_NOTICE, logContext, "Invalid parentName string");
      return error;
   }

   if (outFileInfo->entryNameLen == 0)
      return -EINVAL;
   outFileInfo->entryName = strndup_user(
      (const char __user *) userFileInfo.entryName,
      outFileInfo->entryNameLen);
   if (IS_ERR(outFileInfo->entryName) )
   {
      int error = PTR_ERR(outFileInfo->entryName);
      outFileInfo->entryName = NULL;
      LOG_DEBUG_FORMATTED(log, Log_NOTICE, logContext, "Invalid entryName string");
      return error;
   }

   if (userFileInfo.fileType == DT_LNK && userFileInfo.symlinkToLen > 0)
   { // symlink
      outFileInfo->symlinkTo = strndup_user(
         (const char __user *) userFileInfo.symlinkTo,
         outFileInfo->symlinkToLen);
      if (IS_ERR(outFileInfo->symlinkTo) )
      {
         int error = PTR_ERR(outFileInfo->symlinkTo);
         outFileInfo->symlinkTo = NULL;
         LOG_DEBUG_FORMATTED(log, Log_NOTICE, logContext, "Invalid symlinkTo string");
         return error;
      }
   }

   // copy prefTargets array and verify it...

   /* Note: try not to exceed a page, as kmalloc might fail for high order allocations. However,
    *       that should only happen with very high number of stripes and we will limit the number
    *       in fhgfs-ctl. */

   // check if given num targets actually fits inside given targets array
   // (note: +1 for terminating 0 element)
   if(outFileInfo->prefTargetsLen < ( (outFileInfo->numTargets + 1) * (int) sizeof(uint16_t) ) )
   {
      Logger_logFormatted(log, Log_WARNING, logContext,
         "prefTargetsLen(=%d) too small for given numTargets(=%d+1)",
         outFileInfo->prefTargetsLen,
         outFileInfo->numTargets);
      return -EINVAL;
   }

   // copy prefTargets to kernel buffer based on raw prefTargetsLen
   outFileInfo->prefTargets = memdup_user( (char __user *) userFileInfo.prefTargets,
      outFileInfo->prefTargetsLen);
   if (IS_ERR(outFileInfo->prefTargets) )
   {
      int error = PTR_ERR(outFileInfo->prefTargets);
      LOG_DEBUG_FORMATTED(log, Log_NOTICE, logContext, "Unable to copy prefTargets array");
      outFileInfo->prefTargets = NULL;
      return error;
   }

   // check if prefTargets given by user space has a terminating zero as last array element
   // (note: not +1, because numTargets count does not include terminating zero element)
   if(outFileInfo->prefTargets[outFileInfo->numTargets] != 0)
   {
      Logger_logFormatted(log, Log_WARNING, logContext, "prefTargets array is not zero-terminated");
      return -EINVAL;
   }


   return 0;
}

/**
 * Copy targets from fileInfo->prefTargets array to outCreateInfo->preferredStorageTargets list
 *
 * @param outCreateInfo outCreateInfo->preferredStorageTargets will be alloced and needs to be freed
 * by caller even in case an error is returned (if it is !=NULL)
 * @return 0 on success, negative linux error code otherwise
 */
int IoctlHelper_ioctlCreateFileTargetsToList(App* app, struct BeegfsIoctl_MkFile_Arg* fileInfo,
   struct CreateInfo* outCreateInfo)
{
   Logger* log = App_getLogger(app);
   const char* logContext = __func__;

   int i;

   // construct output list

   outCreateInfo->preferredStorageTargets = UInt16List_construct();
   if(unlikely(!outCreateInfo->preferredStorageTargets) )
      return -ENOMEM;

   // copy all targets from array to list

   for(i=0; i < fileInfo->numTargets; i++)
   {
      uint16_t currentTarget = (fileInfo->prefTargets)[i];

      if(unlikely(!currentTarget) )
      { // invalid target in the middle of the array
         Logger_logFormatted(log, Log_WARNING, logContext,
            "Invalid preferred target at this array index: %d (numTargets: %d)\n",
            i, fileInfo->numTargets);

         return -EINVAL;
      }

      UInt16List_append(outCreateInfo->preferredStorageTargets, currentTarget);
   }

   return 0;
}


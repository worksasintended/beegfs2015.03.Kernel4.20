#include "Program.h"
#include <app/App.h>
#include <common/toolkit/SocketTk.h>
#include <common/Common.h>
#include <filesystem/FhgfsOpsSuper.h>
#include <filesystem/FhgfsOpsInode.h>
#include <filesystem/FhgfsOpsPages.h>
#include <filesystem/ProcFs.h>
#include <components/worker/RWPagesWork.h>
#include <net/filesystem/FhgfsOpsRemoting.h>


Program program;


int Program_main(void)
{
//   int appRes;
   fhgfs_bool assumptionsRes;
   fhgfs_bool sockRes;
   fhgfs_bool cacheRes;
   fhgfs_bool rwPageQueueRes;
   fhgfs_bool msgBufCacheRes;
   fhgfs_bool pageVecCacheRes;

   assumptionsRes = os_checkCompileTimeAssumptions();
   if(!assumptionsRes)
   {
      printk_fhgfs(KERN_WARNING, "Compile-time assumptions were wrong. "
         "Please contact support.");
      return APPCODE_PROGRAM_ERROR;
   }

   sockRes = SocketTk_initOnce();
   if(!sockRes)
   {
      printk_fhgfs(KERN_WARNING, "SocketTk initialization failed\n");
      return APPCODE_PROGRAM_ERROR;
   }

   cacheRes = FhgfsOps_initInodeCache();
   if(!cacheRes)
   {
      printk_fhgfs(KERN_WARNING, "Inode cache initialization failed\n");
      return APPCODE_PROGRAM_ERROR;
   }

   rwPageQueueRes = RWPagesWork_initworkQueue();
   if(!rwPageQueueRes)
   {
      printk_fhgfs(KERN_WARNING, "Page work queue registration failed\n");
      return APPCODE_PROGRAM_ERROR;
   }

   msgBufCacheRes = FhgfsOpsRemoting_initMsgBufCache();
   if(!msgBufCacheRes)
   {
      printk_fhgfs(KERN_WARNING, "Message cache initialization failed\n");
      return APPCODE_PROGRAM_ERROR;
   }

   pageVecCacheRes = FhgfsOpsPages_initPageListVecCache();
   if(!pageVecCacheRes)
   {
      printk_fhgfs(KERN_WARNING, "PageVec cache initialization failed\n");
      return APPCODE_PROGRAM_ERROR;
   }

   program.registerRes = FhgfsOps_registerFilesystem();
   if(program.registerRes)
   {
      printk_fhgfs(KERN_WARNING, "File system registration failed\n");
      return APPCODE_PROGRAM_ERROR;
   }

   printk_fhgfs(KERN_INFO, "File system registered. Type: %s. Version: %s\n",
      BEEGFS_MODULE_NAME_STR, App_getVersionStr() );

   ProcFs_createGeneralDir();

   return 0;
}

void Program_exit(void)
{
   ProcFs_removeGeneralDir();

   if(!program.registerRes)
   {
      int unregisterRes;

      unregisterRes = FhgfsOps_unregisterFilesystem();
      if(unregisterRes)
         printk_fhgfs(KERN_WARNING, "File system deregistration failed\n");
   }

   FhgfsOps_destroyInodeCache();
   FhgfsOpsRemoting_destroyMsgBufCache();
   FhgfsOpsPages_destroyPageListVecCache();

   RWPagesWork_destroyWorkQueue();
   

   SocketTk_uninitOnce();

   printk_fhgfs(KERN_INFO, "BeeGFS client unloaded.\n");
}

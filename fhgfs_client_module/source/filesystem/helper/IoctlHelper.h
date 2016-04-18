/*
 * Ioctl helper functions
 */

#ifndef IOCTLHELPER_H_
#define IOCTLHELPER_H_

#include <app/App.h>
#include <app/config/Config.h>
#include <os/OsCompat.h>
#include <filesystem/FhgfsOpsSuper.h>
#include <filesystem/FhgfsOpsHelper.h>
#include <filesystem/FhgfsOpsIoctl.h>

#ifdef CONFIG_COMPAT
#include <asm/compat.h>
#include <filesystem/FhgfsOpsInode.h>
#endif

#include <linux/mount.h>
#include <opentk/gpl_compat/GPLCompat.h>


extern long IoctlHelper_ioctlCreateFileCopyFromUser(App* app, void __user *argp,
   struct BeegfsIoctl_MkFile_Arg* outFileInfo);

extern int IoctlHelper_ioctlCreateFileTargetsToList(App* app, struct BeegfsIoctl_MkFile_Arg* fileInfo,
   struct CreateInfo* outCreateInfo);


#endif /* IOCTLHELPER_H_ */


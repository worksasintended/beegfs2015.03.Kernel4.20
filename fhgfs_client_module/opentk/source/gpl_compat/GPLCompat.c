/*
 * Wrapper functions for EXPORT_SYMBOL_GPL kernel functions
 */

#include <opentk/OpenTk_Common.h>
#include <common/Common.h>
#include <linux/mount.h>
#include <linux/fs.h>
#include <linux/pagemap.h>

/**
 * Wrapper function for mnt_want_write()
 */
int os_mnt_want_write(struct vfsmount *mnt)
{
   #ifndef KERNEL_HAS_MNT_WANT_WRITE
      if (mnt->mnt_sb->s_flags & MS_RDONLY)
         return -EROFS;
      else
         return 0;
   #else
      // >= 2.6.26
      return mnt_want_write(mnt);
   #endif

}

/**
 * Wrapper function for mnt_drop_write()
 */
void os_mnt_drop_write(struct vfsmount *mnt)
{
   #ifndef KERNEL_HAS_MNT_WANT_WRITE
      // noop before 2.6.26, as those didn't have mnt_want_write yet and so didn't take a
      // write ref-counter yet.
      return;
   #else
      // >= 2.6.26
      return mnt_drop_write(mnt);
   #endif

}

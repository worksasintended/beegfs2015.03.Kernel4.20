/*
 * fhgfs_client_opentk is declared as GPL and so we use it as bridge between the non-GPL fhgfs
 * module and GPL-only kernel code.
 */
#ifndef GPLCOMPAT_H_
#define GPLCOMPAT_H_


struct vfsmount; // forward declaration to avoid inclusion of linux headers in this header file


extern int os_mnt_want_write(struct vfsmount *mnt);
extern void os_mnt_drop_write(struct vfsmount *mnt);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
int os_addToPageCacheLRU(struct page *page, struct address_space *mapping,
   pgoff_t offset, gfp_t gfp_mask);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
int os_GenericDropInode(struct inode *inode);
#else
void os_GenericDropInode(struct inode *inode);
#endif

#endif /* GPLCOMPAT_H_ */

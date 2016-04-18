/*
 * Compatibility functions for older Linux versions
 */

#ifndef OSCOMPAT_H_
#define OSCOMPAT_H_

#include <common/Common.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <asm/kmap_types.h>
#include <linux/compat.h>
#include <linux/list.h>
#include <linux/posix_acl_xattr.h>

#ifdef KERNEL_HAS_TASK_IO_ACCOUNTING
   #include <linux/task_io_accounting_ops.h>
#endif


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
   extern void *memdup_user(const void __user *src, size_t len);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,3,0)
   extern struct dentry *d_make_root(struct inode *root_inode);
#endif

#ifndef KERNEL_HAS_TASK_IO_ACCOUNTING
   static inline void task_io_account_read(size_t bytes);
   static inline void task_io_account_write(size_t bytes);
#endif

static inline int os_generic_permission(struct inode *inode, int mask);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34)
   extern int bdi_setup_and_register(struct backing_dev_info *bdi, char *name, unsigned int cap);
#endif

/**
 * generic_permission() compatibility function
 *
 * NOTE: Only kernels > 2.6.32 do have inode->i_op->check_acl, but as we do not
 *       support it anyway for now, we do not need a complete kernel version check for it.
 *       Also, in order to skip useless pointer references we just pass NULL here.
 */
int os_generic_permission(struct inode *inode, int mask)
{
   #if LINUX_VERSION_CODE >= KERNEL_VERSION(3,1,0)
      return generic_permission(inode, mask);
   #elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
      return generic_permission(inode, mask, 0, NULL);
   #else
      return generic_permission(inode, mask, NULL);
   #endif
}


#ifndef KERNEL_HAS_TASK_IO_ACCOUNTING

/**
 * No-op if kernel version does not support this.
 */
void task_io_account_read(size_t bytes)
{
}

/**
 * No-op if kernel version does not support this.
 */
void task_io_account_write(size_t bytes)
{
}

#endif // KERNEL_HAS_TASK_IO_ACCOUNTING


#ifndef KERNEL_HAS_D_MATERIALISE_UNIQUE
extern struct dentry* d_materialise_unique(struct dentry *dentry, struct inode *inode);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
/**
 * Taken from ext3 dir.c. is_compat_task() does work for all kernels, although it was already there.
 * So we are conservativ and only allow it for recent kernels.
 */
static inline int is_32bit_api(void)
{
#ifdef CONFIG_COMPAT
   return is_compat_task();
#else
   return (BITS_PER_LONG == 32);
#endif
}
#else
static inline int is_32bit_api(void)
{
   return (BITS_PER_LONG == 32);
}
#endif // LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)


#ifndef KERNEL_HAS_GET_UNALIGNED_LE
#include "AccessOk.h"
#endif

#ifndef KERNEL_HAS_I_UID_READ
static inline uid_t i_uid_read(const struct inode *inode)
{
   return inode->i_uid;
}

static inline gid_t i_gid_read(const struct inode *inode)
{
   return inode->i_gid;
}

static inline void i_uid_write(struct inode *inode, uid_t uid)
{
   inode->i_uid = uid;
}

static inline void i_gid_write(struct inode *inode, gid_t gid)
{
   inode->i_gid = gid;
}

#endif // KERNEL_HAS_I_UID_READ


#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,23)
struct kmem_cache* OsCompat_initKmemCache(const char* cacheName, size_t cacheSize,
   void initFuncPtr(void* initObj, struct kmem_cache* cache, unsigned long flags) );
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,26)
struct kmem_cache* OsCompat_initKmemCache(const char* cacheName, size_t cacheSize,
   void initFuncPtr(struct kmem_cache* cache, void* initObj) );
#else
struct kmem_cache* OsCompat_initKmemCache(const char* cacheName, size_t cacheSize,
   void initFuncPtr(void* initObj) );
#endif // LINUX_VERSION_CODE


// added to 3.13, backported to -stable
#ifndef list_next_entry
/**
 * list_next_entry - get the next element in list
 * @pos: the type * to cursor
 * @member: the name of the list_struct within the struct.
 */
#define list_next_entry(pos, member) \
   list_entry((pos)->member.next, typeof(*(pos)), member)
#endif


#ifndef KERNEL_HAS_LIST_IS_LAST
/**
 * list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int list_is_last(const struct list_head *list,
            const struct list_head *head)
{
   return list->next == head;
}
#endif // KERNEL_HAS_LIST_IS_LAST

#ifndef list_first_entry
/**
 * list_first_entry - get the first element from a list
 * @ptr: the list head to take the element from.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define list_first_entry(ptr, type, member) \
   list_entry((ptr)->next, type, member)
#endif // list_first_entry


#ifndef KERNEL_HAS_CLEAR_PAGE_LOCKED
static inline void __clear_page_locked(struct page *page)
{
   __clear_bit(PG_locked, &page->flags);
}
#endif // KERNEL_HAS_CLEAR_PAGE_LOCKED

static inline struct posix_acl* os_posix_acl_from_xattr(const void* value, size_t size)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
   return posix_acl_from_xattr(value, size);
#else
   return posix_acl_from_xattr(&init_user_ns, value, size);
#endif
}

static inline int os_posix_acl_to_xattr(const struct posix_acl* acl, void* buffer, size_t size)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
   return posix_acl_to_xattr(acl, buffer, size);
#else
   return posix_acl_to_xattr(&init_user_ns, acl, buffer, size);
#endif
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,1,0)
# define os_generic_write_checks generic_write_checks
#else
static inline int os_generic_write_checks(struct file* filp, loff_t* offset, size_t* size,
   int isblk)
{
   struct iovec iov = { 0, *size };
   struct iov_iter iter;
   ssize_t checkRes;
   struct kiocb iocb;

   iov_iter_init(&iter, WRITE, &iov, 1, *size);
   init_sync_kiocb(&iocb, filp);
   iocb.ki_pos = *offset;

   checkRes = generic_write_checks(&iocb, &iter);
   if(checkRes < 0)
      return checkRes;

   *offset = iocb.ki_pos;
   *size = iter.count;

   return 0;
}
#endif

#endif /* OSCOMPAT_H_ */

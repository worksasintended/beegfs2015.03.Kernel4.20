/*
 * Compatibility functions for older Linux versions
 */

#ifndef OSCOMPAT_H_
#define OSCOMPAT_H_

#include <common/Common.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/uio.h>
#include <asm/kmap_types.h>
#include <linux/compat.h>
#include <linux/list.h>
#include <linux/posix_acl_xattr.h>
#include <linux/swap.h>
#include <linux/writeback.h>

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
# ifdef in_compat_syscall
   return in_compat_syscall();
# else
   return is_compat_task();
# endif
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

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,16,0)
static inline void page_endio(struct page *page, int rw, int err)
{
   if (rw == READ)
   {
      if (!err)
      {
         SetPageUptodate(page);
      }
      else
      {
         ClearPageUptodate(page);
         SetPageError(page);
      }

      unlock_page(page);
   }
   else
   { /* rw == WRITE */
      if (err)
      {
         SetPageError(page);
         if (page->mapping)
            mapping_set_error(page->mapping, err);
      }

      end_page_writeback(page);
   }
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
static inline int os_write_cache_pages(struct address_space* mapping, struct writeback_control* wbc,
   writepage_t writepage, void* data)
{
   return write_cache_pages(mapping, wbc, writepage, data);
}
#else
/* not adding a writepage_t typedef here because it already exists in older kernels */
static inline int os_write_cache_pages(struct address_space* mapping, struct writeback_control* wbc,
   int (*writepage)(struct page*, struct writeback_control*, void*), void* data)
{
   struct backing_dev_info* bdi = mapping->backing_dev_info;
   int ret = 0;
   int done = 0;
   struct pagevec pvec;
   int nr_pages;
   pgoff_t index;
   pgoff_t end;		/* Inclusive */
   int scanned = 0;
   int range_whole = 0;

   if(wbc->nonblocking && bdi_write_congested(bdi) )
   {
      wbc->encountered_congestion = 1;
      return 0;
   }

   pagevec_init(&pvec, 0);
   if(wbc->range_cyclic)
   {
      index = mapping->writeback_index; /* Start from prev offset */
      end = -1;
   }
   else
   {
      index = wbc->range_start >> PAGE_CACHE_SHIFT;
      end = wbc->range_end >> PAGE_CACHE_SHIFT;
      if(wbc->range_start == 0 && wbc->range_end == LLONG_MAX)
         range_whole = 1;

      scanned = 1;
   }

retry:
   while(!done && (index <= end) &&
         (nr_pages = pagevec_lookup_tag(&pvec, mapping, &index,
                                        PAGECACHE_TAG_DIRTY,
                                        min(end - index, (pgoff_t)PAGEVEC_SIZE-1) + 1) ) )
   {
      unsigned i;

      scanned = 1;
      for(i = 0; i < nr_pages; i++)
      {
         struct page *page = pvec.pages[i];

         /*
          * At this point we hold neither mapping->tree_lock nor
          * lock on the page itself: the page may be truncated or
          * invalidated (changing page->mapping to NULL), or even
          * swizzled back from swapper_space to tmpfs file
          * mapping
          */
         lock_page(page);

         if(unlikely(page->mapping != mapping) )
         {
            unlock_page(page);
            continue;
         }

         if(!wbc->range_cyclic && page->index > end)
         {
            done = 1;
            unlock_page(page);
            continue;
         }

         if(wbc->sync_mode != WB_SYNC_NONE)
            wait_on_page_writeback(page);

         if(PageWriteback(page) || !clear_page_dirty_for_io(page) )
         {
            unlock_page(page);
            continue;
         }

         ret = (*writepage)(page, wbc, data);

         if(unlikely(ret == AOP_WRITEPAGE_ACTIVATE) )
            unlock_page(page);

         if(ret || (--(wbc->nr_to_write) <= 0) )
            done = 1;

         if(wbc->nonblocking && bdi_write_congested(bdi) )
         {
            wbc->encountered_congestion = 1;
            done = 1;
         }
      }

      pagevec_release(&pvec);
      cond_resched();
   }

   if(!scanned && !done)
   {
      /*
       * We hit the last page and there is more work to be done: wrap
       * back to the start of the file
       */
      scanned = 1;
      index = 0;
      goto retry;
   }

   if (wbc->range_cyclic || (range_whole && wbc->nr_to_write > 0) )
      mapping->writeback_index = index;

   return ret;
}
#endif

#ifndef KERNEL_HAS_CURRENT_UMASK
#define current_umask() (current->fs->umask)
#endif

#ifndef XATTR_NAME_POSIX_ACL_ACCESS
# define XATTR_POSIX_ACL_ACCESS  "posix_acl_access"
# define XATTR_NAME_POSIX_ACL_ACCESS XATTR_SYSTEM_PREFIX XATTR_POSIX_ACL_ACCESS
# define XATTR_POSIX_ACL_DEFAULT  "posix_acl_default"
# define XATTR_NAME_POSIX_ACL_DEFAULT XATTR_SYSTEM_PREFIX XATTR_POSIX_ACL_DEFAULT
#endif

#endif /* OSCOMPAT_H_ */

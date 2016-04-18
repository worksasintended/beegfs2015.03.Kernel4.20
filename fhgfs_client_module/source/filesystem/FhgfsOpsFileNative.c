#include <app/App.h>
#include <app/log/Logger.h>
#include <linux/pagemap.h>
#include "FhgfsOpsFileNative.h"
#include "FhgfsOpsFile.h"
#include "FhgfsOpsHelper.h"
#include <net/filesystem/FhgfsOpsRemoting.h>
#include <fault-inject/fault-inject.h>
#include <os/OsCompat.h>
#include <linux/aio.h>
#include <linux/kref.h>

static int writepages_pool_init(void);

static void writepages_pool_release(void);

bool beegfs_native_init()
{
   if(writepages_pool_init() < 0)
      return false;

   return true;
}

void beegfs_native_release()
{
   writepages_pool_release();
}



/*
 * PVRs and ARDs use the Private and Checked bits of pages to determine which is attached to a
 * page. Once we support only kernels that have the Private2 bit, we should use Private2 instead
 * of Checked.
 * Note: this is to avoid problems with 64k pages on 32 bit machines, otherwise we could use
 * low-order bits of page->private to discriminate.
 */

enum
{
   PVR_FIRST_SHIFT = 0,
   PVR_FIRST_MASK = ~PAGE_CACHE_MASK << PVR_FIRST_SHIFT,

   PVR_LAST_SHIFT = PAGE_CACHE_SHIFT,
   PVR_LAST_MASK = ~PAGE_CACHE_MASK << PVR_LAST_SHIFT,
};

static void pvr_init(struct page* page)
{
   // pvr values *must* fit into the unsigned long private of struct page
   BUILD_BUG_ON(PVR_LAST_SHIFT + PAGE_CACHE_SHIFT > 8 * sizeof(unsigned long) );

   SetPagePrivate(page);
   ClearPageChecked(page);
   page->private = 0;
}

static bool pvr_present(struct page* page)
{
   return PagePrivate(page) && !PageChecked(page);
}

static void pvr_clear(struct page* page)
{
   ClearPagePrivate(page);
}

static unsigned pvr_get_first(struct page* page)
{
   return (page->private & PVR_FIRST_MASK) >> PVR_FIRST_SHIFT;
}

static unsigned pvr_get_last(struct page* page)
{
   return (page->private & PVR_LAST_MASK) >> PVR_LAST_SHIFT;
}

static void pvr_set_first(struct page* page, unsigned first)
{
   page->private &= ~PVR_FIRST_MASK;
   page->private |= (first << PVR_FIRST_SHIFT) & PVR_FIRST_MASK;
}

static void pvr_set_last(struct page* page, unsigned last)
{
   page->private &= ~PVR_LAST_MASK;
   page->private |= (last << PVR_LAST_SHIFT) & PVR_LAST_MASK;
}

static bool pvr_can_merge(struct page* page, unsigned first, unsigned last)
{
   unsigned old_first = pvr_get_first(page);
   unsigned old_last = pvr_get_last(page);

   if(first == old_last + 1 || last + 1 == old_first)
      return true;

   if(old_first <= first && first <= old_last)
      return true;

   if(old_first <= last && last <= old_last)
      return true;

   return false;
}

static void pvr_merge(struct page* page, unsigned first, unsigned last)
{
   if(pvr_get_first(page) > first)
      pvr_set_first(page, first);

   if(pvr_get_last(page) < last)
      pvr_set_last(page, last);
}



struct append_range_descriptor
{
   unsigned long firstPageIndex;

   loff_t fileOffset;
   loff_t firstPageStart;

   size_t size;
   size_t dataPresent;

   struct mutex mtx;

   struct kref refs;
};

static struct append_range_descriptor* ard_create(size_t size)
{
   struct append_range_descriptor* ard;

   ard = kmalloc(sizeof(*ard), GFP_NOFS);
   if(!ard)
      return NULL;

   ard->fileOffset = -1;
   ard->firstPageIndex = 0;
   ard->firstPageStart = -1;

   ard->size = size;
   ard->dataPresent = 0;

   mutex_init(&ard->mtx);

   kref_init(&ard->refs);

   return ard;
}

static void ard_kref_release(struct kref* kref)
{
   struct append_range_descriptor* ard = container_of(kref, struct append_range_descriptor, refs);

   mutex_destroy(&ard->mtx);
   kfree(ard);
}

static void ard_get(struct append_range_descriptor* ard)
{
   kref_get(&ard->refs);
}

static int ard_put(struct append_range_descriptor* ard)
{
   return kref_put(&ard->refs, ard_kref_release);
}

static struct append_range_descriptor* ard_from_page(struct page* page)
{
   if(!PagePrivate(page) || !PageChecked(page) )
      return NULL;

   return (struct append_range_descriptor*) page->private;
}

static struct append_range_descriptor* __mapping_ard(struct address_space* mapping)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,8,0)
   // guarantees that pointer casting round-trips properly. gotta love C
   BUILD_BUG_ON(__alignof__(struct append_range_descriptor) < __alignof(struct address_space) );
   // same field, same semantics, different type.
   return (struct append_range_descriptor*) mapping->assoc_mapping;
#else
   return mapping->private_data;
#endif
}

static struct append_range_descriptor* mapping_ard_get(struct address_space* mapping)
{
   struct append_range_descriptor* ard;

   spin_lock(&mapping->private_lock);
   {
      ard = __mapping_ard(mapping);
      if(ard)
         ard_get(ard);
   }
   spin_unlock(&mapping->private_lock);

   return ard;
}

static void __set_mapping_ard(struct address_space* mapping, struct append_range_descriptor* ard)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,8,0)
   mapping->assoc_mapping = (struct address_space*) ard;
#else
   mapping->private_data = ard;
#endif
}

static void set_mapping_ard(struct address_space* mapping, struct append_range_descriptor* ard)
{
   spin_lock(&mapping->private_lock);
   __set_mapping_ard(mapping, ard);
   spin_unlock(&mapping->private_lock);
}

static void ard_assign(struct page* page, struct append_range_descriptor* ard)
{
   if(!ard)
   {
      ard = ard_from_page(page);

      BUG_ON(!ard);

      if(atomic_read(&ard->refs.refcount) == 1)
      {
         /* last ref to ard is on a page, so it must be detached if it hasn't been yet.
          * a concurrent writer may still get a reference to the ard *exactly here*,
          * but then the ard will be allocated and the writer will drop it. */
         spin_lock(&page->mapping->private_lock);
         {
            if(__mapping_ard(page->mapping) == ard)
               __set_mapping_ard(page->mapping, NULL);
         }
         spin_unlock(&page->mapping->private_lock);
      }

      ard_put(ard);
      ClearPagePrivate(page);
      ClearPageChecked(page);
      page->private = 0;
      return;
   }

   BUG_ON(ard_from_page(page) );

   ard_get(ard);
   SetPagePrivate(page);
   SetPageChecked(page);
   page->private = (unsigned long) ard;
}



static void beegfs_drop_all_caches(struct inode* inode)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,12,0)
   // 2.6.32 and later have truncate_pagecache, but that was incorrect
   unmap_mapping_range(inode->i_mapping, 0, 0, 1);
   truncate_inode_pages(inode->i_mapping, 0);
   unmap_mapping_range(inode->i_mapping, 0, 0, 1);
#else
   truncate_pagecache(inode, 0);
#endif
}



static int beegfs_release_range(struct file* filp, loff_t first, loff_t last)
{
   int writeRes;

   // expand range to fit full pages
   first &= PAGE_CACHE_MASK;
   last |= ~PAGE_CACHE_MASK;

   clear_bit(AS_EIO, &filp->f_mapping->flags);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
   writeRes = filemap_write_and_wait(filp->f_mapping);
#else
   writeRes = filemap_write_and_wait_range(filp->f_mapping, first, last);
#endif
   if(writeRes < 0)
   {
      App* app = FhgfsOps_getApp(filp->f_dentry->d_sb);
      FhgfsOpsHelper_logOpMsg(3, app, filp->f_dentry, filp->f_mapping->host, __func__,
         "error %i during flush", writeRes);
      IGNORE_UNUSED_VARIABLE(app);
      return writeRes;
   }

   return 0;
}

// TODO: this should not be a full flush-and-invalidate
static int beegfs_acquire_range(struct file* filp, loff_t first, loff_t last)
{
   App* app = FhgfsOps_getApp(filp->f_dentry->d_sb);
   FhgfsIsizeHints iSizeHints;
   int err;

   // expand range to fit full pages
   first &= PAGE_CACHE_MASK;
   last |= ~PAGE_CACHE_MASK;

   err = beegfs_release_range(filp, first, last);
   if(err)
      return err;

   err = __FhgfsOps_refreshInode(app, file_inode(filp), NULL, &iSizeHints);
   if(err)
      return err;

   err = invalidate_inode_pages2_range(filp->f_mapping, first >> PAGE_CACHE_SHIFT,
      last >> PAGE_CACHE_SHIFT);
   return err;
}



static int beegfs_open(struct inode* inode, struct file* filp)
{
   App* app = FhgfsOps_getApp(filp->f_dentry->d_sb);
   int err;
   RemotingIOInfo ioInfo;

   FhgfsOpsHelper_logOp(5, app, filp->f_dentry, file_inode(filp), __func__);
   IGNORE_UNUSED_VARIABLE(app);

   err = FhgfsOps_open(inode, filp);
   if(err)
      return err;

   FsFileInfo_getIOInfo(__FhgfsOps_getFileInfo(filp), BEEGFS_INODE(inode), &ioInfo);
   if(ioInfo.pattern->chunkSize % PAGE_CACHE_SIZE)
   {
      FhgfsOpsHelper_logOpMsg(1, app, filp->f_dentry, inode, __func__,
         "chunk size is not a multiple of PAGE_SIZE!");
      FhgfsOps_release(inode, filp);
      return -EIO;
   }

   err = beegfs_acquire_range(filp, 0, -1);
   return err;
}

static int beegfs_flush(struct file* filp, fl_owner_t id)
{
   IGNORE_UNUSED_VARIABLE(id);

   return beegfs_release_range(filp, 0, -1);
}

static int beegfs_release(struct inode* inode, struct file* filp)
{
   if(test_bit(AS_EIO, &inode->i_mapping->flags) &&
         test_and_clear_bit(AS_EIO, &inode->i_mapping->flags) )
      beegfs_drop_all_caches(inode);

   return FhgfsOps_release(inode, filp);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
static ssize_t beegfs_file_write_iter(struct kiocb* iocb, struct iov_iter* from)
{
   struct iovec iov = BEEGFS_IOV_ITER_IOVEC(from);

   return generic_file_aio_write(iocb, iov.iov_base, iov.iov_len, iocb->ki_pos);
}
#elif LINUX_VERSION_CODE < KERNEL_VERSION(3,16,0)
static ssize_t beegfs_file_write_iter(struct kiocb* iocb, struct iov_iter* from)
{
   return generic_file_aio_write(iocb, BEEGFS_IOV_ITER_RAW(from), from->nr_segs, iocb->ki_pos);
}
#else
static ssize_t beegfs_file_write_iter(struct kiocb* iocb, struct iov_iter* from)
{
   return generic_file_write_iter(iocb, from);
}
#endif


static struct append_range_descriptor* beegfs_get_or_create_ard(struct address_space* mapping,
   loff_t begin, size_t newData)
{
   struct append_range_descriptor* ard;

   ard = mapping_ard_get(mapping);

   if(!ard)
      goto no_ard;

   mutex_lock(&ard->mtx);

   if(ard->fileOffset >= 0)
      goto replace_ard;

   if(ard->firstPageIndex * PAGE_CACHE_SIZE + ard->firstPageStart + ard->size < begin)
      goto replace_ard;

   ard->size += newData;

   mutex_unlock(&ard->mtx);
   return ard;

replace_ard:
   ard_put(ard);
   set_mapping_ard(mapping, NULL);
   mutex_unlock(&ard->mtx);

no_ard:
   ard = ard_create(newData);
   if(!ard)
      return ERR_PTR(-ENOMEM);

   set_mapping_ard(mapping, ard);
   return ard;
}

static ssize_t beegfs_write_iter(struct kiocb* iocb, struct iov_iter* from)
{
   struct file* filp = iocb->ki_filp;
   size_t size = from->count;
   struct FhgfsInode* inode = BEEGFS_INODE(filp->f_mapping->host);

   App* app = FhgfsOps_getApp(filp->f_dentry->d_sb);

   FhgfsOpsHelper_logOpDebug(app, filp->f_dentry, file_inode(filp), __func__,
      "(offset: %lld; size: %zu)", iocb->ki_pos, size);
   IGNORE_UNUSED_VARIABLE(app);
   IGNORE_UNUSED_VARIABLE(size);

   if(filp->f_flags & O_APPEND)
   {
      struct append_range_descriptor* ard;
      ssize_t written;

      Mutex_lock(&inode->appendMutex);
      do {
         /* if the append is too large, we may overflow the page cache. make sure concurrent
          * flushers will allocate the correct size on the storage servers if nothing goes wrong
          * during the actual append write. if errors do happen, we will produce a hole in the
          * file. */
         ard = beegfs_get_or_create_ard(inode->vfs_inode.i_mapping, iocb->ki_pos, size);
         if(IS_ERR(ard) )
         {
            written = PTR_ERR(ard);
            break;
         }

         written = beegfs_file_write_iter(iocb, from);

         mutex_lock(&ard->mtx);
         do {
            if(ard->fileOffset >= 0)
            {
               set_mapping_ard(inode->vfs_inode.i_mapping, NULL);
               ard->dataPresent += MAX(written, 0);
               i_size_write(&inode->vfs_inode, i_size_read(&inode->vfs_inode) + size);
               break;
            }

            if(written < 0)
               ard->size -= size;
            else
            {
               ard->size -= size - written;
               i_size_write(&inode->vfs_inode, iocb->ki_pos);
               ard->dataPresent += written;
            }

            if(ard->size == 0)
               set_mapping_ard(inode->vfs_inode.i_mapping, NULL);
         } while (0);
         mutex_unlock(&ard->mtx);

         ard_put(ard);
      } while (0);
      Mutex_unlock(&inode->appendMutex);

      return written;
   }

   return beegfs_file_write_iter(iocb, from);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,16,0)
static ssize_t beegfs_aio_write_iov(struct kiocb* iocb, const struct iovec* iov,
   unsigned long nr_segs, loff_t pos)
{
   struct iov_iter iter;
   size_t count = iov_length(iov, nr_segs);

   BEEGFS_IOV_ITER_INIT(&iter, WRITE, iov, nr_segs, count);

   return beegfs_write_iter(iocb, &iter);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
static ssize_t beegfs_aio_write(struct kiocb* iocb, const char __user* buf, size_t count,
   loff_t pos)
{
   struct iovec iov = {
      .iov_base = (char*) buf,
      .iov_len = count,
   };

   return beegfs_aio_write_iov(iocb, &iov, 1, pos);
}
#else
static ssize_t beegfs_aio_write(struct kiocb* iocb, const struct iovec* iov,
   unsigned long nr_segs, loff_t pos)
{
   return beegfs_aio_write_iov(iocb, iov, nr_segs, pos);
}
#endif
#endif


static int __beegfs_fsync(struct file* filp, loff_t start, loff_t end, int datasync)
{
   App* app = FhgfsOps_getApp(filp->f_dentry->d_sb);
   Config* cfg = App_getConfig(app);
   int err;

   FhgfsOpsHelper_logOp(5, app, filp->f_dentry, file_inode(filp), __func__);

   IGNORE_UNUSED_VARIABLE(start);
   IGNORE_UNUSED_VARIABLE(end);
   IGNORE_UNUSED_VARIABLE(datasync);

   err = beegfs_release_range(filp, start, end);
   if(err)
      return err;

   if(Config_getTuneRemoteFSync(cfg) )
   {
      RemotingIOInfo ioInfo;
      FhgfsOpsErr res;

      FsFileInfo_getIOInfo(__FhgfsOps_getFileInfo(filp), BEEGFS_INODE(file_inode(filp) ), &ioInfo);
      res = FhgfsOpsRemoting_fsyncfile(&ioInfo, true, true, false);
      if(res != FhgfsOpsErr_SUCCESS)
         return FhgfsOpsErr_toSysErr(res);
   }

   return 0;
}

#ifdef KERNEL_HAS_FSYNC_RANGE
static int beegfs_fsync(struct file* file, loff_t start, loff_t end, int datasync)
{
   return __beegfs_fsync(file, start, end, datasync);
}
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35)
static int beegfs_fsync(struct file* file, int datasync)
{
   return __beegfs_fsync(file, 0, LLONG_MAX, datasync);
}
#else
static int beegfs_fsync(struct file* file, struct dentry* dentry, int datasync)
{
   return __beegfs_fsync(file, 0, LLONG_MAX, datasync);
}
#endif

static int beegfs_flock(struct file* filp, int cmd, struct file_lock* flock)
{
   App* app = FhgfsOps_getApp(filp->f_dentry->d_sb);
   int err = -EINVAL;

   FhgfsOpsHelper_logOp(5, app, filp->f_dentry, file_inode(filp), __func__);
   IGNORE_UNUSED_VARIABLE(app);

   switch(flock->fl_type)
   {
      case F_RDLCK:
      case F_WRLCK:
         err = beegfs_acquire_range(filp, 0, -1);
         break;

      case F_UNLCK:
         err = beegfs_release_range(filp, 0, -1);
         break;
   }

   if(err)
      return err;

   return FhgfsOps_flock(filp, cmd, flock);
}

static int beegfs_lock(struct file* filp, int cmd, struct file_lock* flock)
{
   App* app = FhgfsOps_getApp(filp->f_dentry->d_sb);
   int err = -EINVAL;

   FhgfsOpsHelper_logOp(5, app, filp->f_dentry, file_inode(filp), __func__);
   IGNORE_UNUSED_VARIABLE(app);

   switch(flock->fl_type)
   {
      case F_RDLCK:
      case F_WRLCK:
         err = beegfs_acquire_range(filp, flock->fl_start, flock->fl_end);
         break;

      case F_UNLCK:
         err = beegfs_release_range(filp, flock->fl_start, flock->fl_end);
         break;
   }

   if(err)
      return err;

   return FhgfsOps_lock(filp, cmd, flock);
}

static int beegfs_mmap(struct file* filp, struct vm_area_struct* vma)
{
   App* app = FhgfsOps_getApp(filp->f_dentry->d_sb);
   int err = -EINVAL;

   FhgfsOpsHelper_logOp(5, app, filp->f_dentry, file_inode(filp), __func__);
   IGNORE_UNUSED_VARIABLE(app);

   err = beegfs_acquire_range(filp, 0, -1);
   if(err)
      return err;

   err = generic_file_mmap(filp, vma);
   return err;
}

const struct file_operations fhgfs_file_native_ops = {
   .open = beegfs_open,
   .flush = beegfs_flush,
   .release = beegfs_release,
   .fsync = beegfs_fsync,
   .llseek = FhgfsOps_llseek,
   .flock = beegfs_flock,
   .lock = beegfs_lock,
   .mmap = beegfs_mmap,

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,16,0)
   .read = do_sync_read,
   .write = do_sync_write,
   .aio_read = generic_file_aio_read,
   .aio_write = beegfs_aio_write,
#else
   .read_iter = generic_file_read_iter,
   .write_iter = beegfs_write_iter,
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,16,0)
   .splice_read  = generic_file_splice_read,
   .splice_write = iter_file_splice_write,
#else
   .splice_read  = generic_file_splice_read,
   .splice_write = generic_file_splice_write,
#endif
};



enum
{
   BEEGFS_WRITEPAGES_BLOCK_SIZE = 128,
};

struct beegfs_writepages_state
{
   struct page* pages[BEEGFS_WRITEPAGES_BLOCK_SIZE];
   struct iovec iov[BEEGFS_WRITEPAGES_BLOCK_SIZE];
   unsigned nr_pages;

   RemotingIOInfo* ioInfo;
   struct writeback_control* wbc;
   bool unlockPages;
};

static mempool_t* writepages_pool;

static int writepages_pool_init()
{
   writepages_pool = mempool_create_kmalloc_pool(1, sizeof(struct beegfs_writepages_state) );
   if(!writepages_pool)
      return -ENOMEM;

   return 0;
}

static void writepages_pool_release()
{
   if(writepages_pool)
      mempool_destroy(writepages_pool);
}

static int beegfs_allocate_ard(struct beegfs_writepages_state* state)
{
   struct append_range_descriptor* ard = ard_from_page(state->pages[0]);
   struct address_space* mapping = state->pages[0]->mapping;

   char zero = 0;
   struct iovec reserveIov = {
      .iov_len = 1,
      .iov_base = &zero,
   };
   int err = 0;

   mutex_lock(&ard->mtx);
   if(ard->fileOffset >= 0)
   {
      mutex_unlock(&ard->mtx);
      return 0;
   }

   err = FhgfsOpsHelper_appendfileVecOffset(BEEGFS_INODE(mapping->host), &reserveIov, 1,
      state->ioInfo, ard->size - 1, &ard->fileOffset);
   if (err < 0)
      err = FhgfsOpsErr_toSysErr(-err);
   else
      ard->fileOffset -= ard->size;

   mutex_unlock(&ard->mtx);
   return err;
}

static int beegfs_wps_prepare(struct beegfs_writepages_state* state, loff_t* offset, size_t* size)
{
   struct append_range_descriptor* ard;
   int err;
   int i;
   loff_t appendEnd;

   *size = 0;

   if(pvr_present(state->pages[0]) )
   {
      loff_t isize = i_size_read(state->pages[0]->mapping->host);

      *offset = page_offset(state->pages[0]) + pvr_get_first(state->pages[0]);

      for(i = 0; i < state->nr_pages; i++)
      {
         struct page* page = state->pages[i];
         unsigned length;

         if(page_offset(page) + pvr_get_last(page) >= isize)
            length = isize - page_offset(page) - pvr_get_first(page);
         else
            length = pvr_get_last(page) - pvr_get_first(page) + 1;

         state->iov[i].iov_base = page_address(page) + pvr_get_first(page);
         state->iov[i].iov_len = length;

         *size += length;
      }

      return 0;
   }

   ard = ard_from_page(state->pages[0]);
   BUG_ON(!ard);

   err = beegfs_allocate_ard(state);
   if(err < 0)
      return err;

   *offset = ard->fileOffset;
   appendEnd = ard->firstPageIndex * PAGE_CACHE_SIZE + ard->firstPageStart + ard->dataPresent;

   for(i = 0; i < state->nr_pages; i++)
   {
      struct page* page = state->pages[i];
      loff_t offsetInPage;

      if(page->index == ard->firstPageIndex)
         offsetInPage = ard->firstPageStart;
      else
         offsetInPage = 0;

      state->iov[i].iov_base = page_address(page) + offsetInPage;
      state->iov[i].iov_len = min_t(size_t, appendEnd - (page_offset(page) + offsetInPage),
            PAGE_CACHE_SIZE - offsetInPage);

      *size += state->iov[i].iov_len;
   }

   return 0;
}

static int beegfs_writepages_submit(struct beegfs_writepages_state* state)
{
   App* app;
   struct iov_iter iter;
   unsigned i;
   int result = 0;
   ssize_t written;
   loff_t offset;
   size_t size;

   if(state->nr_pages == 0)
      return 0;

   app = FhgfsOps_getApp(state->pages[0]->mapping->host->i_sb);

   FhgfsOpsHelper_logOpDebug(app, NULL, state->pages[0]->mapping->host, __func__,
      "first offset: %lld nr_pages %u", page_offset(state->pages[0]), state->nr_pages);
   IGNORE_UNUSED_VARIABLE(app);

   result = beegfs_wps_prepare(state, &offset, &size);
   if(result < 0)
      goto writes_done;

   if(BEEGFS_SHOULD_FAIL(writepage, 1) )
   {
      result = -EIO;
      goto artifical_write_error;
   }

   BEEGFS_IOV_ITER_INIT(&iter, WRITE, state->iov, state->nr_pages, size);
   written = FhgfsOpsRemoting_writefileVec(&iter, offset, state->ioInfo);

   if(written < 0)
   {
      result = FhgfsOpsErr_toSysErr(-written);

artifical_write_error:
      for(i = 0; i < state->nr_pages; i++)
      {
         page_endio(state->pages[i], WRITE, result);
         redirty_page_for_writepage(state->wbc, state->pages[i]);
      }

      goto out;
   }

   task_io_account_write(written);
   size = written;

writes_done:
   for(i = 0; i < state->nr_pages; i++)
   {
      struct page* page = state->pages[i];

      if(size >= state->iov[i].iov_len)
      {
         size -= state->iov[i].iov_len;

         if(pvr_present(page) )
            pvr_clear(page);
         else
            ard_assign(page, NULL);

         page_endio(page, WRITE, 0);
      }
      else
      {
         // can't decipher *what* exactly happened, but it was a short write and thus bad
         result = -1;
         redirty_page_for_writepage(state->wbc, page);
      }
   }

out:
   if(state->unlockPages)
   {
      for(i = 0; i < state->nr_pages; i++)
         unlock_page(state->pages[i]);
   }

   state->nr_pages = 0;
   return result;
}

static bool beegfs_wps_must_flush_before(struct beegfs_writepages_state* state, struct page* next)
{
   struct append_range_descriptor* ard;

   if(state->nr_pages == 0)
      return false;

   if(state->nr_pages == BEEGFS_WRITEPAGES_BLOCK_SIZE)
      return true;

   if(state->pages[state->nr_pages - 1]->index + 1 != next->index)
      return true;

   if(pvr_present(next) )
   {
      if(pvr_get_first(next) != 0)
         return true;

      if(pvr_get_last(state->pages[state->nr_pages - 1]) != PAGE_CACHE_SIZE - 1)
         return true;

      if(!pvr_present(state->pages[state->nr_pages - 1]) )
         return true;
   }

   ard = ard_from_page(next);
   if(ard)
   {
      if(ard_from_page(state->pages[state->nr_pages - 1]) != ard)
         return true;
   }

   return false;
}

static int beegfs_writepages_callback(struct page* page, struct writeback_control* wbc, void* data)
{
   App* app = FhgfsOps_getApp(page->mapping->host->i_sb);
   struct beegfs_writepages_state* state = data;
   int flushRes;
   bool mustFlush;

   FhgfsOpsHelper_logOpDebug(app, NULL, page->mapping->host, __func__, "page %lu",
      page ? page->index : 0);
   IGNORE_UNUSED_VARIABLE(app);

   BUG_ON(!pvr_present(page) && !ard_from_page(page) );

   mustFlush = beegfs_wps_must_flush_before(state, page);

   if(mustFlush)
   {
      flushRes = beegfs_writepages_submit(state);
      if(flushRes < 0)
      {
         page_endio(page, WRITE, flushRes);
         return flushRes;
      }
   }

   state->pages[state->nr_pages] = page;
   state->nr_pages += 1;
   set_page_writeback(page);

   return 0;
}

static int beegfs_do_write_pages(struct address_space* mapping, struct writeback_control* wbc,
   struct page* page, bool unlockPages)
{
   struct inode* inode = mapping->host;
   App* app = FhgfsOps_getApp(inode->i_sb);

   struct beegfs_writepages_state* state = mempool_alloc(writepages_pool, GFP_NOFS);
   FhgfsOpsErr referenceRes;
   FileHandleType handleType;
   RemotingIOInfo ioInfo;
   int err;

   FhgfsOpsHelper_logOpDebug(app, NULL, inode, __func__, "page? %i %lu", page != NULL,
      page ? page->index : 0);
   IGNORE_UNUSED_VARIABLE(app);

   referenceRes = FhgfsInode_referenceHandle(BEEGFS_INODE(inode), OPENFILE_ACCESS_READWRITE, true,
      NULL, &handleType);
   if(referenceRes != FhgfsOpsErr_SUCCESS)
      return FhgfsOpsErr_toSysErr(referenceRes);

   FhgfsInode_getRefIOInfo(BEEGFS_INODE(inode), handleType, OPENFILE_ACCESS_READWRITE, &ioInfo);

   state->nr_pages = 0;
   state->ioInfo = &ioInfo;
   state->wbc = wbc;
   state->unlockPages = unlockPages;

   FhgfsInode_incWriteBackCounter(BEEGFS_INODE(inode) );

   if(page)
      err = beegfs_writepages_callback(page, wbc, state);
   else
      err = os_write_cache_pages(mapping, wbc, beegfs_writepages_callback, state);

   if(!err)
      err = beegfs_writepages_submit(state);

   FhgfsInode_releaseHandle(BEEGFS_INODE(inode), handleType);

   mempool_free(state, writepages_pool);

   FhgfsInode_decWriteBackCounter(BEEGFS_INODE(inode) );
   FhgfsInode_unsetNoIsizeDecrease(BEEGFS_INODE(inode) );

   return err;
}

static int beegfs_writepage(struct page* page, struct writeback_control* wbc)
{
   struct inode* inode = page->mapping->host;
   App* app = FhgfsOps_getApp(inode->i_sb);

   FhgfsOpsHelper_logOpDebug(app, NULL, inode, __func__, "");
   IGNORE_UNUSED_VARIABLE(app);

   return beegfs_do_write_pages(page->mapping, wbc, page, true);
}

static int beegfs_writepages(struct address_space* mapping, struct writeback_control* wbc)
{
   struct inode* inode = mapping->host;
   App* app = FhgfsOps_getApp(inode->i_sb);

   FhgfsOpsHelper_logOpDebug(app, NULL, inode, __func__, "");
   IGNORE_UNUSED_VARIABLE(app);

   return beegfs_do_write_pages(mapping, wbc, NULL, true);
}



static int beegfs_flush_page(struct page* page)
{
   struct writeback_control wbc = {
      .nr_to_write = 1,
      .sync_mode = WB_SYNC_ALL,
   };

   if(!clear_page_dirty_for_io(page) )
      return 0;

   return beegfs_do_write_pages(page->mapping, &wbc, page, false);
}

static int beegfs_readpage(struct file* filp, struct page* page)
{
   App* app = FhgfsOps_getApp(filp->f_dentry->d_sb);

   struct inode* inode = filp->f_mapping->host;
   FhgfsInode* fhgfsInode = BEEGFS_INODE(inode);
   FsFileInfo* fileInfo = __FhgfsOps_getFileInfo(filp);
   RemotingIOInfo ioInfo;
   ssize_t readRes = -EIO;

   loff_t offset = page_offset(page);

   FhgfsOpsHelper_logOpDebug(app, filp->f_dentry, inode, __func__, "offset: %lld", offset);
   IGNORE_UNUSED_VARIABLE(app);

   FsFileInfo_getIOInfo(fileInfo, fhgfsInode, &ioInfo);

   if(pvr_present(page) || ard_from_page(page) )
   {
      readRes = beegfs_flush_page(page);
      if(readRes)
         goto out;
   }

   if(BEEGFS_SHOULD_FAIL(readpage, 1) )
      goto out;

   // TODO: handle sparse files correctly here, but first we need a sensible consistency model
   // for inode sizes
   readRes = FhgfsOpsRemoting_readfile(page_address(page), PAGE_CACHE_SIZE, offset, &ioInfo,
      fhgfsInode);

   if(readRes < 0)
   {
      readRes = FhgfsOpsErr_toSysErr(-readRes);
      goto out;
   }

   if(readRes < PAGE_CACHE_SIZE)
      memset(page_address(page) + readRes, 0, PAGE_CACHE_SIZE - readRes);

   readRes = 0;
   task_io_account_read(PAGE_CACHE_SIZE);

out:
   page_endio(page, READ, readRes);
   return readRes;
}


enum
{
   BEEGFS_READPAGES_BLOCK_SIZE = 128,
};

struct beegfs_readpages_state
{
   struct page* pages[BEEGFS_READPAGES_BLOCK_SIZE];
   struct iovec iov[BEEGFS_READPAGES_BLOCK_SIZE];
   unsigned nr_pages;

   RemotingIOInfo* ioInfo;
};

static int beegfs_readpages_submit(struct beegfs_readpages_state* state)
{
   App* app;
   struct iov_iter iter;
   ssize_t readRes;
   unsigned validPages = 0;
   int err = 0;
   int i;

   if(!state->nr_pages)
      return 0;

   app = FhgfsOps_getApp(state->pages[0]->mapping->host->i_sb);

   FhgfsOpsHelper_logOpDebug(app, NULL, state->pages[0]->mapping->host, __func__,
      "first offset: %lld nr_pages %u", page_offset(state->pages[0]), state->nr_pages);
   IGNORE_UNUSED_VARIABLE(app);

   if(BEEGFS_SHOULD_FAIL(readpage, 1) )
   {
      err = -EIO;
      goto endio;
   }

   BEEGFS_IOV_ITER_INIT(&iter, READ, state->iov, state->nr_pages,
      state->nr_pages * PAGE_CACHE_SIZE);

   // TODO: handle sparse files better (see readpage)
   // currently, this simply fails readahead for sparse files and has readpage fix it
   readRes = FhgfsOpsRemoting_readfileVec(&iter, page_offset(state->pages[0]), state->ioInfo,
      BEEGFS_INODE(state->pages[0]->mapping->host) );
   if(readRes < 0)
      err = FhgfsOpsErr_toSysErr(-readRes);

   if(err < 0)
      goto endio;

   validPages = readRes / PAGE_CACHE_SIZE;

   if(readRes % PAGE_CACHE_SIZE != 0)
   {
      int start = readRes % PAGE_CACHE_SIZE;
      memset(page_address(state->pages[validPages]) + start, 0, PAGE_CACHE_SIZE - start);
      validPages += 1;
   }

endio:
   for(i = 0; i < validPages; i++)
      page_endio(state->pages[i], READ, err);

   for(i = validPages; i < state->nr_pages; i++)
   {
      ClearPageUptodate(state->pages[i]);
      unlock_page(state->pages[i]);
   }

   state->nr_pages = 0;
   return err;
}

static int beegfs_readpages_add_page(void* data, struct page* page)
{
   struct beegfs_readpages_state* state = data;
   bool mustFlush;

   mustFlush = (state->nr_pages == BEEGFS_READPAGES_BLOCK_SIZE)
      || (state->nr_pages > 0 && state->pages[state->nr_pages - 1]->index + 1 != page->index);

   if(mustFlush)
   {
      int flushRes;

      flushRes = beegfs_readpages_submit(state);
      if(flushRes < 0)
      {
         page_endio(page, READ, flushRes);
         return flushRes;
      }
   }

   state->pages[state->nr_pages] = page;
   state->iov[state->nr_pages].iov_base = page_address(page);
   state->iov[state->nr_pages].iov_len = PAGE_CACHE_SIZE;
   state->nr_pages += 1;

   return 0;
}

static int beegfs_readpages(struct file* filp, struct address_space* mapping,
   struct list_head* pages, unsigned nr_pages)
{
   App* app = FhgfsOps_getApp(filp->f_dentry->d_sb);

   struct inode* inode = mapping->host;
   FhgfsInode* fhgfsInode = BEEGFS_INODE(inode);
   FsFileInfo* fileInfo = __FhgfsOps_getFileInfo(filp);
   RemotingIOInfo ioInfo;
   int err = 0;

   struct beegfs_readpages_state* state;

   state = kmalloc(sizeof(*state), GFP_NOFS);
   if(!state)
      return -ENOMEM;

   FhgfsOpsHelper_logOpDebug(app, filp->f_dentry, inode, __func__, "first offset: %lld nr_pages %u",
      page_offset(list_entry(pages->prev, struct page, lru) ), nr_pages);
   IGNORE_UNUSED_VARIABLE(app);

   FsFileInfo_getIOInfo(fileInfo, fhgfsInode, &ioInfo);

   state->nr_pages = 0;
   state->ioInfo = &ioInfo;

   err = read_cache_pages(mapping, pages, beegfs_readpages_add_page, state);
   if(!err)
      err = beegfs_readpages_submit(state);

   kfree(state);
   return err;
}



static int __beegfs_write_begin(struct file* filp, loff_t pos, unsigned len, struct page* page)
{
   struct append_range_descriptor* ard;
   int result = 0;

   App* app = FhgfsOps_getApp(filp->f_dentry->d_sb);

   FhgfsOpsHelper_logOpDebug(app, filp->f_dentry, page->mapping->host, __func__,
      "write_begin %lli by %u", pos, len);
   IGNORE_UNUSED_VARIABLE(app);
   IGNORE_UNUSED_VARIABLE(len);

   if(filp->f_flags & O_APPEND)
   {
      if(pvr_present(page) )
         goto flush_page;

      ard = ard_from_page(page);
      if(!ard)
         goto page_clean;

      /* mapping_ard will not change here because the write path holds a reference to it. */
      if(ard == __mapping_ard(page->mapping) )
         goto success;
   }
   else
   {
      if(ard_from_page(page) )
         goto flush_page;

      if(!pvr_present(page) )
         goto success;

      if(pvr_can_merge(page, pos & ~PAGE_CACHE_MASK, (pos & ~PAGE_CACHE_MASK) + len - 1))
         goto success;
   }

flush_page:
   result = beegfs_flush_page(page);
   if(result)
      goto out_err;

page_clean:
   if(filp->f_flags & O_APPEND)
      ard_assign(page, __mapping_ard(page->mapping) );

success:
   return result;

out_err:
   unlock_page(page);
   page_cache_release(page);
   return result;
}

static int __beegfs_write_end(struct file* filp, loff_t pos, unsigned len, unsigned copied,
   struct page* page)
{
   struct inode* inode = page->mapping->host;
   int result = copied;

   App* app = FhgfsOps_getApp(filp->f_dentry->d_sb);

   FhgfsOpsHelper_logOpDebug(app, filp->f_dentry, inode, __func__, "write_end %lli by %u", pos,
      len);
   IGNORE_UNUSED_VARIABLE(filp);

   if(copied != len && pvr_present(page) )
   {
      FhgfsOpsHelper_logOpMsg(2, app, filp->f_dentry, inode, __func__, "short write!");
      result = 0;
      goto out;
   }

   if(filp->f_flags & O_APPEND)
   {
      struct append_range_descriptor* ard = ard_from_page(page);

      BUG_ON(!ard);

      if( (filp->f_flags & O_APPEND) && ard->firstPageStart < 0)
      {
         mutex_lock(&ard->mtx);
         ard->firstPageIndex = page->index;
         ard->firstPageStart = pos & ~PAGE_CACHE_MASK;
         mutex_unlock(&ard->mtx);
      }

      goto out;
   }

   /* write_iter updates isize for appends, since a later page may fail on an allocated ard */
   if(i_size_read(inode) < pos + copied)
   {
      i_size_write(inode, pos + copied);
      FhgfsInode_setPageWriteFlag(BEEGFS_INODE(inode) );
      FhgfsInode_setLastWriteBackOrIsizeWriteTime(BEEGFS_INODE(inode) );
      FhgfsInode_setNoIsizeDecrease(BEEGFS_INODE(inode) );
   }

   if(pvr_present(page) )
      pvr_merge(page, pos & ~PAGE_CACHE_MASK, (pos & ~PAGE_CACHE_MASK) + copied - 1);
   else
   {
      pvr_init(page);
      pvr_set_first(page, pos & ~PAGE_CACHE_MASK);
      pvr_set_last(page, (pos & ~PAGE_CACHE_MASK) + copied - 1);
   }

out:
   ClearPageUptodate(page);
   __set_page_dirty_nobuffers(page);

   unlock_page(page);
   page_cache_release(page);

   return result;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
static int beegfs_prepare_write(struct file* filp, struct page* page, unsigned from, unsigned to)
{
   return __beegfs_write_begin(filp, from, to - from + 1, page);
}

static int beegfs_commit_write(struct file* filp, struct page* page, unsigned from, unsigned to)
{
   return __beegfs_write_end(filp, from, to - from + 1, to - from + 1, page);
}
#else
static int beegfs_write_begin(struct file* filp, struct address_space* mapping, loff_t pos,
   unsigned len, unsigned flags, struct page** pagep, void** fsdata)
{
   pgoff_t index = pos >> PAGE_CACHE_SHIFT;

   *pagep = grab_cache_page_write_begin(mapping, index, flags);
   if(!*pagep)
      return -ENOMEM;

   return __beegfs_write_begin(filp, pos, len, *pagep);
}

static int beegfs_write_end(struct file* filp, struct address_space* mapping, loff_t pos,
   unsigned len, unsigned copied, struct page* page, void* fsdata)
{
   return __beegfs_write_end(filp, pos, len, copied, page);
}
#endif

static int beegfs_releasepage(struct page* page, gfp_t gfp)
{
   IGNORE_UNUSED_VARIABLE(gfp);

   if(pvr_present(page) )
   {
      pvr_clear(page);
      return 1;
   }

   BUG_ON(!ard_from_page(page) );
   ard_assign(page, NULL);
   return 1;
}

static int beegfs_set_page_dirty(struct page* page)
{
   if(ard_from_page(page) )
   {
      int err;

      err = beegfs_flush_page(page);
      if(err < 0)
         return err;
   }

   pvr_init(page);
   pvr_set_first(page, 0);
   pvr_set_last(page, PAGE_CACHE_SIZE - 1);
   return __set_page_dirty_nobuffers(page);
}

static void __beegfs_invalidate_page(struct page* page, unsigned begin, unsigned end)
{
   if(pvr_present(page) )
   {
      unsigned pvr_begin = pvr_get_first(page);
      unsigned pvr_end = pvr_get_last(page);

      if(begin == 0 && end == PAGE_CACHE_SIZE)
      {
         pvr_clear(page);
         ClearPageUptodate(page);
         return;
      }

      if(begin < pvr_begin)
         pvr_set_first(page, begin);

      if(pvr_end < end)
         pvr_set_last(page, end);

      return;
   }

   BUG_ON(!ard_from_page(page) );

   ard_assign(page, NULL);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,11,0)
static void beegfs_invalidate_page(struct page* page, unsigned long begin)
{
   __beegfs_invalidate_page(page, begin, PAGE_CACHE_SIZE);
}
#else
static void beegfs_invalidate_page(struct page* page, unsigned begin, unsigned end)
{
   __beegfs_invalidate_page(page, begin, end);
}
#endif

static ssize_t beegfs_dIO_read(struct kiocb* iocb, struct iov_iter* iter, loff_t offset,
   RemotingIOInfo* ioInfo)
{
   struct file* filp = iocb->ki_filp;
   struct inode* inode = file_inode(filp);
   struct dentry* dentry = filp->f_dentry;

   App* app = FhgfsOps_getApp(dentry->d_sb);

   struct iov_iter loopIter;
   struct iovec iov;

   ssize_t readRes = 0;
   ssize_t result = 0;

   FhgfsOpsHelper_logOpDebug(app, dentry, inode, __func__, "pos: %lld, nr_segs: %lld",
      offset, iter->nr_segs);
   IGNORE_UNUSED_VARIABLE(app);

   // TODO: think of a sensible isize consistency model (see beegfs_do_readpage)
   iov_for_each(iov, loopIter, *iter)
   {
      readRes = FhgfsOpsRemoting_readfile(iov.iov_base, iov.iov_len, offset, ioInfo,
         BEEGFS_INODE(inode) );
      if(readRes < 0)
         break;

      if(readRes < iov.iov_len)
      {
         result += readRes;
         offset += readRes;
         readRes = __FhgfsOps_readSparse(filp, iov.iov_base + readRes, iov.iov_len - readRes,
            offset + readRes);

         if(readRes < 0)
            break;
      }

      result += readRes;
      offset += readRes;

      if(readRes < iov.iov_len)
         break;
   }

   task_io_account_read(result);

   if(offset > i_size_read(inode) )
   {
      spin_lock(&inode->i_lock);
      {
         if(offset > i_size_read(inode) )
            i_size_write(inode, offset);
      }
      spin_unlock(&inode->i_lock);
   }

   if(result == 0)
      return FhgfsOpsErr_toSysErr(-readRes);

   return result;
}

static ssize_t beegfs_dIO_write(struct kiocb* iocb, struct iov_iter* iter, loff_t offset,
   RemotingIOInfo* ioInfo)
{
   struct file* filp = iocb->ki_filp;
   struct inode* inode = file_inode(filp);
   struct dentry* dentry = filp->f_dentry;

   App* app = FhgfsOps_getApp(dentry->d_sb);

   struct iov_iter loopIter;
   struct iovec iov;

   ssize_t writeRes = 0;
   ssize_t result = 0;

   FhgfsOpsHelper_logOpDebug(app, dentry, inode, __func__, "pos: %lld, nr_segs: %lld",
      offset, iter->nr_segs);
   IGNORE_UNUSED_VARIABLE(app);
   IGNORE_UNUSED_VARIABLE(inode);

   iov_for_each(iov, loopIter, *iter)
   {
      writeRes = FhgfsOpsRemoting_writefile(iov.iov_base, iov.iov_len, offset, ioInfo);
      if(writeRes < 0)
         break;

      result += writeRes;
      offset += writeRes;
      if( (size_t) writeRes < iov.iov_len)
         break;
   }

   task_io_account_write(result);

   if(result == 0)
      return FhgfsOpsErr_toSysErr(-writeRes);

   return result;
}

static ssize_t __beegfs_direct_IO(int rw, struct kiocb* iocb, struct iov_iter* iter, loff_t offset)
{
   struct file* filp = iocb->ki_filp;
   struct inode* inode = file_inode(filp);
   RemotingIOInfo ioInfo;

   FsFileInfo_getIOInfo(__FhgfsOps_getFileInfo(filp), BEEGFS_INODE(inode), &ioInfo);

   // TODO: handle swap file BVEC
   if(!iter_is_iovec(iter) )
   {
      printk(KERN_ERR "unexpected direct_IO rw %d\n", rw);
      WARN_ON(1);
      return -EINVAL;
   }

   switch(rw)
   {
      case READ:
         return beegfs_dIO_read(iocb, iter, offset, &ioInfo);

      case WRITE:
         return beegfs_dIO_write(iocb, iter, offset, &ioInfo);
   }

   BUG();
   return -EINVAL;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,1,0)
static ssize_t beegfs_direct_IO(struct kiocb* iocb, struct iov_iter* iter, loff_t offset)
{
   return __beegfs_direct_IO(iov_iter_rw(iter), iocb, iter, offset);
}
#elif defined(KERNEL_HAS_DIRECT_IO_ITER)
static ssize_t beegfs_direct_IO(int rw, struct kiocb* iocb, struct iov_iter* iter, loff_t offset)
{
   return __beegfs_direct_IO(rw, iocb, iter, offset);
}
#else
static ssize_t beegfs_direct_IO(int rw, struct kiocb* iocb, const struct iovec* iov, loff_t pos,
      unsigned long nr_segs)
{
   struct iov_iter iter;

   BEEGFS_IOV_ITER_INIT(&iter, rw & RW_MASK, iov, nr_segs, iov_length(iov, nr_segs) );

   return __beegfs_direct_IO(rw, iocb, &iter, pos);
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
static int beegfs_launderpage(struct page* page)
{
   return beegfs_flush_page(page);
}
#endif

const struct address_space_operations fhgfs_addrspace_native_ops = {
   .readpage = beegfs_readpage,
   .writepage = beegfs_writepage,
   .releasepage = beegfs_releasepage,
   .set_page_dirty = beegfs_set_page_dirty,
   .invalidatepage = beegfs_invalidate_page,
   .direct_IO = beegfs_direct_IO,

   .readpages = beegfs_readpages,
   .writepages = beegfs_writepages,

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
   .launder_page = beegfs_launderpage,
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
   .prepare_write = beegfs_prepare_write,
   .commit_write = beegfs_commit_write,
#else
   .write_begin = beegfs_write_begin,
   .write_end = beegfs_write_end,
#endif
};

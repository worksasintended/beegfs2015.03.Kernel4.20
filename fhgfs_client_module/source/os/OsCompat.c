/*
 * Compatibility functions for older Linux versions
 */

//#include <linux/mm.h> // for old sles10 kernels, which forgot to include it in backing-dev.h
#include <linux/backing-dev.h>
#include <linux/pagemap.h>
#include <linux/uio.h>
#include <linux/writeback.h>

#include <app/App.h>
#include <app/log/Logger.h>
#include <common/Common.h>
#include <filesystem/FhgfsOpsSuper.h>

#ifndef KERNEL_HAS_MEMDUP_USER
   /**
    * memdup_user - duplicate memory region from user space
    *
    * @src: source address in user space
    * @len: number of bytes to copy
    *
    * Returns an ERR_PTR() on failure.
    */
   void *memdup_user(const void __user *src, size_t len)
   {
      void *p;

      /*
       * Always use GFP_KERNEL, since copy_from_user() can sleep and
       * cause pagefault, which makes it pointless to use GFP_NOFS
       * or GFP_ATOMIC.
       */
      p = kmalloc(len, GFP_KERNEL);
      if (!p)
         return ERR_PTR(-ENOMEM);

      if (copy_from_user(p, src, len)) {
         kfree(p);
         return ERR_PTR(-EFAULT);
      }

      return p;
   }
#endif // memdup_user, LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)



#if defined(KERNEL_HAS_SB_BDI) && !defined(KERNEL_HAS_BDI_SETUP_AND_REGISTER) && \
    !defined(KERNEL_HAS_SUPER_SETUP_BDI_NAME)
   /*
    * For use from filesystems to quickly init and register a bdi associated
    * with dirty writeback
    */
//   int bdi_setup_and_register(struct backing_dev_info *bdi, char *name,
//               unsigned int cap)
//   {
//      static atomic_long_t fhgfs_bdiSeq = ATOMIC_LONG_INIT(0);
//      char tmp[32];
//      int err;
//
//      bdi->name = name;
//      bdi->capabilities = cap;
//      err = bdi_init(bdi);
//      if (err)
//         return err;
//
//      sprintf(tmp, "%.28s%s", name, "-%d");
//      err = bdi_register(bdi, NULL, tmp, atomic_long_inc_return(&fhgfs_bdiSeq));
//      if (err) {
//         bdi_destroy(bdi);
//         return err;
//      }
//
//      return 0;
//   }
#endif



//if defined(KERNEL_HAS_SB_BDI) && !defined(KERNEL_HAS_BDI_SETUP_AND_REGISTER)
//   /*
//    * For use from filesystems to quickly init and register a bdi associated
//    * with dirty writeback
//    */
//   int bdi_setup_and_register(struct backing_dev_info *bdi, char *name,
//               unsigned int cap)
//   {
//      static atomic_long_t fhgfs_bdiSeq = ATOMIC_LONG_INIT(0);
//      char tmp[32];
//      int err;
//
//      bdi->name = name;
//      bdi->capabilities = cap;
//      err = bdi_init(bdi);
//      if (err)
//         return err;
//
//      sprintf(tmp, "%.28s%s", name, "-%d");
//      err = bdi_register(bdi, NULL, tmp, atomic_long_inc_return(&fhgfs_bdiSeq));
//      if (err) {
//         bdi_destroy(bdi);
//         return err;
//      }
//
//      return 0;
//   }
//#endif
//


/* NOTE: We can't do a feature detection for find_get_pages_tag(), as 
 *       this function is in all headers of all supported kernel versions.
 *       However, it is only _exported_ since 2.6.22 and also only 
 *       exported in RHEL >=5.10. */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)
   /**
    * find_get_pages_tag - find and return pages that match @tag
    * @mapping:   the address_space to search
    * @index:  the starting page index
    * @tag: the tag index
    * @nr_pages:  the maximum number of pages
    * @pages:  where the resulting pages are placed
    *
    * Like find_get_pages, except we only return pages which are tagged with
    * @tag.   We update @index to index the next page for the traversal.
    */
   unsigned find_get_pages_tag(struct address_space *mapping, pgoff_t *index,
            int tag, unsigned int nr_pages, struct page **pages)
   {
      unsigned int i;
      unsigned int ret;

      read_lock_irq(&mapping->tree_lock);
      ret = radix_tree_gang_lookup_tag(&mapping->page_tree,
               (void **)pages, *index, nr_pages, tag);
      for (i = 0; i < ret; i++)
         page_cache_get(pages[i]);
      if (ret)
         *index = pages[ret - 1]->index + 1;
      read_unlock_irq(&mapping->tree_lock);
      return ret;
   }
#endif // find_get_pages_tag() for <2.6.22


#ifndef KERNEL_HAS_D_MAKE_ROOT

/**
 * This is the former d_alloc_root with an additional iput on error.
 */
struct dentry *d_make_root(struct inode *root_inode)
{
   struct dentry* allocRes = d_alloc_root(root_inode);
   if(!allocRes)
      iput(root_inode);

   return allocRes;
}
#endif

#ifndef KERNEL_HAS_D_MATERIALISE_UNIQUE
/**
 * d_materialise_unique() was merged into d_splice_alias() in linux-3.19
 */
struct dentry* d_materialise_unique(struct dentry *dentry, struct inode *inode)
{
   return d_splice_alias(inode, dentry);
}
#endif // KERNEL_HAS_D_MATERIALISE_UNIQUE

/**
 * Note: Call this once during module init (and remember to call kmem_cache_destroy() )
 */
#if defined(KERNEL_HAS_KMEMCACHE_CACHE_FLAGS_CTOR)
struct kmem_cache* OsCompat_initKmemCache(const char* cacheName, size_t cacheSize,
   void initFuncPtr(void* initObj, struct kmem_cache* cache, unsigned long flags) )
#elif defined(KERNEL_HAS_KMEMCACHE_CACHE_CTOR)
struct kmem_cache* OsCompat_initKmemCache(const char* cacheName, size_t cacheSize,
   void initFuncPtr(struct kmem_cache* cache, void* initObj) )
#else
struct kmem_cache* OsCompat_initKmemCache(const char* cacheName, size_t cacheSize,
   void initFuncPtr(void* initObj) )
#endif // LINUX_VERSION_CODE
{
   struct kmem_cache* cache;

   unsigned long cacheFlags = SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD;

#if defined(KERNEL_HAS_KMEMCACHE_DTOR)
   cache = kmem_cache_create(cacheName, cacheSize, 0, cacheFlags, initFuncPtr, NULL);
#else
   cache = kmem_cache_create(cacheName, cacheSize, 0, cacheFlags, initFuncPtr);
#endif // LINUX_VERSION_CODE


   return cache;
}


/*
 * compatibility for older kernels. this code is mostly taken from include/linux/uio.h,
 * include/linuxfs/fs.h and associated .c files.
 *
 * the originals are licensed as:
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */

#ifndef os_iov_iter_h_gkoNxI6c8tqzi7GeSKSVlb
#define os_iov_iter_h_gkoNxI6c8tqzi7GeSKSVlb

#include <linux/kernel.h>
#include <linux/uio.h>
#include <linux/version.h>

#ifndef KERNEL_HAS_IOV_ITER_TYPE
struct iov_iter {
   const struct iovec *iov;
   unsigned long nr_segs;
   size_t iov_offset;
   size_t count;
};
#endif

#ifndef KERNEL_HAS_IOV_ITER_FUNCTIONS
static inline void iov_iter_advance(struct iov_iter *i, size_t bytes)
{
   BUG_ON(i->count < bytes);

   if (likely(i->nr_segs == 1) )
   {
      i->iov_offset += bytes;
      i->count -= bytes;
   }
   else
   {
      const struct iovec *iov = i->iov;
      size_t base = i->iov_offset;
      unsigned long nr_segs = i->nr_segs;

      /*
       * The !iov->iov_len check ensures we skip over unlikely
       * zero-length segments (without overruning the iovec).
       */
      while (bytes || unlikely(i->count && !iov->iov_len) )
      {
         int copy;

         copy = min(bytes, iov->iov_len - base);
         BUG_ON(!i->count || i->count < copy);
         i->count -= copy;
         bytes -= copy;
         base += copy;

         if (iov->iov_len == base)
         {
            iov++;
            nr_segs--;
            base = 0;
         }
      }

      i->iov = iov;
      i->iov_offset = base;
      i->nr_segs = nr_segs;
   }
}

static inline void iov_iter_init(struct iov_iter *i, const struct iovec *iov, unsigned long nr_segs,
   size_t count, size_t written)
{
   i->iov = iov;
   i->nr_segs = nr_segs;
   i->iov_offset = 0;
   i->count = count + written;

   iov_iter_advance(i, written);
}
#endif

#if defined(KERNEL_HAS_IOV_ITER_IN_FS)
#include <linux/fs.h>
#else
#include <linux/uio.h>
#endif

#ifndef iov_for_each
#define iov_for_each(iov, iter, start)                          \
        for (iter = (start);                                    \
             (iter).count &&                                    \
             ((iov = iov_iter_iovec(&(iter))), 1);              \
             iov_iter_advance(&(iter), (iov).iov_len))
#endif

#if !defined(KERNEL_HAS_IOV_ITER_IOVEC)
static inline struct iovec iov_iter_iovec(const struct iov_iter *iter)
{
   return (struct iovec) {
      .iov_base = iter->iov->iov_base + iter->iov_offset,
      .iov_len = min(iter->count, iter->iov->iov_len - iter->iov_offset),
   };
}
#endif

#ifndef KERNEL_HAS_IOV_ITER_TRUNCATE 
static inline void iov_iter_truncate(struct iov_iter *i, size_t count)
{
   if (i->count > count)
      i->count = count;
}
#endif

static inline void BEEGFS_IOV_ITER_INIT(struct iov_iter* iter, int direction,
   const struct iovec* iov, unsigned long nr_segs, size_t count)
{
#ifdef KERNEL_HAS_IOV_ITER_INIT_DIR
   iov_iter_init(iter, direction, iov, nr_segs, count);
#else
   iov_iter_init(iter, iov, nr_segs, count, 0);
#endif
}

#if !defined(KERNEL_HAS_ITER_BVEC)
static inline bool iter_is_iovec(struct iov_iter* i)
{
   return true;
}
#elif !defined(KERNEL_HAS_ITER_IS_IOVEC)
static inline bool iter_is_iovec(struct iov_iter* i)
{
   return !(i->type & (ITER_BVEC | ITER_KVEC) );
}
#endif

#endif

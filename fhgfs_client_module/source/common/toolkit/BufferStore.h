#ifndef BUFFERSTORE_H_
#define BUFFERSTORE_H_

// O B S O L E T E (we have NoAllocBufferStore now)
// O B S O L E T E (we have NoAllocBufferStore now)
// O B S O L E T E (we have NoAllocBufferStore now)

//#include <common/threading/Open_Mutex.h>
//#include <common/threading/Open_Condition.h>
//#include <common/toolkit/list/PointerList.h>
//#include <common/toolkit/list/PointerListIter.h>
//#include <common/Common.h>
//
//
//#define BUFSTORE_DEFAULT_THREADEDIO_BUFSIZE    1048576
//
//
//struct BufferStore;
//typedef struct BufferStore BufferStore;
//
//static inline void BufferStore_init(BufferStore* this, size_t numBufs, size_t bufSize);
//static inline BufferStore* BufferStore_construct(size_t numBufs, size_t bufSize);
//static inline void BufferStore_uninit(BufferStore* this);
//static inline void BufferStore_destruct(BufferStore* this);
//
//static inline char* BufferStore_waitForBuf(BufferStore* this);
//static inline char* BufferStore_instantBuf(BufferStore* this);
//static inline void BufferStore_addBuf(BufferStore* this, char* buf);
//
//static inline void __BufferStore_initBuffers(BufferStore* this);
//
//// getters & setters
//static inline size_t BufferStore_getBufSize(BufferStore* this);
//static inline fhgfs_bool BufferStore_getValid(BufferStore* this);
//
//
//struct BufferStore
//{
//   PointerList bufList;
//
//   size_t numBufs;
//   size_t bufSize;
//
//   fhgfs_bool storeValid;
//
//   Mutex* mutex;
//   Condition* newBufCond;
//};
//
///**
// * @param numBufs number of buffers to be allocated
// * @param bufSize size of each allocated buffer
// */
//void BufferStore_init(BufferStore* this, size_t numBufs, size_t bufSize)
//{
//   this->mutex = Mutex_construct();
//   this->newBufCond = Condition_construct();
//
//   PointerList_init(&this->bufList);
//
//   this->numBufs = numBufs;
//   this->bufSize = bufSize;
//
//   this->storeValid = fhgfs_true;
//
//   __BufferStore_initBuffers(this);
//}
//
///**
// * @param numBufs number of buffers to be allocated
// * @param bufSize size of each allocated buffer
// */
//struct BufferStore* BufferStore_construct(size_t numBufs, size_t bufSize)
//{
//   struct BufferStore* this = (BufferStore*)os_kmalloc(sizeof(*this) );
//
//   BufferStore_init(this, numBufs, bufSize);
//
//   return this;
//}
//
//void BufferStore_uninit(BufferStore* this)
//{
//   // delete buffers
//   PointerListIter iter;
//
//   PointerListIter_init(&iter, &this->bufList);
//   for( ; !PointerListIter_end(&iter); PointerListIter_next(&iter) )
//   {
//      void* buf = PointerListIter_value(&iter);
//      os_vfree(buf);
//   }
//
//   PointerListIter_uninit(&iter);
//
//   // normal clean-up
//   PointerList_uninit(&this->bufList);
//
//   Condition_destruct(this->newBufCond);
//   Mutex_destruct(this->mutex);
//}
//
//void BufferStore_destruct(BufferStore* this)
//{
//   BufferStore_uninit(this);
//
//   os_kfree(this);
//}
//
///**
// * Gets a buffer from the store.
// * Waits if no buffer is immediately available.
// *
// * @return a valid buffer pointer
// */
//char* BufferStore_waitForBuf(BufferStore* this)
//{
//   char* buf;
//
//   Mutex_lock(this->mutex);
//
//   while(!PointerList_length(&this->bufList) )
//      Condition_wait(this->newBufCond, this->mutex);
//
//   buf = PointerList_getHeadValue(&this->bufList);
//   PointerList_removeHead(&this->bufList);
//
//   Mutex_unlock(this->mutex);
//
//   return buf;
//}
//
///**
// * Gets a buffer from the store.
// * Fails if no buffer is immediately available.
// *
// * @return buffer pointer if a buffer was immediately available, NULL otherwise
// */
//char* BufferStore_instantBuf(BufferStore* this)
//{
//   char* buf;
//
//   Mutex_lock(this->mutex);
//
//   if(!PointerList_length(&this->bufList) )
//      buf = NULL;
//   else
//   {
//      buf = PointerList_getHeadValue(&this->bufList);
//      PointerList_removeHead(&this->bufList);
//   }
//
//   Mutex_unlock(this->mutex);
//
//   return buf;
//}
//
//void BufferStore_addBuf(BufferStore* this, char* buf)
//{
//   Mutex_lock(this->mutex);
//
//   Condition_signal(this->newBufCond);
//
//   PointerList_addHead(&this->bufList, buf);
//
//   Mutex_unlock(this->mutex);
//}
//
//void __BufferStore_initBuffers(BufferStore* this)
//{
//   size_t i;
//
//   for(i = 0; i < this->numBufs; i++)
//   {
//      char* buf = os_vmalloc(this->bufSize);
//      if(!buf)
//      {
//         printk_fhgfs(KERN_WARNING, "BufferStore_initBuffers: vmalloc failed to alloc "
//            "%lld bytes for buffer number %lld\n", (long long)this->bufSize, (long long)i);
//
//         this->storeValid = fhgfs_false;
//
//         break;
//      }
//
//      PointerList_append(&this->bufList, buf);
//   }
//}
//
//size_t BufferStore_getBufSize(BufferStore* this)
//{
//   return this->bufSize;
//}
//
//fhgfs_bool BufferStore_getValid(BufferStore* this)
//{
//   return this->storeValid;
//}

#endif /*BUFFERSTORE_H_*/

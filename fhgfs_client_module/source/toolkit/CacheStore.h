#ifndef CACHESTORE_H_
#define CACHESTORE_H_

#include <common/threading/Mutex.h>
#include <common/threading/Condition.h>
#include <common/threading/Thread.h>
#include <common/toolkit/list/PointerList.h>
#include <common/toolkit/list/PointerListIter.h>
#include <common/toolkit/tree/PointerRBTree.h>
#include <common/toolkit/tree/PointerRBTreeIter.h>
#include <common/Common.h>

/**
 * currently unused. we now use a NoAllocBufferStore to provide the caches for buffered mode.
 */


//enum FileBufferType
//{
//   FileBufferType_NONE    =  0,
//   FileBufferType_READ    =  1,
//   FileBufferType_WRITE   =  2
//};
//
//
//struct CacheEntry;
//typedef struct CacheEntry CacheEntry;
//
//struct StoredCacheEntry;
//typedef struct StoredCacheEntry StoredCacheEntry;
//
//struct CacheStore;
//typedef struct CacheStore CacheStore;
//
//
//static inline void CacheStore_init(CacheStore* this, size_t numBufs, size_t bufSize);
//static inline CacheStore* CacheStore_construct(size_t numBufs, size_t bufSize);
//static inline void CacheStore_uninit(CacheStore* this);
//static inline void CacheStore_destruct(CacheStore* this);
//
//// static
//extern int __CacheStore_keyComparator(const void* key1, const void* key2);
//
//// inliners
//static inline CacheEntry* CacheStore_createEntry(CacheStore* this, size_t entryID);
//static inline CacheEntry* CacheStore_acquireEntry(CacheStore* this, size_t entryID);
//static inline void CacheStore_releaseEntry(CacheStore* this, CacheEntry* entry);
//static inline void CacheStore_invalidateEntry(CacheStore* this, size_t entryID);
//
//static inline void __CacheStore_initBuffers(CacheStore* this);
//static inline void __CacheStore_destroyEntry(CacheStore* this, CacheEntry* entry);
//
//// getters & setters
//static inline size_t CacheStore_getNumAvailabe(CacheStore* this);
//static inline size_t CacheStore_getNumUsed(CacheStore* this);
//static inline size_t CacheStore_getBufSize(CacheStore* this);
//static inline fhgfs_bool CacheStore_getValid(CacheStore* this);
//
//
//struct CacheEntry
//{
//   char* buf; // the file contents cache buffer
//   //size_t bufLen; // length of buf (not needed, because we have bufUsageMaxLen)
//   size_t bufUsageLen; // how much of the buffer is already used up
//   size_t bufUsageMaxLen; // how much of the buffer may be used (is min(bufLen, chunk_end) )
//
//   loff_t fileOffset; // the file offset where this buffer starts
//
//   enum FileBufferType cacheType;
//};
//
//struct StoredCacheEntry // "derived" from CacheEntry
//{
//   CacheEntry super;
//
//   fhgfs_bool isExclusive; // fhgfs_true if someone has exclusive access to this entry
//      // Note: This value may only be changed with the store mutex lock held
//};
//
//
//struct CacheStore
//{
//   PointerList bufList; // available buffers
//   RBTree cacheMap; // (key: entryID => value: CacheEntry*)
//
//   size_t numBufs;
//   size_t bufSize;
//
//   fhgfs_bool storeValid;
//
//   Mutex* mutex;
//   Condition* cacheReleaseCond; // for release or invalidate
//};
//
///**
// * @param numBufs number of buffers to be allocated
// * @param bufSize size of each allocated buffer
// */
//void CacheStore_init(CacheStore* this, size_t numBufs, size_t bufSize)
//{
//   this->mutex = Mutex_construct();
//   this->cacheReleaseCond = Condition_construct();
//
//   PointerList_init(&this->bufList);
//   PointerRBTree_init(&this->cacheMap, __CacheStore_keyComparator);
//
//   this->numBufs = numBufs;
//   this->bufSize = bufSize;
//
//   this->storeValid = fhgfs_true;
//
//   __CacheStore_initBuffers(this);
//}
//
///**
// * @param numBufs number of buffers to be allocated
// * @param bufSize size of each allocated buffer
// */
//struct CacheStore* CacheStore_construct(size_t numBufs, size_t bufSize)
//{
//   struct CacheStore* this = (CacheStore*)os_kmalloc(sizeof(*this) );
//
//   CacheStore_init(this, numBufs, bufSize);
//
//   return this;
//}
//
//void CacheStore_uninit(CacheStore* this)
//{
//   RBTreeIter treeIter;
//   PointerListIter listIter;
//
//   // destroy all remaining entries in the map
//
//   for( ; ; )
//   {
//      treeIter = PointerRBTree_begin(&this->cacheMap);
//      if(PointerRBTreeIter_end(&treeIter) )
//         break;
//      else
//      {
//         void* entryID = PointerRBTreeIter_key(&treeIter);
//         CacheEntry* cacheEntry = (CacheEntry*)PointerRBTreeIter_value(&treeIter);
//
//         __CacheStore_destroyEntry(this, cacheEntry);
//         PointerRBTree_erase(&this->cacheMap, entryID);
//
//         PointerRBTreeIter_uninit(&treeIter);
//      }
//   }
//
//   PointerRBTreeIter_uninit(&treeIter); // (additional uninit for the very last iter)
//
//
//   // delete buffers from bufList
//
//   PointerListIter_init(&listIter, &this->bufList);
//   for( ; !PointerListIter_end(&listIter); PointerListIter_next(&listIter) )
//   {
//      void* buf = PointerListIter_value(&listIter);
//      os_vfree(buf);
//   }
//
//   PointerListIter_uninit(&listIter);
//
//
//   // normal clean-up
//
//   PointerRBTree_uninit(&this->cacheMap);
//   PointerList_uninit(&this->bufList);
//
//   Condition_destruct(this->cacheReleaseCond);
//   Mutex_destruct(this->mutex);
//}
//
//void CacheStore_destruct(CacheStore* this)
//{
//   CacheStore_uninit(this);
//
//   os_kfree(this);
//}
//
///**
// * Create a new entry in the store.
// * Fails if no buffer is immediately available.
// *
// * Note: This does not check whether a cache with this ID already exists, so be careful with that.
// *
// * @return new entry with attached buffer (if buffer was available), NULL otherwise
// */
//CacheEntry* CacheStore_createEntry(CacheStore* this, size_t entryID)
//{
//   StoredCacheEntry* entry = NULL;
//
//   Mutex_lock(this->mutex);
//
//   if(PointerList_length(&this->bufList) )
//   { // buffer available
//
//      // create new entry
//      entry = (StoredCacheEntry*)os_kzalloc(sizeof(*entry) );
//
//      // init new entry + attach buffer
//      entry->isExclusive = fhgfs_true;
//      entry->super.bufUsageMaxLen = this->bufSize;
//
//      entry->super.buf = PointerList_getHeadValue(&this->bufList);
//      PointerList_removeHead(&this->bufList);
//
//      // insert into cacheMap
//      PointerRBTree_insert(&this->cacheMap, (void*)entryID, entry);
//   }
//
//   Mutex_unlock(this->mutex);
//
//   return (CacheEntry*)entry;
//}
//
///**
// * Acquires a CacheEntry for exclusive(!) access.
// * If this entry is currently acquired by someone else, this method will block until it is
// * released or invalidated.
// *
// * Note: Do not forget to call releaseEntry() or invalidateEntry() later.
// *
// * @return NULL if this entry does not exist
// */
//CacheEntry* CacheStore_acquireEntry(CacheStore* this, size_t entryID)
//{
//   RBTreeIter iter;
//   StoredCacheEntry* entry = NULL;
//
//   Mutex_lock(this->mutex);
//
//   // loop (wait) if the buffer is currently acquired for exclusive access
//   for( ; ; )
//   {
//      iter = PointerRBTree_find(&this->cacheMap, (void*)entryID);
//      if(PointerRBTreeIter_end(&iter) )
//      { // entry does not exist
//         entry = NULL;
//         break;
//      }
//
//      entry = (StoredCacheEntry*)PointerRBTreeIter_value(&iter);
//      if(!entry->isExclusive)
//      {
//         entry->isExclusive = fhgfs_true;
//         break;
//      }
//
//      // entry is acquired => wait and try again
//      Condition_wait(this->cacheReleaseCond, this->mutex);
//   }
//
//   PointerRBTreeIter_uninit(&iter);
//
//   Mutex_unlock(this->mutex);
//
//
//   return (CacheEntry*)entry;
//}
//
//
//void CacheStore_releaseEntry(CacheStore* this, CacheEntry* entry)
//{
//   StoredCacheEntry* storeEntry = (StoredCacheEntry*)entry;
//
//   Mutex_lock(this->mutex);
//
//   storeEntry->isExclusive = fhgfs_false;
//
//   Condition_signal(this->cacheReleaseCond);
//
//   Mutex_unlock(this->mutex);
//}
//
///**
// * Note: It is safe to call this if the caller cannot guarantee that the entryID really exists
// * in the store.
// */
//void CacheStore_invalidateEntry(CacheStore* this, size_t entryID)
//{
//   // Note: This might be called even if there is no entry for this ID, so be prepared for that.
//
//   RBTreeIter iter;
//
//   Mutex_lock(this->mutex);
//
//   iter = PointerRBTree_find(&this->cacheMap, (void*)entryID);
//   if(!PointerRBTreeIter_end(&iter) )
//   { // entry exists => erase entry from map and return buffer to the bufList
//      StoredCacheEntry* entry = (StoredCacheEntry*)PointerRBTreeIter_value(&iter);
//
//      PointerRBTree_erase(&this->cacheMap, (void*)entryID);
//
//      __CacheStore_destroyEntry(this, (CacheEntry*)entry);
//
//      Condition_signal(this->cacheReleaseCond);
//   }
//
//   Mutex_unlock(this->mutex);
//}
//
//void __CacheStore_initBuffers(CacheStore* this)
//{
//   size_t i;
//
//   for(i = 0; i < this->numBufs; i++)
//   {
//      char* buf = os_vmalloc(this->bufSize);
//      if(!buf)
//      {
//         printk_fhgfs(KERN_WARNING, "CacheStore_initBuffers: vmalloc failed to alloc "
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
///**
// * Insert cacheBuf into bufList and free CacheEntry
// */
//void __CacheStore_destroyEntry(CacheStore* this, CacheEntry* entry)
//{
//   PointerList_addHead(&this->bufList, entry->buf);
//   os_kfree(entry);
//}
//
//size_t CacheStore_getNumAvailabe(CacheStore* this)
//{
//   size_t bufListLen;
//
//   Mutex_lock(this->mutex);
//
//   bufListLen = PointerList_length(&this->bufList);
//
//   Mutex_unlock(this->mutex);
//
//   return bufListLen;
//}
//
//size_t CacheStore_getNumUsed(CacheStore* this)
//{
//   size_t cacheMapLen;
//
//   Mutex_lock(this->mutex);
//
//   cacheMapLen = PointerRBTree_length(&this->cacheMap);
//
//   Mutex_unlock(this->mutex);
//
//   return cacheMapLen;
//}
//
//size_t CacheStore_getBufSize(CacheStore* this)
//{
//   return this->bufSize;
//}
//
//fhgfs_bool CacheStore_getValid(CacheStore* this)
//{
//   return this->storeValid;
//}

#endif /*CACHESTORE_H_*/

#ifndef SYNCEDCOUNTERSTORE_H_
#define SYNCEDCOUNTERSTORE_H_

#include <common/toolkit/SynchronizedCounter.h>
#include <common/threading/Mutex.h>
#include <common/threading/Condition.h>
#include <common/threading/Thread.h>
#include <common/Common.h>


struct SyncedCounterStore;
typedef struct SyncedCounterStore SyncedCounterStore;


extern void SyncedCounterStore_init(SyncedCounterStore* this, size_t numCounters);
extern SyncedCounterStore* SyncedCounterStore_construct(size_t numCounters);
extern void SyncedCounterStore_uninit(SyncedCounterStore* this);
extern void SyncedCounterStore_destruct(SyncedCounterStore* this);

extern SynchronizedCounter* SyncedCounterStore_waitForCounter(SyncedCounterStore* this);
extern SynchronizedCounter* SyncedCounterStore_waitInterruptibleForCounter(
   SyncedCounterStore* this);
extern SynchronizedCounter* SyncedCounterStore_instantCounter(SyncedCounterStore* this);
extern void SyncedCounterStore_addCounter(SyncedCounterStore* this, SynchronizedCounter* counter);


extern void __SyncedCounterStore_initCounters(SyncedCounterStore* this);


// getters & setters
extern size_t SyncedCounterStore_getNumAvailable(SyncedCounterStore* this);
extern fhgfs_bool SyncedCounterStore_getStoreValid(SyncedCounterStore* this);


/**
 * A store for SynchronizedCounters.
 *
 * We need this store because the SynchronizedCounters cannot just be freed when the counter reaches
 * a certain value in a multi-threaded use-case. Thus, we have them in a store to avoid free'ing
 * them.
 * See here for details on the mutex problem: http://lwn.net/Articles/575460/
 */
struct SyncedCounterStore
{
   SynchronizedCounter** countersArray; // array of SynchronizedCounter pointers

   size_t numCounters; // initial total number of counters in the store

   size_t numAvailable; // number of currently available counters in the store

   fhgfs_bool storeValid;

   Mutex mutex;
   Condition newAddCond; // signaled when a counter is added to the store
};


#endif /* SYNCEDCOUNTERSTORE_H_ */

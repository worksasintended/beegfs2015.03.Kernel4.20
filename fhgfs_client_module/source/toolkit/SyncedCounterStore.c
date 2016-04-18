#include <common/toolkit/tree/PointerRBTree.h>
#include <common/toolkit/tree/PointerRBTreeIter.h>
#include "SyncedCounterStore.h"


/**
 * @param numCounters number of counters to be allocated.
 */
void SyncedCounterStore_init(SyncedCounterStore* this, size_t numCounters)
{
   Mutex_init(&this->mutex);
   Condition_init(&this->newAddCond);

   this->countersArray = NULL;

   this->numAvailable = 0;
   this->numCounters = numCounters;

   this->storeValid   = fhgfs_false;

   this->countersArray = (SynchronizedCounter**)os_kzalloc(
      numCounters * sizeof(SynchronizedCounter*) );
   if(!this->countersArray)
      return;

   this->storeValid = fhgfs_true; /* do not move below __SyncedCounterStore_initCounters() as
                                     it may set storeValid to false */

   __SyncedCounterStore_initCounters(this);
}

/**
 * @param numCounters number of counters to be allocated.
 */
struct SyncedCounterStore* SyncedCounterStore_construct(size_t numCounters)
{
   struct SyncedCounterStore* this = (SyncedCounterStore*)os_kmalloc(sizeof(*this) );

   if(likely(this) )
      SyncedCounterStore_init(this, numCounters);

   return this;
}

void SyncedCounterStore_uninit(SyncedCounterStore* this)
{
   size_t i;

   // delete counters
   for(i=0; i < this->numAvailable; i++)
   {
      SynchronizedCounter_destruct( (this->countersArray)[i] );
   }

   // normal clean-up
   Condition_uninit(&this->newAddCond);
   Mutex_uninit(&this->mutex);

   SAFE_KFREE(this->countersArray);
}

void SyncedCounterStore_destruct(SyncedCounterStore* this)
{
   SyncedCounterStore_uninit(this);

   os_kfree(this);
}

/**
 * Gets a counter from the store.
 * Waits if no counter is immediately available.
 *
 * @return a reset counter.
 */
SynchronizedCounter* SyncedCounterStore_waitForCounter(SyncedCounterStore* this)
{
   SynchronizedCounter* counter;

   Mutex_lock(&this->mutex);

   while(!this->numAvailable)
      Condition_wait(&this->newAddCond, &this->mutex);

   counter = (this->countersArray)[this->numAvailable-1];
   (this->numAvailable)--;

   SynchronizedCounter_reset(counter);

   Mutex_unlock(&this->mutex);

   return counter;
}

/**
 * Gets a counter from the store.
 * Waits interruptible if no counter is immediately available.
 *
 * @return a reset counter or NULL if interrupted by signal.
 */
SynchronizedCounter* SyncedCounterStore_waitInterruptibleForCounter(SyncedCounterStore* this)
{
   SynchronizedCounter* counter = NULL;
   cond_wait_res_t condRes;

   Mutex_lock(&this->mutex);

   while(!this->numAvailable)
   {
      condRes = Condition_waitInterruptible(&this->newAddCond, &this->mutex);
      if(condRes == COND_WAIT_SIGNAL)
         goto unlock_and_exit;
   }

   counter = (this->countersArray)[this->numAvailable-1];
   (this->numAvailable)--;

   SynchronizedCounter_reset(counter);

unlock_and_exit:
   Mutex_unlock(&this->mutex);

   return counter;
}

/**
 * Gets a counter from the store, fails if no counter is immediately available.
 *
 * @return reset counter if a counter was immediately available, NULL otherwise
 */
SynchronizedCounter* SyncedCounterStore_instantCounter(SyncedCounterStore* this)
{
   SynchronizedCounter* counter;

   Mutex_lock(&this->mutex);

   if(!this->numAvailable)
      counter = NULL;
   else
   {
      counter = (this->countersArray)[this->numAvailable-1];
      (this->numAvailable)--;

      SynchronizedCounter_reset(counter);
   }

   Mutex_unlock(&this->mutex);

   return counter;
}

/**
 * Re-add a counter to the pool.
 *
 * Caller must ensure that no one is relying on the counter-value when this is called.
 */
void SyncedCounterStore_addCounter(SyncedCounterStore* this, SynchronizedCounter* counter)
{
   if(unlikely(!counter) )
   {
      BEEGFS_BUG_ON(!counter, "NULL counter pointer detected!");
      return; // if the caller had a real counter, it will leak now
   }

   Mutex_lock(&this->mutex);

   Condition_signal(&this->newAddCond);

   (this->countersArray)[this->numAvailable] = counter;
   (this->numAvailable)++;

   Mutex_unlock(&this->mutex);
}

void __SyncedCounterStore_initCounters(SyncedCounterStore* this)
{
   size_t i;

   for(i = 0; i < this->numCounters; i++)
   {
      SynchronizedCounter* counter = SynchronizedCounter_construct();
      if(!counter)
      {
         printk_fhgfs(KERN_WARNING, "SyncedCounterStore_initCounters: failed to alloc "
            "counter number %lld\n", (long long)i);

         this->storeValid = fhgfs_false;

         break;
      }

      (this->countersArray)[i] = counter;
      (this->numAvailable)++;
   }
}

size_t SyncedCounterStore_getNumAvailable(SyncedCounterStore* this)
{
   size_t numAvailable;

   Mutex_lock(&this->mutex);

   numAvailable = this->numAvailable;

   Mutex_unlock(&this->mutex);

   return numAvailable;
}

/**
 * Returns whether this store was successfully initialized, so check this after _construct/_init().
 */
fhgfs_bool SyncedCounterStore_getStoreValid(SyncedCounterStore* this)
{
   return this->storeValid;
}


#ifndef SYNCHRONIZEDCOUNTER_H_
#define SYNCHRONIZEDCOUNTER_H_

#include <common/Common.h>
#include <common/threading/Mutex.h>
#include <common/threading/Condition.h>

struct SynchronizedCounter;
typedef struct SynchronizedCounter SynchronizedCounter;

static inline void SynchronizedCounter_init(SynchronizedCounter* this);
static inline SynchronizedCounter* SynchronizedCounter_construct(void);
static inline void SynchronizedCounter_uninit(SynchronizedCounter* this);
static inline void SynchronizedCounter_destruct(SynchronizedCounter* this);

// inliners
static inline void SynchronizedCounter_waitForCount(SynchronizedCounter* this, unsigned waitCount);
static inline fhgfs_bool SynchronizedCounter_waitForCountInterruptible(SynchronizedCounter* this,
   unsigned waitCount);
static inline void SynchronizedCounter_waitForMaxCount(SynchronizedCounter* this,
   unsigned maxCount);
static inline void SynchronizedCounter_incCount(SynchronizedCounter* this);
static inline void SynchronizedCounter_decCount(SynchronizedCounter* this);
static inline void SynchronizedCounter_reset(SynchronizedCounter* this);


/**
 * A counter variable protected by mutex and combined with a condition, so that it allows waiting
 * until a certain counter values is reached.
 *
 * Warning: This kind of counter may not be used in cases where it is free'd based on the counter
 *    value. That's why we have a store of counters in the App.
 *    See here for details: http://lwn.net/Articles/575460/
 */
struct SynchronizedCounter
{
   Mutex mutex;
   Condition cond; // broadcasted when count is changed

   unsigned count;
};


void SynchronizedCounter_init(SynchronizedCounter* this)
{
   Mutex_init(&this->mutex);
   Condition_init(&this->cond);

   this->count = 0;
}

SynchronizedCounter* SynchronizedCounter_construct(void)
{
   SynchronizedCounter* this = (SynchronizedCounter*)os_kmalloc(sizeof(*this) );

   if(likely(this) )
      SynchronizedCounter_init(this);

   return this;
}

void SynchronizedCounter_uninit(SynchronizedCounter* this)
{
   Condition_uninit(&this->cond);
   Mutex_uninit(&this->mutex);
}

void SynchronizedCounter_destruct(SynchronizedCounter* this)
{
   SynchronizedCounter_uninit(this);

   os_kfree(this);
}

void SynchronizedCounter_waitForCount(SynchronizedCounter* this, unsigned waitCount)
{
   Mutex_lock(&this->mutex);

   while(this->count != waitCount)
      Condition_wait(&this->cond, &this->mutex);

   Mutex_unlock(&this->mutex);
}

/**
 * Wait until this->count < maxCount
 */
void SynchronizedCounter_waitForMaxCount(SynchronizedCounter* this, unsigned maxCount)
{
   Mutex_lock(&this->mutex);

   while(this->count >= maxCount)
      Condition_wait(&this->cond, &this->mutex);

   Mutex_unlock(&this->mutex);
}


/**
 * @return true if count reached, false if interrupted
 */
fhgfs_bool SynchronizedCounter_waitForCountInterruptible(SynchronizedCounter* this,
   unsigned waitCount)
{
   fhgfs_bool retVal;
   cond_wait_res_t waitRes;

   Mutex_lock(&this->mutex);

   while(this->count != waitCount)
   {
      waitRes = Condition_waitInterruptible(&this->cond, &this->mutex);

      if(unlikely(waitRes == COND_WAIT_SIGNAL) )
      { // interrupted
         break;
      }
   }

   retVal = (this->count == waitCount) ? fhgfs_true : fhgfs_false;

   Mutex_unlock(&this->mutex);

   return retVal;
}

void SynchronizedCounter_incCount(SynchronizedCounter* this)
{
   Mutex_lock(&this->mutex);

   this->count++;

   Condition_broadcast(&this->cond);

   Mutex_unlock(&this->mutex);
}

void SynchronizedCounter_decCount(SynchronizedCounter* this)
{
   Mutex_lock(&this->mutex);

   BEEGFS_BUG_ON(!this->count, "decrease counter called, but counter already zero");

   this->count--;

   Condition_broadcast(&this->cond);

   Mutex_unlock(&this->mutex);
}


/**
 * Note: not synchronized
 */
void SynchronizedCounter_reset(SynchronizedCounter* this)
{
   this->count = 0;
}


#endif /*SYNCHRONIZEDCOUNTER_H_*/

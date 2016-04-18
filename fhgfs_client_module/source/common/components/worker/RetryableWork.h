#ifndef RETRYABLEWORK_H_
#define RETRYABLEWORK_H_

#include <common/Common.h>
#include <common/toolkit/Time.h>
#include "Work.h"

/**
 * This is an abstract class.
 */

struct RetryableWork;
typedef struct RetryableWork RetryableWork;

static inline void RetryableWork_init(RetryableWork* this);
//static inline RetryableWork* RetryableWork_construct(void);
static inline void RetryableWork_uninit(Work* this);
//static inline void RetryableWork_destruct(Work* this);

static inline fhgfs_bool RetryableWork_isRunnable(RetryableWork* this);

// setter & getters
static inline void RetryableWork_setInterruptor(RetryableWork* this, fhgfs_bool* interruptor);
static inline fhgfs_bool RetryableWork_getIsInterrupted(RetryableWork* this);


struct RetryableWork
{
   Work work;

   Time lastRetryT; // assumed to be init'ed only if "retryMS != 0"
   unsigned retryMS; // delay before next retry (0 for immediate)
   unsigned currentRetryNum; // number of used up retries so far

   fhgfs_bool* isInterrupted;
};


void RetryableWork_init(RetryableWork* this)
{
   Work_init( (Work*)this);

   this->retryMS = 0;
   this->currentRetryNum = 0;

   this->isInterrupted = NULL;
}

//struct RetryableWork* RetryableWork_construct(void)
//{
//   struct RetryableWork* this = (RetryableWork*)os_kmalloc(sizeof(*this) );
//
//   RetryableWork_init(this);
//
//   return this;
//}

void RetryableWork_uninit(Work* this)
{
   RetryableWork* thisCast = (RetryableWork*)this;

   if(thisCast->retryMS)
      Time_uninit(&thisCast->lastRetryT);

   Work_uninit( (Work*)this);
}

//void RetryableWork_destruct(Work* this)
//{
//   RetryableWork_uninit( (Work*)this);
//
//   os_kfree(this);
//}

/**
 * @return fhgfs_true if enough time passed to start a retry for this work
 */
fhgfs_bool RetryableWork_isRunnable(RetryableWork* this)
{
   if(!this->retryMS)
      return fhgfs_true;

   if(RetryableWork_getIsInterrupted(this) )
      return fhgfs_true;

   if(Time_elapsedMS(&this->lastRetryT) >= this->retryMS)
      return fhgfs_true;

   return fhgfs_false;
}

/**
 * @param interruptor pointer to a variable that becomes fhgfs_true if you want to stop any
 * further retries.
 */
void RetryableWork_setInterruptor(RetryableWork* this, fhgfs_bool* interruptor)
{
   this->isInterrupted = interruptor;
}

fhgfs_bool RetryableWork_getIsInterrupted(RetryableWork* this)
{
   return (this->isInterrupted && *this->isInterrupted);
}

#endif /* RETRYABLEWORK_H_ */

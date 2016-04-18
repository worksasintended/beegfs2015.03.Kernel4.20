#ifndef WorkQueueQUEUE_H_
#define WorkQueueQUEUE_H_

#include <common/threading/Mutex.h>
#include <common/threading/Condition.h>
#include <common/toolkit/list/PointerList.h>
#include <common/toolkit/list/PointerListIter.h>
#include <common/Common.h>
#include "Work.h"


struct WorkQueue;
typedef struct WorkQueue WorkQueue;

static inline void WorkQueue_init(WorkQueue* this);
static inline WorkQueue* WorkQueue_construct(void);
static inline void WorkQueue_uninit(WorkQueue* this);
static inline void WorkQueue_destruct(WorkQueue* this);

static inline Work* WorkQueue_popNextWork(WorkQueue* this);
static inline Work* WorkQueue_waitForNewWork(WorkQueue* this);
static inline Work* WorkQueue_waitForNewWorkT(WorkQueue* this, int timeoutMS);
static inline void WorkQueue_addWork(WorkQueue* this, Work* work);

// getters & setters
static inline size_t WorkQueue_getSize(WorkQueue* this);


struct WorkQueue
{
   PointerList workList;

   Mutex* mutex;
   Condition* newWorkCond;
};


void WorkQueue_init(WorkQueue* this)
{
   this->mutex = Mutex_construct();
   this->newWorkCond = Condition_construct();

   PointerList_init(&this->workList);
}

struct WorkQueue* WorkQueue_construct(void)
{
   struct WorkQueue* this = (WorkQueue*)os_kmalloc(sizeof(WorkQueue) );

   WorkQueue_init(this);

   return this;
}

void WorkQueue_uninit(WorkQueue* this)
{
   // delete remaining elems
   PointerListIter iter;

   PointerListIter_init(&iter, &this->workList);
   for( ; !PointerListIter_end(&iter); PointerListIter_next(&iter) )
   {
      Work* work = (Work*)PointerListIter_value(&iter);
      Work_virtualDestruct(work);
   }

   PointerListIter_uninit(&iter);

   // normal clean-up
   PointerList_uninit(&this->workList);

   Condition_destruct(this->newWorkCond);
   Mutex_destruct(this->mutex);
}

void WorkQueue_destruct(WorkQueue* this)
{
   WorkQueue_uninit(this);

   os_kfree(this);
}

/**
 * @return may be NULL if no work in the queue
 */
Work* WorkQueue_popNextWork(WorkQueue* this)
{
   Work* work = NULL;

   Mutex_lock(this->mutex);

   if(PointerList_length(&this->workList) )
   {
      work = (Work*)PointerList_getHeadValue(&this->workList);
      PointerList_removeHead(&this->workList);
   }

   Mutex_unlock(this->mutex);

   return work;
}

Work* WorkQueue_waitForNewWork(WorkQueue* this)
{
   Work* work;

   Mutex_lock(this->mutex);

   while(!PointerList_length(&this->workList) )
      Condition_waitInterruptible(this->newWorkCond, this->mutex);

   work = (Work*)PointerList_getHeadValue(&this->workList);
   PointerList_removeHead(&this->workList);

   Mutex_unlock(this->mutex);

   return work;
}

/**
 * @return NULL if timeout expired (or some other thread caught the added work), next work otherwise
 */
Work* WorkQueue_waitForNewWorkT(WorkQueue* this, int timeoutMS)
{
   Work* work = NULL;

   Mutex_lock(this->mutex);

   if(!PointerList_length(&this->workList) )
      Condition_timedwaitInterruptible(this->newWorkCond, this->mutex, timeoutMS);

   if(PointerList_length(&this->workList) )
   {
      work = (Work*)PointerList_getHeadValue(&this->workList);
      PointerList_removeHead(&this->workList);
   }

   Mutex_unlock(this->mutex);

   return work;
}

void WorkQueue_addWork(WorkQueue* this, Work* work)
{
   Mutex_lock(this->mutex);

   Condition_signal(this->newWorkCond);

   PointerList_append(&this->workList, work);

   Mutex_unlock(this->mutex);
}

size_t WorkQueue_getSize(WorkQueue* this)
{
   return PointerList_length(&this->workList);
}

#endif /*WorkQueueQUEUE_H_*/

#ifndef RETRYWORKER_H_
#define RETRYWORKER_H_

#include <app/log/Logger.h>
#include <app/App.h>
#include <common/threading/Thread.h>
#include <common/Common.h>
#include "Worker.h"
#include "WorkQueue.h"
#include "RetryableWork.h"

struct RetryWorker;
typedef struct RetryWorker RetryWorker;

static inline void RetryWorker_init(RetryWorker* this, App* app, const char* workerID,
   WorkQueue* workQueue);
static inline RetryWorker* RetryWorker_construct(App* app, const char* workerID,
   WorkQueue* workQueue);
static inline void RetryWorker_uninit(RetryWorker* this);
static inline void RetryWorker_destruct(RetryWorker* this);

static inline void __RetryWorker_run(Thread* this);
static inline void __RetryWorker_workLoop(RetryWorker* this);


struct RetryWorker
{
   Thread thread;

   App* app;

   char* bufIn;
   char* bufOut;
   WorkQueue* workQueue;
};


void RetryWorker_init(RetryWorker* this, App* app, const char* workerID, WorkQueue* workQueue)
{
   Thread_init( (Thread*)this, workerID, __RetryWorker_run);

   this->app = app;
   this->workQueue = workQueue;

   this->bufIn = (char*)os_vmalloc(WORKER_BUFIN_SIZE);
   this->bufOut = (char*)os_vmalloc(WORKER_BUFOUT_SIZE);
}

struct RetryWorker* RetryWorker_construct(App* app, const char* workerID, WorkQueue* workQueue)
{
   struct RetryWorker* this = (RetryWorker*)os_kmalloc(sizeof(*this) );

   RetryWorker_init(this, app, workerID, workQueue);

   return this;
}

void RetryWorker_uninit(RetryWorker* this)
{
   SAFE_VFREE(this->bufIn);
   SAFE_VFREE(this->bufOut);

   Thread_uninit( (Thread*)this);
}

void RetryWorker_destruct(RetryWorker* this)
{
   RetryWorker_uninit( (RetryWorker*)this);

   os_kfree(this);
}

void __RetryWorker_run(Thread* this)
{
   RetryWorker* thisCast = (RetryWorker*)this;

   const char* logContext = "RetryWorker (run)";
   Logger* log = App_getLogger(thisCast->app);

   __RetryWorker_workLoop(thisCast);

   Logger_log(log, 4, logContext, "Component stopped.");
}

void __RetryWorker_workLoop(RetryWorker* this)
{
   Logger* log = App_getLogger(this->app);
   const char* logContext = "Work loop";

   Thread* thisThread = (Thread*)this;

   const int waitTimeMS = 120000; // actually not really limited
   const int sleepTimeMS = 4000;

   size_t queueLen;
   size_t i;

   Logger_log(log, 4, logContext, "Ready");

   while(!Thread_getSelfTerminate(thisThread) )
   {
      //log.log(4, "Waiting for work...");

      Work* work = WorkQueue_waitForNewWorkT(this->workQueue, waitTimeMS);
      if(!work)
         continue;

      if(unlikely(Thread_getSelfTerminate(thisThread) ) )
      { /* special case: app probably has woken us up with dummy work to let us terminate, so don't
           process any more work and just delete it (rest will be deleted by queue destructor) */
         Work_virtualDestruct(work);

         break;
      }

      // we got work => walk over the whole queue (and process all work that is runnable)

      queueLen = WorkQueue_getSize(this->workQueue);

      for(i=0; ; i++)
      {
         if(!RetryableWork_isRunnable( (RetryableWork*)work) )
         { // this work is not ready for retry yet => re-insert into queue
            WorkQueue_addWork(this->workQueue, work);
         }
         else
         { // this work is ready to be processed
            work->process(work, this->bufIn, WORKER_BUFIN_SIZE, this->bufOut, WORKER_BUFOUT_SIZE);

            LOG_DEBUG(log, 4, logContext, "Work processed");

            Work_virtualDestruct(work);
         }

         /* note: termination checks need to be done below (and not as for-loop condition)
                  to make sure that we're not leaking work packages */

         if(i >= queueLen)
            break; // we walked over the whole queue (at least once)

         if(unlikely(Thread_getSelfTerminate(thisThread) ) )
            break;

         work = WorkQueue_popNextWork(this->workQueue);
         if(!work)
            break; // another RetryWorker did the remaining work
      }

      // if still work in the queue, take a rest before we proceed
      if(WorkQueue_getSize(this->workQueue) )
         _Thread_waitForSelfTerminateOrder(thisThread, sleepTimeMS);
   }
}


#endif /* RETRYWORKER_H_ */

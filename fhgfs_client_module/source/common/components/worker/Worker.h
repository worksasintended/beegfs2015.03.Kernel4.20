#ifndef WORKER_H_
#define WORKER_H_

#include <app/log/Logger.h>
#include <app/App.h>
#include <common/threading/Thread.h>
#include <common/Common.h>
#include "WorkQueue.h"

#define WORKER_BUFIN_SIZE     1048576
#define WORKER_BUFOUT_SIZE    WORKER_BUFIN_SIZE

struct Worker;
typedef struct Worker Worker;

static inline void Worker_init(Worker* this, App* app, const char* workerID, WorkQueue* workQueue);
static inline Worker* Worker_construct(App* app, const char* workerID, WorkQueue* workQueue);
static inline void Worker_uninit(Worker* this);
static inline void Worker_destruct(Worker* this);

static inline void __Worker_run(Thread* this);
static inline void __Worker_workLoop(Worker* this);
static inline void __Worker_initBuffers(Worker* this);


struct Worker
{
   Thread thread;

   App* app;

   char* bufIn;
   char* bufOut;
   WorkQueue* workQueue;
};


void Worker_init(Worker* this, App* app, const char* workerID, WorkQueue* workQueue)
{
   Thread_init( (Thread*)this, workerID, __Worker_run);

   this->bufIn = NULL;
   this->bufOut = NULL;

   this->app = app;
   this->workQueue = workQueue;
}

struct Worker* Worker_construct(App* app, const char* workerID, WorkQueue* workQueue)
{
   struct Worker* this = (Worker*)os_kmalloc(sizeof(Worker) );

   Worker_init(this, app, workerID, workQueue);

   return this;
}

void Worker_uninit(Worker* this)
{
   SAFE_VFREE(this->bufIn);
   SAFE_VFREE(this->bufOut);

   Thread_uninit( (Thread*)this);
}

void Worker_destruct(Worker* this)
{
   Worker_uninit( (Worker*)this);

   os_kfree(this);
}

void __Worker_run(Thread* this)
{
   Worker* thisCast = (Worker*)this;

   const char* logContext = "Worker (run)";
   Logger* log = App_getLogger(thisCast->app);

   __Worker_initBuffers(thisCast);

   __Worker_workLoop(thisCast);

   Logger_log(log, 4, logContext, "Component stopped.");
}

void __Worker_workLoop(Worker* this)
{
   Logger* log = App_getLogger(this->app);
   const char* logContext = "Work loop";

   Thread* thisThread = (Thread*)this;

   Logger_log(log, 4, logContext, "Ready");

   while(!Thread_getSelfTerminate(thisThread) )
   {
      //log.log(4, "Waiting for work...");

      Work* work = WorkQueue_waitForNewWork(this->workQueue);

      //log.log(4, "Got work");

      work->process(work, this->bufIn, WORKER_BUFIN_SIZE, this->bufOut, WORKER_BUFOUT_SIZE);

      LOG_DEBUG(log, 4, logContext, "Work processed");

      Work_virtualDestruct(work);
   }
}

/**
 * Note: Delayed init of buffers (better for NUMA).
 */
void __Worker_initBuffers(Worker* this)
{
   this->bufIn = (char*)os_vmalloc(WORKER_BUFIN_SIZE);
   this->bufOut = (char*)os_vmalloc(WORKER_BUFOUT_SIZE);
}

#endif /*WORKER_H_*/

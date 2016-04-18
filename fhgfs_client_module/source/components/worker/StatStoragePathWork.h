#ifndef STATSTORAGEPATHWORK_H_
#define STATSTORAGEPATHWORK_H_

#include <common/Common.h>
#include <common/net/sock/Socket.h>
#include <common/components/worker/RetryableWork.h>
#include <common/toolkit/SynchronizedCounter.h>
#include <common/toolkit/vector/StrCpyVec.h>
#include <common/toolkit/vector/IntCpyVec.h>
#include <common/toolkit/vector/Int64CpyVec.h>

struct StatStoragePathData;
typedef struct StatStoragePathData StatStoragePathData;


struct StatStoragePathWork;
typedef struct StatStoragePathWork StatStoragePathWork;

static inline void StatStoragePathWork_init(StatStoragePathWork* this, App* app,
   StatStoragePathData* statData, SynchronizedCounter* counter);
static inline StatStoragePathWork* StatStoragePathWork_construct(App* app,
   StatStoragePathData* statData, SynchronizedCounter* counter);
static inline void StatStoragePathWork_uninit(Work* this);
static inline void StatStoragePathWork_destruct(Work* this);

extern FhgfsOpsErr __StatStoragePathWork_communicate(StatStoragePathWork* this, Node* node,
   char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen);
extern fhgfs_bool __StatStoragePathWork_prepareRetry(StatStoragePathWork* this);

// virtual functions
extern void StatStoragePathWork_process(Work* this, char* bufIn, unsigned bufInLen,
   char* bufOut, unsigned bufOutLen);


struct StatStoragePathData
{
   uint16_t targetID;

   int64_t outSizeTotal;
   int64_t outSizeFree;
   FhgfsOpsErr outResult;
};

struct StatStoragePathWork
{
   RetryableWork retryableWork;

   App* app;

   StatStoragePathData* statData;
   SynchronizedCounter* counter;
};


void StatStoragePathWork_init(StatStoragePathWork* this, App* app, StatStoragePathData* statData,
   SynchronizedCounter* counter)
{
   RetryableWork_init( (RetryableWork*)this);

   this->app = app;

   this->statData = statData;
   this->counter = counter;

   // assign virtual functions
   ( (Work*)this)->uninit = StatStoragePathWork_uninit;

   ( (Work*)this)->process = StatStoragePathWork_process;
}

struct StatStoragePathWork* StatStoragePathWork_construct(App* app, StatStoragePathData* statData,
   SynchronizedCounter* counter)
{
   struct StatStoragePathWork* this =
      (StatStoragePathWork*)os_kmalloc(sizeof(*this) );

   if(likely(this) )
      StatStoragePathWork_init(this, app, statData, counter);

   return this;
}

void StatStoragePathWork_uninit(Work* this)
{
   RetryableWork_uninit(this);
}

void StatStoragePathWork_destruct(Work* this)
{
   StatStoragePathWork_uninit(this);

   os_kfree(this);
}


#endif /*STATSTORAGEPATHWORK_H_*/

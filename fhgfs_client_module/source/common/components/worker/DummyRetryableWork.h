#ifndef DUMMYRETRYABLEWORK_H_
#define DUMMYRETRYABLEWORK_H_

#include <common/Common.h>
#include "RetryableWork.h"

struct DummyRetryableWork;
typedef struct DummyRetryableWork DummyRetryableWork;

static inline void DummyRetryableWork_init(DummyRetryableWork* this);
static inline DummyRetryableWork* DummyRetryableWork_construct(void);
static inline void DummyRetryableWork_uninit(Work* this);
static inline void DummyRetryableWork_destruct(Work* this);

// virtual functions
static inline void DummyRetryableWork_process(Work* this, char* bufIn, unsigned bufInLen,
   char* bufOut, unsigned bufOutLen);


struct DummyRetryableWork
{
   RetryableWork retryableWork;
};


void DummyRetryableWork_init(DummyRetryableWork* this)
{
   RetryableWork_init( (RetryableWork*)this);


   // assign virtual functions
   ( (Work*)this)->uninit = DummyRetryableWork_uninit;

   ( (Work*)this)->process = DummyRetryableWork_process;
}

struct DummyRetryableWork* DummyRetryableWork_construct(void)
{
   struct DummyRetryableWork* this = (DummyRetryableWork*)os_kmalloc(sizeof(*this) );

   DummyRetryableWork_init(this);

   return this;
}

void DummyRetryableWork_uninit(Work* this)
{
   RetryableWork_uninit(this);
}

void DummyRetryableWork_destruct(Work* this)
{
   DummyRetryableWork_uninit(this);

   os_kfree(this);
}

void DummyRetryableWork_process(Work* this, char* bufIn, unsigned bufInLen, char* bufOut,
   unsigned bufOutLen)
{
   // nothing to be done here
}


#endif /* DUMMYRETRYABLEWORK_H_ */

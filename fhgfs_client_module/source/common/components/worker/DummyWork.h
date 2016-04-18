#ifndef DUMMYWORK_H_
#define DUMMYWORK_H_

#include <common/Common.h>
#include "Work.h"

struct DummyWork;
typedef struct DummyWork DummyWork;

static inline void DummyWork_init(DummyWork* this);
static inline DummyWork* DummyWork_construct(void);
static inline void DummyWork_uninit(Work* this);
static inline void DummyWork_destruct(Work* this);

// virtual functions
static inline void DummyWork_process(Work* this, char* bufIn, unsigned bufInLen,
   char* bufOut, unsigned bufOutLen);


struct DummyWork
{
   Work work;
};


void DummyWork_init(DummyWork* this)
{
   Work_init( (Work*)this);

   // assign virtual functions
   ( (Work*)this)->uninit = DummyWork_uninit;

   ( (Work*)this)->process = DummyWork_process;
}

struct DummyWork* DummyWork_construct(void)
{
   struct DummyWork* this = (DummyWork*)os_kmalloc(sizeof(*this) );

   DummyWork_init(this);

   return this;
}

void DummyWork_uninit(Work* this)
{
   Work_uninit(this);
}

void DummyWork_destruct(Work* this)
{
   DummyWork_uninit(this);

   os_kfree(this);
}

void DummyWork_process(Work* this, char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen)
{
   // nothing to be done here
}


#endif /*DUMMYWORK_H_*/

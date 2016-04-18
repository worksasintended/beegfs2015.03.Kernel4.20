#ifndef INTCPYLISTITER_H_
#define INTCPYLISTITER_H_

#include <common/toolkit/list/PointerListIter.h>
#include "IntCpyList.h"

struct IntCpyListIter;
typedef struct IntCpyListIter IntCpyListIter;

static inline void IntCpyListIter_init(IntCpyListIter* this, IntCpyList* list);
static inline IntCpyListIter* IntCpyListIter_construct(IntCpyList* list);
static inline void IntCpyListIter_uninit(IntCpyListIter* this);
static inline void IntCpyListIter_destruct(IntCpyListIter* this);
static inline fhgfs_bool IntCpyListIter_hasNext(IntCpyListIter* this);
static inline void IntCpyListIter_next(IntCpyListIter* this);
static inline int IntCpyListIter_value(IntCpyListIter* this);
static inline fhgfs_bool IntCpyListIter_end(IntCpyListIter* this);


struct IntCpyListIter
{
   struct PointerListIter pointerListIter;
};


void IntCpyListIter_init(IntCpyListIter* this, IntCpyList* list)
{
   PointerListIter_init( (PointerListIter*)this, (PointerList*)list);
}

IntCpyListIter* IntCpyListIter_construct(IntCpyList* list)
{
   IntCpyListIter* this = (IntCpyListIter*)os_kmalloc(sizeof(*this) );

   if(likely(this) )
      IntCpyListIter_init(this, list);

   return this;
}

void IntCpyListIter_uninit(IntCpyListIter* this)
{
   PointerListIter_uninit( (PointerListIter*)this);
}

void IntCpyListIter_destruct(IntCpyListIter* this)
{
   IntCpyListIter_uninit(this);

   os_kfree(this);
}

fhgfs_bool IntCpyListIter_hasNext(IntCpyListIter* this)
{
   return PointerListIter_hasNext( (PointerListIter*)this);
}

void IntCpyListIter_next(IntCpyListIter* this)
{
   PointerListIter_next( (PointerListIter*)this);
}

int IntCpyListIter_value(IntCpyListIter* this)
{
   return *(int*)PointerListIter_value( (PointerListIter*)this);
}

fhgfs_bool IntCpyListIter_end(IntCpyListIter* this)
{
   return PointerListIter_end( (PointerListIter*)this);
}


#endif /*INTCPYLISTITER_H_*/

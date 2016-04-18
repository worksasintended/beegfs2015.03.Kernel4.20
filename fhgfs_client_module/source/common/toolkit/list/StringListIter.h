#ifndef STRINGLISTITER_H_
#define STRINGLISTITER_H_

#include "StringList.h"
#include "PointerListIter.h"

struct StringListIter;
typedef struct StringListIter StringListIter;

static inline void StringListIter_init(StringListIter* this, StringList* list);
static inline StringListIter* StringListIter_construct(StringList* list);
static inline void StringListIter_uninit(StringListIter* this);
static inline void StringListIter_destruct(StringListIter* this);
static inline fhgfs_bool StringListIter_hasNext(StringListIter* this);
static inline void StringListIter_next(StringListIter* this);
static inline char* StringListIter_value(StringListIter* this);
static inline fhgfs_bool StringListIter_end(StringListIter* this);


struct StringListIter
{
   PointerListIter pointerListIter;
};


void StringListIter_init(StringListIter* this, StringList* list)
{
   PointerListIter_init( (PointerListIter*)this, (PointerList*)list); 
}

StringListIter* StringListIter_construct(StringList* list)
{
   StringListIter* this = (StringListIter*)os_kmalloc(sizeof(*this) );
   
   if(likely(this) )
      StringListIter_init(this, list);
   
   return this;
}

void StringListIter_uninit(StringListIter* this)
{
   PointerListIter_uninit( (PointerListIter*)this);
}

void StringListIter_destruct(StringListIter* this)
{
   StringListIter_uninit(this);
   
   os_kfree(this);
}

fhgfs_bool StringListIter_hasNext(StringListIter* this)
{
   return PointerListIter_hasNext( (PointerListIter*)this);
}

void StringListIter_next(StringListIter* this)
{
   PointerListIter_next( (PointerListIter*)this);
}

char* StringListIter_value(StringListIter* this)
{
   return (char*)PointerListIter_value( (PointerListIter*)this);
}

fhgfs_bool StringListIter_end(StringListIter* this)
{
   return PointerListIter_end( (PointerListIter*)this);
}


#endif /*STRINGLISTITER_H_*/

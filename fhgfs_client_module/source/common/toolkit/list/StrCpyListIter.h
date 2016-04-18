#ifndef STRCPYLISTITER_H_
#define STRCPYLISTITER_H_

#include "StrCpyList.h"
#include "StringListIter.h"

struct StrCpyListIter;
typedef struct StrCpyListIter StrCpyListIter;

static inline void StrCpyListIter_init(StrCpyListIter* this, StrCpyList* list);
static inline StrCpyListIter* StrCpyListIter_construct(StrCpyList* list);
static inline void StrCpyListIter_uninit(StrCpyListIter* this);
static inline void StrCpyListIter_destruct(StrCpyListIter* this);
static inline fhgfs_bool StrCpyListIter_hasNext(StrCpyListIter* this);
static inline void StrCpyListIter_next(StrCpyListIter* this);
static inline char* StrCpyListIter_value(StrCpyListIter* this);
static inline fhgfs_bool StrCpyListIter_end(StrCpyListIter* this);


struct StrCpyListIter
{
   struct StringListIter stringListIter;
};


void StrCpyListIter_init(StrCpyListIter* this, StrCpyList* list)
{
   StringListIter_init( (StringListIter*)this, (StringList*)list); 
}

StrCpyListIter* StrCpyListIter_construct(StrCpyList* list)
{
   StrCpyListIter* this = (StrCpyListIter*)os_kmalloc(sizeof(*this) );
   
   if(likely(this) )
      StrCpyListIter_init(this, list);
   
   return this;
}

void StrCpyListIter_uninit(StrCpyListIter* this)
{
   StringListIter_uninit( (StringListIter*)this);
}

void StrCpyListIter_destruct(StrCpyListIter* this)
{
   StrCpyListIter_uninit(this);
   
   os_kfree(this);
}

fhgfs_bool StrCpyListIter_hasNext(StrCpyListIter* this)
{
   return StringListIter_hasNext( (StringListIter*)this);
}

void StrCpyListIter_next(StrCpyListIter* this)
{
   StringListIter_next( (StringListIter*)this);
}

char* StrCpyListIter_value(StrCpyListIter* this)
{
   return (char*)StringListIter_value( (StringListIter*)this);
}

fhgfs_bool StrCpyListIter_end(StrCpyListIter* this)
{
   return StringListIter_end( (StringListIter*)this);
}


#endif /*STRCPYLISTITER_H_*/

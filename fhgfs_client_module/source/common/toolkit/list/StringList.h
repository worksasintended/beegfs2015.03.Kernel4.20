#ifndef STRINGLIST_H_
#define STRINGLIST_H_

#include "PointerList.h"

struct StringList;
typedef struct StringList StringList;

static inline void StringList_init(StringList* this);
static inline StringList* StringList_construct(void);
static inline void StringList_uninit(StringList* this);
static inline void StringList_destruct(StringList* this);
static inline void StringList_addHead(StringList* this, char* valuePointer);
static inline void StringList_append(StringList* this, char* valuePointer);
static inline size_t StringList_length(StringList* this);
static inline char* StringList_getTailValue(StringList* this);
static inline void StringList_clear(StringList* this);

struct StringList
{
   PointerList pointerList;
};


void StringList_init(StringList* this)
{
   PointerList_init( (PointerList*)this);
}

struct StringList* StringList_construct(void)
{
   struct StringList* this = (StringList*)os_kmalloc(sizeof(*this) );

   if(likely(this) )
      StringList_init(this);

   return this;
}

void StringList_uninit(StringList* this)
{
   PointerList_uninit( (PointerList*)this);
}

void StringList_destruct(StringList* this)
{
   StringList_uninit(this);

   os_kfree(this);
}

void StringList_addHead(StringList* this, char* valuePointer)
{
   PointerList_addHead( (PointerList*)this, valuePointer);
}

void StringList_append(StringList* this, char* valuePointer)
{
   PointerList_append( (PointerList*)this, valuePointer);
}

size_t StringList_length(StringList* this)
{
   return PointerList_length( (PointerList*)this);
}

char* StringList_getTailValue(StringList* this)
{
   return (char*)PointerList_getTailValue( (PointerList*)this);
}

void StringList_clear(StringList* this)
{
   PointerList_clear( (PointerList*)this);
}

#endif /*STRINGLIST_H_*/

#ifndef STRCPYLIST_H_
#define STRCPYLIST_H_

#include "StringList.h"

struct StrCpyList;
typedef struct StrCpyList StrCpyList;

static inline void StrCpyList_init(StrCpyList* this);
static inline StrCpyList* StrCpyList_construct(void);
static inline void StrCpyList_uninit(StrCpyList* this);
static inline void StrCpyList_destruct(StrCpyList* this);
static inline void StrCpyList_addHead(StrCpyList* this, const char* valuePointer);
static inline void StrCpyList_append(StrCpyList* this, const char* valuePointer);
static inline size_t StrCpyList_length(StrCpyList* this);
static inline char* StrCpyList_getTailValue(StrCpyList* this);
static inline void StrCpyList_clear(StrCpyList* this);

struct StrCpyList
{
   struct StringList stringList;
};


void StrCpyList_init(StrCpyList* this)
{
   StringList_init( (StringList*)this);
}

struct StrCpyList* StrCpyList_construct(void)
{
   struct StrCpyList* this = (StrCpyList*)os_kmalloc(sizeof(*this) );

   if(likely(this) )
      StrCpyList_init(this);

   return this;
}

void StrCpyList_uninit(StrCpyList* this)
{
   struct PointerListElem* elem = ( (PointerList*)this)->head;
   while(elem)
   {
      struct PointerListElem* next = elem->next;
      os_kfree(elem->valuePointer);
      elem = next;
   }


   StringList_uninit( (StringList*)this);
}

void StrCpyList_destruct(StrCpyList* this)
{
   StrCpyList_uninit(this);

   os_kfree(this);
}

void StrCpyList_addHead(StrCpyList* this, const char* valuePointer)
{
   size_t valueLen = os_strlen(valuePointer)+1;
   char* valueCopy = (char*)os_kmalloc(valueLen);
   os_memcpy(valueCopy, valuePointer, valueLen);

   StringList_addHead( (StringList*)this, valueCopy);
}

void StrCpyList_append(StrCpyList* this, const char* valuePointer)
{
   size_t valueLen = os_strlen(valuePointer)+1;
   char* valueCopy = (char*)os_kmalloc(valueLen);
   os_memcpy(valueCopy, valuePointer, valueLen);

   StringList_append( (StringList*)this, valueCopy);
}

size_t StrCpyList_length(StrCpyList* this)
{
   return StringList_length( (StringList*)this);
}

char* StrCpyList_getTailValue(StrCpyList* this)
{
   return StringList_getTailValue( (StringList*)this);
}

void StrCpyList_clear(StrCpyList* this)
{
   struct PointerListElem* elem = ( (PointerList*)this)->head;
   while(elem)
   {
      struct PointerListElem* next = elem->next;
      os_kfree(elem->valuePointer);
      elem = next;
   }

   StringList_clear( (StringList*)this);
}

#endif /*STRCPYLIST_H_*/

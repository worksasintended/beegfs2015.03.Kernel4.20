#ifndef INTCPYLIST_H_
#define INTCPYLIST_H_

#include <common/toolkit/list/PointerList.h>

/**
 * This class allocates mem twice: once for the new list elements and once more for the new integer.
 * That's obviously not good for performance, so this shouldn't be used in performance-critical
 * places.
 */

struct IntCpyList;
typedef struct IntCpyList IntCpyList;

static inline void IntCpyList_init(IntCpyList* this);
static inline IntCpyList* IntCpyList_construct(void);
static inline void IntCpyList_uninit(IntCpyList* this);
static inline void IntCpyList_destruct(IntCpyList* this);
static inline void IntCpyList_append(IntCpyList* this, int value);
static inline size_t IntCpyList_length(IntCpyList* this);
static inline void IntCpyList_clear(IntCpyList* this);

struct IntCpyList
{
   struct PointerList pointerList;
};


void IntCpyList_init(IntCpyList* this)
{
   PointerList_init( (PointerList*)this);
}

struct IntCpyList* IntCpyList_construct(void)
{
   struct IntCpyList* this = (IntCpyList*)os_kmalloc(sizeof(*this) );

   if(likely(this) )
      IntCpyList_init(this);

   return this;
}

void IntCpyList_uninit(IntCpyList* this)
{
   struct PointerListElem* elem = ( (PointerList*)this)->head;
   while(elem)
   {
      struct PointerListElem* next = elem->next;
      os_kfree(elem->valuePointer);
      elem = next;
   }


   PointerList_uninit( (PointerList*)this);
}

void IntCpyList_destruct(IntCpyList* this)
{
   IntCpyList_uninit(this);

   os_kfree(this);
}

void IntCpyList_append(IntCpyList* this, int value)
{
   int* valueCopyPointer = (int*)os_kmalloc(sizeof(int) );

   *valueCopyPointer = value;

   PointerList_append( (PointerList*)this, valueCopyPointer);
}

static inline size_t IntCpyList_length(IntCpyList* this)
{
   return PointerList_length( (PointerList*)this);
}

void IntCpyList_clear(IntCpyList* this)
{
   struct PointerListElem* elem = ( (PointerList*)this)->head;
   while(elem)
   {
      struct PointerListElem* next = elem->next;
      os_kfree(elem->valuePointer);
      elem = next;
   }

   PointerList_clear( (PointerList*)this);
}

#endif /*INTCPYLIST_H_*/

#ifndef INTCPYVEC_H_
#define INTCPYVEC_H_

#include <common/toolkit/list/IntCpyList.h>

/**
 * Note: Derived from the corresponding list. Use the list iterator for read-only access
 */

struct IntCpyVec;
typedef struct IntCpyVec IntCpyVec;

static inline void IntCpyVec_init(IntCpyVec* this);
static inline IntCpyVec* IntCpyVec_construct(void);
static inline void IntCpyVec_uninit(IntCpyVec* this);
static inline void IntCpyVec_destruct(IntCpyVec* this);
static inline void IntCpyVec_append(IntCpyVec* this, int value);
static inline size_t IntCpyVec_length(IntCpyVec* this);
static inline void IntCpyVec_clear(IntCpyVec* this);

// getters & setters
static inline int IntCpyVec_at(IntCpyVec* this, size_t index);


struct IntCpyVec
{
   IntCpyList IntCpyList;
   
   int** vecArray;
   size_t vecArrayLen;
};


void IntCpyVec_init(IntCpyVec* this)
{
   IntCpyList_init( (IntCpyList*)this);
   
   this->vecArrayLen = 4;
   this->vecArray = (int**)os_kmalloc(
      this->vecArrayLen * sizeof(int*) );
}

struct IntCpyVec* IntCpyVec_construct(void)
{
   struct IntCpyVec* this = (IntCpyVec*)os_kmalloc(sizeof(*this) );
   
   if(likely(this) )
      IntCpyVec_init(this);
   
   return this;
}

void IntCpyVec_uninit(IntCpyVec* this)
{
   os_kfree(this->vecArray);

   IntCpyList_uninit( (IntCpyList*)this);
}

void IntCpyVec_destruct(IntCpyVec* this)
{
   IntCpyVec_uninit(this);
   
   os_kfree(this);
}

void IntCpyVec_append(IntCpyVec* this, int value)
{
   size_t newListLen;
   PointerListElem* lastElem;
   int* lastElemValuePointer;
   
   IntCpyList_append( (IntCpyList*)this, value);
   
   newListLen = IntCpyList_length( (IntCpyList*)this);

   // check if we have enough buffer space for new elem 
   if(newListLen > this->vecArrayLen)
   { // double vector array size (alloc new, copy, delete old, switch to new)
      int** newVecArray = (int**)os_kmalloc(this->vecArrayLen * sizeof(int*) * 2);
      os_memcpy(newVecArray, this->vecArray, this->vecArrayLen * sizeof(int*) );

      os_kfree(this->vecArray);

      this->vecArrayLen = this->vecArrayLen * 2;
      this->vecArray = newVecArray;
   }
   
   // get last elem and add the valuePointer to the array
   lastElem = PointerList_getTail( (PointerList*)this);
   lastElemValuePointer = (int*)lastElem->valuePointer;
   (this->vecArray)[newListLen-1] = lastElemValuePointer;
}

size_t IntCpyVec_length(IntCpyVec* this)
{
   return IntCpyList_length( (IntCpyList*)this);
}

int IntCpyVec_at(IntCpyVec* this, size_t index)
{
   BEEGFS_BUG_ON_DEBUG(index >= IntCpyVec_length(this), "Index out of bounds");

   return *(this->vecArray[index]);
}

void IntCpyVec_clear(IntCpyVec* this)
{
   IntCpyList_clear( (IntCpyList*)this);
}

#endif /*INTCPYVEC_H_*/

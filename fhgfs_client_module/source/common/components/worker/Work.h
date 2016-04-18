#ifndef WORK_H_
#define WORK_H_

#include <common/Common.h>

/**
 * Note: Contains "virtual" methods.
 * Derived classes have a destructor with a Work-Pointer (instead of the real type)
 * because of the generic virtual destructor signature.
 */

struct Work;
typedef struct Work Work;

static inline void Work_init(Work* this);
//static inline Work* Work_construct(void);
static inline void Work_uninit(Work* this);
//static inline void Work_destruct(Work* this);

static inline void Work_virtualDestruct(Work* this);


struct Work
{
   // virtual functions
   void (*uninit) (Work* this);

   void (*process) (Work* this, char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen);
};


void Work_init(Work* this)
{
   os_memset(this, 0, sizeof(Work) ); // clear function pointers etc.
}

//struct Work* Work_construct(void)
//{
//   struct Work* this = (Work*)os_kmalloc(sizeof(*this) );
//
//   Work_init(this);
//
//   return this;
//}

void Work_uninit(Work* this)
{
   // nothing to be done here
}

//void Work_destruct(Work* this)
//{
//   Work_uninit(this);
//
//   os_kfree(this);
//}

/**
 * Calls the virtual uninit method and kfrees the object.
 */
void Work_virtualDestruct(Work* this)
{
   this->uninit(this);
   os_kfree(this);
}


#endif /*WORK_H_*/

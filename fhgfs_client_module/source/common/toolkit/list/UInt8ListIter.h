#ifndef UINT8LISTITER_H_
#define UINT8LISTITER_H_

#include <common/toolkit/list/PointerListIter.h>
#include "UInt8List.h"

struct UInt8ListIter;
typedef struct UInt8ListIter UInt8ListIter;

static inline void UInt8ListIter_init(UInt8ListIter* this, UInt8List* list);
static inline UInt8ListIter* UInt8ListIter_construct(UInt8List* list);
static inline void UInt8ListIter_uninit(UInt8ListIter* this);
static inline void UInt8ListIter_destruct(UInt8ListIter* this);
static inline fhgfs_bool UInt8ListIter_hasNext(UInt8ListIter* this);
static inline void UInt8ListIter_next(UInt8ListIter* this);
static inline uint8_t UInt8ListIter_value(UInt8ListIter* this);
static inline fhgfs_bool UInt8ListIter_end(UInt8ListIter* this);


struct UInt8ListIter
{
   struct PointerListIter pointerListIter;
};


void UInt8ListIter_init(UInt8ListIter* this, UInt8List* list)
{
   PointerListIter_init( (PointerListIter*)this, (PointerList*)list);
}

UInt8ListIter* UInt8ListIter_construct(UInt8List* list)
{
   UInt8ListIter* this = (UInt8ListIter*)os_kmalloc(sizeof(*this) );

   if(likely(this) )
      UInt8ListIter_init(this, list);

   return this;
}

void UInt8ListIter_uninit(UInt8ListIter* this)
{
   PointerListIter_uninit( (PointerListIter*)this);
}

void UInt8ListIter_destruct(UInt8ListIter* this)
{
   UInt8ListIter_uninit(this);

   os_kfree(this);
}

fhgfs_bool UInt8ListIter_hasNext(UInt8ListIter* this)
{
   return PointerListIter_hasNext( (PointerListIter*)this);
}

void UInt8ListIter_next(UInt8ListIter* this)
{
   PointerListIter_next( (PointerListIter*)this);
}

uint8_t UInt8ListIter_value(UInt8ListIter* this)
{
   return (uint8_t)(size_t)PointerListIter_value( (PointerListIter*)this);
}

fhgfs_bool UInt8ListIter_end(UInt8ListIter* this)
{
   return PointerListIter_end( (PointerListIter*)this);
}


#endif /* UINT8LISTITER_H_ */

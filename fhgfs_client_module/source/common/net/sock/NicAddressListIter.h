#ifndef NICADDRESSLISTITER_H_
#define NICADDRESSLISTITER_H_

#include <common/toolkit/list/PointerListIter.h>
#include <common/Common.h>
#include "NicAddressList.h"

struct NicAddressListIter;
typedef struct NicAddressListIter NicAddressListIter;

static inline void NicAddressListIter_init(NicAddressListIter* this, NicAddressList* list);
static inline NicAddressListIter* NicAddressListIter_construct(NicAddressList* list);
static inline void NicAddressListIter_uninit(NicAddressListIter* this);
static inline void NicAddressListIter_destruct(NicAddressListIter* this);
static inline fhgfs_bool NicAddressListIter_hasNext(NicAddressListIter* this);
static inline void NicAddressListIter_next(NicAddressListIter* this);
static inline NicAddress* NicAddressListIter_value(NicAddressListIter* this);
static inline fhgfs_bool NicAddressListIter_end(NicAddressListIter* this);


struct NicAddressListIter
{
   PointerListIter pointerListIter;
};


void NicAddressListIter_init(NicAddressListIter* this, NicAddressList* list)
{
   PointerListIter_init( (PointerListIter*)this, (PointerList*)list); 
}

NicAddressListIter* NicAddressListIter_construct(NicAddressList* list)
{
   NicAddressListIter* this = (NicAddressListIter*)os_kmalloc(
      sizeof(NicAddressListIter) );
   
   NicAddressListIter_init(this, list);
   
   return this;
}

void NicAddressListIter_uninit(NicAddressListIter* this)
{
   PointerListIter_uninit( (PointerListIter*)this);
}

void NicAddressListIter_destruct(NicAddressListIter* this)
{
   NicAddressListIter_uninit(this);
   
   os_kfree(this);
}

fhgfs_bool NicAddressListIter_hasNext(NicAddressListIter* this)
{
   return PointerListIter_hasNext( (PointerListIter*)this);
}

void NicAddressListIter_next(NicAddressListIter* this)
{
   PointerListIter_next( (PointerListIter*)this);
}

NicAddress* NicAddressListIter_value(NicAddressListIter* this)
{
   return (NicAddress*)PointerListIter_value( (PointerListIter*)this);
}

fhgfs_bool NicAddressListIter_end(NicAddressListIter* this)
{
   return PointerListIter_end( (PointerListIter*)this);
}



#endif /*NICADDRESSLISTITER_H_*/

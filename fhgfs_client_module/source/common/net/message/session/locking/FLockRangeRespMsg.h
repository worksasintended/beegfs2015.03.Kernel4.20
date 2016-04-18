#ifndef FLOCKRANGERESPMSG_H_
#define FLOCKRANGERESPMSG_H_

#include <common/net/message/SimpleIntMsg.h>
#include <common/storage/StorageErrors.h>

/**
 * This message is for deserialiazation (incoming) only, serialization is not implemented!!
 */

struct FLockRangeRespMsg;
typedef struct FLockRangeRespMsg FLockRangeRespMsg;

static inline void FLockRangeRespMsg_init(FLockRangeRespMsg* this);
static inline FLockRangeRespMsg* FLockRangeRespMsg_construct(void);
static inline void FLockRangeRespMsg_uninit(NetMessage* this);
static inline void FLockRangeRespMsg_destruct(NetMessage* this);

// getters & setters
static inline FhgfsOpsErr FLockRangeRespMsg_getResult(FLockRangeRespMsg* this);

struct FLockRangeRespMsg
{
   SimpleIntMsg simpleIntMsg;
};


void FLockRangeRespMsg_init(FLockRangeRespMsg* this)
{
   SimpleIntMsg_init( (SimpleIntMsg*)this, NETMSGTYPE_FLockRangeResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = FLockRangeRespMsg_uninit;
}

FLockRangeRespMsg* FLockRangeRespMsg_construct(void)
{
   struct FLockRangeRespMsg* this = os_kmalloc(sizeof(*this) );

   FLockRangeRespMsg_init(this);

   return this;
}

void FLockRangeRespMsg_uninit(NetMessage* this)
{
   SimpleIntMsg_uninit( (NetMessage*)this);
}

void FLockRangeRespMsg_destruct(NetMessage* this)
{
   FLockRangeRespMsg_uninit( (NetMessage*)this);

   os_kfree(this);
}


FhgfsOpsErr FLockRangeRespMsg_getResult(FLockRangeRespMsg* this)
{
   return (FhgfsOpsErr)SimpleIntMsg_getValue( (SimpleIntMsg*)this);
}



#endif /* FLOCKRANGERESPMSG_H_ */

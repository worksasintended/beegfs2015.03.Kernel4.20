#ifndef FLOCKAPPENDRESPMSG_H_
#define FLOCKAPPENDRESPMSG_H_

#include <common/net/message/SimpleIntMsg.h>
#include <common/storage/StorageErrors.h>

/**
 * This message is for deserialiazation (incoming) only, serialization is not implemented!!
 */

struct FLockAppendRespMsg;
typedef struct FLockAppendRespMsg FLockAppendRespMsg;

static inline void FLockAppendRespMsg_init(FLockAppendRespMsg* this);
static inline FLockAppendRespMsg* FLockAppendRespMsg_construct(void);
static inline void FLockAppendRespMsg_uninit(NetMessage* this);
static inline void FLockAppendRespMsg_destruct(NetMessage* this);

// getters & setters
static inline FhgfsOpsErr FLockAppendRespMsg_getResult(FLockAppendRespMsg* this);


struct FLockAppendRespMsg
{
   SimpleIntMsg simpleIntMsg;
};


void FLockAppendRespMsg_init(FLockAppendRespMsg* this)
{
   SimpleIntMsg_init( (SimpleIntMsg*)this, NETMSGTYPE_FLockAppendResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = FLockAppendRespMsg_uninit;
}

FLockAppendRespMsg* FLockAppendRespMsg_construct(void)
{
   struct FLockAppendRespMsg* this = os_kmalloc(sizeof(*this) );

   FLockAppendRespMsg_init(this);

   return this;
}

void FLockAppendRespMsg_uninit(NetMessage* this)
{
   SimpleIntMsg_uninit( (NetMessage*)this);
}

void FLockAppendRespMsg_destruct(NetMessage* this)
{
   FLockAppendRespMsg_uninit( (NetMessage*)this);

   os_kfree(this);
}


FhgfsOpsErr FLockAppendRespMsg_getResult(FLockAppendRespMsg* this)
{
   return (FhgfsOpsErr)SimpleIntMsg_getValue( (SimpleIntMsg*)this);
}


#endif /* FLOCKAPPENDRESPMSG_H_ */

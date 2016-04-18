#ifndef FLOCKENTRYRESPMSG_H_
#define FLOCKENTRYRESPMSG_H_

#include <common/net/message/SimpleIntMsg.h>
#include <common/storage/StorageErrors.h>

/**
 * This message is for deserialiazation (incoming) only, serialization is not implemented!!
 */

struct FLockEntryRespMsg;
typedef struct FLockEntryRespMsg FLockEntryRespMsg;

static inline void FLockEntryRespMsg_init(FLockEntryRespMsg* this);
static inline FLockEntryRespMsg* FLockEntryRespMsg_construct(void);
static inline void FLockEntryRespMsg_uninit(NetMessage* this);
static inline void FLockEntryRespMsg_destruct(NetMessage* this);

// getters & setters
static inline FhgfsOpsErr FLockEntryRespMsg_getResult(FLockEntryRespMsg* this);


struct FLockEntryRespMsg
{
   SimpleIntMsg simpleIntMsg;
};


void FLockEntryRespMsg_init(FLockEntryRespMsg* this)
{
   SimpleIntMsg_init( (SimpleIntMsg*)this, NETMSGTYPE_FLockEntryResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = FLockEntryRespMsg_uninit;
}

FLockEntryRespMsg* FLockEntryRespMsg_construct(void)
{
   struct FLockEntryRespMsg* this = os_kmalloc(sizeof(*this) );

   FLockEntryRespMsg_init(this);

   return this;
}

void FLockEntryRespMsg_uninit(NetMessage* this)
{
   SimpleIntMsg_uninit( (NetMessage*)this);
}

void FLockEntryRespMsg_destruct(NetMessage* this)
{
   FLockEntryRespMsg_uninit( (NetMessage*)this);

   os_kfree(this);
}


FhgfsOpsErr FLockEntryRespMsg_getResult(FLockEntryRespMsg* this)
{
   return (FhgfsOpsErr)SimpleIntMsg_getValue( (SimpleIntMsg*)this);
}


#endif /* FLOCKENTRYRESPMSG_H_ */

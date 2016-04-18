#ifndef REFRESHTARGETSTATESMSGEX_H_
#define REFRESHTARGETSTATESMSGEX_H_

#include <common/net/message/NetMessage.h>
#include <common/Common.h>

/**
 * Note: Only the receive/deserialize part of this message is implemented (so it cannot be sent).
 * Note: Processing only sends response when ackID is set (no RefreshTargetStatesRespMsg
 * implemented).
 */

struct RefreshTargetStatesMsgEx;
typedef struct RefreshTargetStatesMsgEx RefreshTargetStatesMsgEx;

static inline void RefreshTargetStatesMsgEx_init(RefreshTargetStatesMsgEx* this);
static inline RefreshTargetStatesMsgEx* RefreshTargetStatesMsgEx_construct(void);
static inline void RefreshTargetStatesMsgEx_uninit(NetMessage* this);
static inline void RefreshTargetStatesMsgEx_destruct(NetMessage* this);

// virtual functions
extern fhgfs_bool RefreshTargetStatesMsgEx_deserializePayload(NetMessage* this, const char* buf,
   size_t bufLen);
extern unsigned RefreshTargetStatesMsgEx_calcMessageLength(NetMessage* this);
extern fhgfs_bool __RefreshTargetStatesMsgEx_processIncoming(NetMessage* this, struct App* app,
   fhgfs_sockaddr_in* fromAddr, struct Socket* sock, char* respBuf, size_t bufLen);

// getters & setters
static inline const char* RefreshTargetStatesMsgEx_getAckID(RefreshTargetStatesMsgEx* this);


struct RefreshTargetStatesMsgEx
{
   NetMessage netMessage;

   unsigned ackIDLen;
   const char* ackID;
};


void RefreshTargetStatesMsgEx_init(RefreshTargetStatesMsgEx* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_RefreshTargetStates);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = RefreshTargetStatesMsgEx_uninit;

   //( (NetMessage*)this)->serializePayload = RefreshTargetStatesMsgEx_serializePayload; // not impl'ed
   ( (NetMessage*)this)->deserializePayload = RefreshTargetStatesMsgEx_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = RefreshTargetStatesMsgEx_calcMessageLength;

   ( (NetMessage*)this)->processIncoming = __RefreshTargetStatesMsgEx_processIncoming;
}


RefreshTargetStatesMsgEx* RefreshTargetStatesMsgEx_construct(void)
{
   struct RefreshTargetStatesMsgEx* this = os_kmalloc(sizeof(*this) );

   if (likely(this))
      RefreshTargetStatesMsgEx_init(this);

   return this;
}

void RefreshTargetStatesMsgEx_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void RefreshTargetStatesMsgEx_destruct(NetMessage* this)
{
   RefreshTargetStatesMsgEx_uninit(this);

   os_kfree(this);
}

const char* RefreshTargetStatesMsgEx_getAckID(RefreshTargetStatesMsgEx* this)
{
   return this->ackID;
}

#endif /* REFRESHTARGETSTATESMSGEX_H_ */

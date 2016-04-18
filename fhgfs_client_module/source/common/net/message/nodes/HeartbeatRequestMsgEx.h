#ifndef HEARTBEATREQUESTMSGEX_H_
#define HEARTBEATREQUESTMSGEX_H_

#include "../SimpleMsg.h"


struct HeartbeatRequestMsgEx;
typedef struct HeartbeatRequestMsgEx HeartbeatRequestMsgEx;

static inline void HeartbeatRequestMsgEx_init(HeartbeatRequestMsgEx* this);
static inline HeartbeatRequestMsgEx* HeartbeatRequestMsgEx_construct(void);
static inline void HeartbeatRequestMsgEx_uninit(NetMessage* this);
static inline void HeartbeatRequestMsgEx_destruct(NetMessage* this);

// virtual functions
extern fhgfs_bool __HeartbeatRequestMsgEx_processIncoming(NetMessage* this, struct App* app,
   fhgfs_sockaddr_in* fromAddr, struct Socket* sock, char* respBuf, size_t bufLen);


struct HeartbeatRequestMsgEx
{
   SimpleMsg simpleMsg;
};


void HeartbeatRequestMsgEx_init(HeartbeatRequestMsgEx* this)
{
   SimpleMsg_init( (SimpleMsg*)this, NETMSGTYPE_HeartbeatRequest);

   // virtual functions
   ( (NetMessage*)this)->uninit = HeartbeatRequestMsgEx_uninit;

   ( (NetMessage*)this)->processIncoming = __HeartbeatRequestMsgEx_processIncoming;
}

HeartbeatRequestMsgEx* HeartbeatRequestMsgEx_construct(void)
{
   struct HeartbeatRequestMsgEx* this = os_kmalloc(sizeof(*this) );
   
   HeartbeatRequestMsgEx_init(this);
   
   return this;
}

void HeartbeatRequestMsgEx_uninit(NetMessage* this)
{
   SimpleMsg_uninit(this);
}

void HeartbeatRequestMsgEx_destruct(NetMessage* this)
{
   HeartbeatRequestMsgEx_uninit(this);
   
   os_kfree(this);
}

#endif /* HEARTBEATREQUESTMSGEX_H_ */

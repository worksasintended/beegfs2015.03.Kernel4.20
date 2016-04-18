#ifndef AUTHENTICATECHANNELMSG_H_
#define AUTHENTICATECHANNELMSG_H_

#include <common/net/message/SimpleInt64Msg.h>


struct AuthenticateChannelMsg;
typedef struct AuthenticateChannelMsg AuthenticateChannelMsg;


static inline void AuthenticateChannelMsg_init(AuthenticateChannelMsg* this);
static inline void AuthenticateChannelMsg_initFromValue(AuthenticateChannelMsg* this,
   uint64_t authHash);
static inline AuthenticateChannelMsg* AuthenticateChannelMsg_construct(void);
static inline AuthenticateChannelMsg* AuthenticateChannelMsg_constructFromValue(uint64_t authHash);
static inline void AuthenticateChannelMsg_uninit(NetMessage* this);
static inline void AuthenticateChannelMsg_destruct(NetMessage* this);


struct AuthenticateChannelMsg
{
   SimpleInt64Msg simpleInt64Msg;
};


void AuthenticateChannelMsg_init(AuthenticateChannelMsg* this)
{
   SimpleInt64Msg_init( (SimpleInt64Msg*)this, NETMSGTYPE_AuthenticateChannel);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = AuthenticateChannelMsg_uninit;
}

void AuthenticateChannelMsg_initFromValue(AuthenticateChannelMsg* this, uint64_t authHash)
{
   SimpleInt64Msg_initFromValue( (SimpleInt64Msg*)this, NETMSGTYPE_AuthenticateChannel, authHash);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = AuthenticateChannelMsg_uninit;
}

AuthenticateChannelMsg* AuthenticateChannelMsg_construct(void)
{
   struct AuthenticateChannelMsg* this = os_kmalloc(sizeof(*this) );

   AuthenticateChannelMsg_init(this);

   return this;
}

AuthenticateChannelMsg* AuthenticateChannelMsg_constructFromValue(uint64_t authHash)
{
   struct AuthenticateChannelMsg* this = os_kmalloc(sizeof(*this) );

   AuthenticateChannelMsg_initFromValue(this, authHash);

   return this;
}

void AuthenticateChannelMsg_uninit(NetMessage* this)
{
   SimpleInt64Msg_uninit( (NetMessage*)this);
}

void AuthenticateChannelMsg_destruct(NetMessage* this)
{
   AuthenticateChannelMsg_uninit( (NetMessage*)this);

   os_kfree(this);
}


#endif /* AUTHENTICATECHANNELMSG_H_ */

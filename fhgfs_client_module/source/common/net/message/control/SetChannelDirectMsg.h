#ifndef SETCHANNELDIRECTMSG_H_
#define SETCHANNELDIRECTMSG_H_

#include <common/net/message/SimpleIntMsg.h>


struct SetChannelDirectMsg;
typedef struct SetChannelDirectMsg SetChannelDirectMsg;

static inline void SetChannelDirectMsg_init(SetChannelDirectMsg* this);
static inline void SetChannelDirectMsg_initFromValue(SetChannelDirectMsg* this, int value);
static inline SetChannelDirectMsg* SetChannelDirectMsg_construct(void);
static inline SetChannelDirectMsg* SetChannelDirectMsg_constructFromValue(int value);
static inline void SetChannelDirectMsg_uninit(NetMessage* this);
static inline void SetChannelDirectMsg_destruct(NetMessage* this);

// getters & setters
static inline int SetChannelDirectMsg_getValue(SetChannelDirectMsg* this);

struct SetChannelDirectMsg
{
   SimpleIntMsg simpleIntMsg;
};


void SetChannelDirectMsg_init(SetChannelDirectMsg* this)
{
   SimpleIntMsg_init( (SimpleIntMsg*)this, NETMSGTYPE_SetChannelDirect);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = SetChannelDirectMsg_uninit;
}
   
void SetChannelDirectMsg_initFromValue(SetChannelDirectMsg* this, int value)
{
   SimpleIntMsg_initFromValue( (SimpleIntMsg*)this, NETMSGTYPE_SetChannelDirect, value);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = SetChannelDirectMsg_uninit;
}

SetChannelDirectMsg* SetChannelDirectMsg_construct(void)
{
   struct SetChannelDirectMsg* this = os_kmalloc(sizeof(struct SetChannelDirectMsg) );
   
   SetChannelDirectMsg_init(this);
   
   return this;
}

SetChannelDirectMsg* SetChannelDirectMsg_constructFromValue(int value)
{
   struct SetChannelDirectMsg* this = os_kmalloc(sizeof(struct SetChannelDirectMsg) );
   
   SetChannelDirectMsg_initFromValue(this, value);
   
   return this;
}

void SetChannelDirectMsg_uninit(NetMessage* this)
{
   SimpleIntMsg_uninit( (NetMessage*)this);
}

void SetChannelDirectMsg_destruct(NetMessage* this)
{
   SetChannelDirectMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


int SetChannelDirectMsg_getValue(SetChannelDirectMsg* this)
{
   return SimpleIntMsg_getValue( (SimpleIntMsg*)this);
}

#endif /*SETCHANNELDIRECTMSG_H_*/

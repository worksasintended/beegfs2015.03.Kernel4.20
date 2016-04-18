#ifndef GETTARGETSTATESMSG_H
#define GETTARGETSTATESMSG_H

#include <common/net/message/SimpleIntMsg.h>

struct GetTargetStatesMsg;
typedef struct GetTargetStatesMsg GetTargetStatesMsg;

static inline void GetTargetStatesMsg_init(GetTargetStatesMsg* this, NodeType nodeType);
static inline GetTargetStatesMsg* GetTargetStatesMsg_construct(NodeType nodeType);
static inline void GetTargetStatesMsg_uninit(NetMessage* this);
static inline void GetTargetStatesMsg_destruct(NetMessage* this);

struct GetTargetStatesMsg
{
   SimpleIntMsg simpleIntMsg;
};

void GetTargetStatesMsg_init(GetTargetStatesMsg* this, NodeType nodeType)
{
   SimpleIntMsg_initFromValue( (SimpleIntMsg*)this, NETMSGTYPE_GetTargetStates, nodeType);

   // virtual functions
   ( (NetMessage*)this)->uninit = GetTargetStatesMsg_uninit;
}

GetTargetStatesMsg* GetTargetStatesMsg_construct(NodeType nodeType)
{
   struct GetTargetStatesMsg* this = os_kmalloc(sizeof(*this) );

   if(likely(this) )
      GetTargetStatesMsg_init(this, nodeType);

   return this;
}

void GetTargetStatesMsg_uninit(NetMessage* this)
{
   SimpleIntMsg_uninit(this);
}

void GetTargetStatesMsg_destruct(NetMessage* this)
{
   GetTargetStatesMsg_uninit(this);

   os_kfree(this);
}

#endif /* GETTARGETSTATESMSG_H */

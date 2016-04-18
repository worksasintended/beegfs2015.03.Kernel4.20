#ifndef REMOVENODERESPMSG_H_
#define REMOVENODERESPMSG_H_

#include <common/net/message/SimpleIntMsg.h>


struct RemoveNodeRespMsg;
typedef struct RemoveNodeRespMsg RemoveNodeRespMsg;

static inline void RemoveNodeRespMsg_init(RemoveNodeRespMsg* this);
static inline void RemoveNodeRespMsg_initFromValue(RemoveNodeRespMsg* this, int value);
static inline RemoveNodeRespMsg* RemoveNodeRespMsg_construct(void);
static inline RemoveNodeRespMsg* RemoveNodeRespMsg_constructFromValue(int value);
static inline void RemoveNodeRespMsg_uninit(NetMessage* this);
static inline void RemoveNodeRespMsg_destruct(NetMessage* this);

// getters & setters
static inline int RemoveNodeRespMsg_getValue(RemoveNodeRespMsg* this);

struct RemoveNodeRespMsg
{
   SimpleIntMsg simpleIntMsg;
};


void RemoveNodeRespMsg_init(RemoveNodeRespMsg* this)
{
   SimpleIntMsg_init( (SimpleIntMsg*)this, NETMSGTYPE_RemoveNodeResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = RemoveNodeRespMsg_uninit;
}
   
void RemoveNodeRespMsg_initFromValue(RemoveNodeRespMsg* this, int value)
{
   SimpleIntMsg_initFromValue( (SimpleIntMsg*)this, NETMSGTYPE_RemoveNodeResp, value);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = RemoveNodeRespMsg_uninit;
}

RemoveNodeRespMsg* RemoveNodeRespMsg_construct(void)
{
   struct RemoveNodeRespMsg* this = os_kmalloc(sizeof(*this) );
   
   RemoveNodeRespMsg_init(this);
   
   return this;
}

RemoveNodeRespMsg* RemoveNodeRespMsg_constructFromValue(int value)
{
   struct RemoveNodeRespMsg* this = os_kmalloc(sizeof(*this) );
   
   RemoveNodeRespMsg_initFromValue(this, value);
   
   return this;
}

void RemoveNodeRespMsg_uninit(NetMessage* this)
{
   SimpleIntMsg_uninit( (NetMessage*)this);
}

void RemoveNodeRespMsg_destruct(NetMessage* this)
{
   RemoveNodeRespMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


int RemoveNodeRespMsg_getValue(RemoveNodeRespMsg* this)
{
   return SimpleIntMsg_getValue( (SimpleIntMsg*)this);
}

#endif /* REMOVENODERESPMSG_H_ */

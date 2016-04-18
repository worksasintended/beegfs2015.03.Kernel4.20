#ifndef  REMOVEXATTRRESPMSG_H_
#define  REMOVEXATTRRESPMSG_H_

#include <common/net/message/SimpleIntMsg.h>

struct RemoveXAttrRespMsg;
typedef struct RemoveXAttrRespMsg RemoveXAttrRespMsg;

static inline void RemoveXAttrRespMsg_init(RemoveXAttrRespMsg* this);
static inline RemoveXAttrRespMsg* RemoveXAttrRespMsg_construct(void);
static inline void RemoveXAttrRespMsg_uninit(NetMessage* this);
static inline void RemoveXAttrRespMsg_destruct(NetMessage* this);

struct RemoveXAttrRespMsg
{
   SimpleIntMsg simpleIntMsg;
};

void RemoveXAttrRespMsg_init(RemoveXAttrRespMsg* this)
{
   SimpleIntMsg_init( (SimpleIntMsg*)this, NETMSGTYPE_RemoveXAttrResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = RemoveXAttrRespMsg_uninit;
}

RemoveXAttrRespMsg* RemoveXAttrRespMsg_construct(void)
{
   struct RemoveXAttrRespMsg* this = os_kmalloc(sizeof(*this) );

   RemoveXAttrRespMsg_init(this);

   return this;
}

void RemoveXAttrRespMsg_uninit(NetMessage* this)
{
   SimpleIntMsg_uninit(this);
}

void RemoveXAttrRespMsg_destruct(NetMessage* this)
{
   RemoveXAttrRespMsg_uninit(this);
   os_kfree(this);
}

// getters & setters
static inline int RemoveXAttrRespMsg_getValue(RemoveXAttrRespMsg* this)
{
   return SimpleIntMsg_getValue( (SimpleIntMsg*)this);
}

#endif /*REMOVEXATTRRESPMSG_H_*/

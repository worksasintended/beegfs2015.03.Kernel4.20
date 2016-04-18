#ifndef SETXATTRRESPMSG_H_
#define SETXATTRRESPMSG_H_

#include <common/net/message/SimpleIntMsg.h>

struct SetXAttrRespMsg;
typedef struct SetXAttrRespMsg SetXAttrRespMsg;

static inline void SetXAttrRespMsg_init(SetXAttrRespMsg* this);
static inline SetXAttrRespMsg* SetXAttrRespMsg_construct(void);
static inline void SetXAttrRespMsg_uninit(NetMessage* this);
static inline void SetXAttrRespMsg_destruct(NetMessage* this);

struct SetXAttrRespMsg
{
   SimpleIntMsg simpleIntMsg;
};

void SetXAttrRespMsg_init(SetXAttrRespMsg* this)
{
   SimpleIntMsg_init( (SimpleIntMsg*)this, NETMSGTYPE_SetXAttrResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = SetXAttrRespMsg_uninit;
}

SetXAttrRespMsg* SetXAttrRespMsg_construct(void)
{
   struct SetXAttrRespMsg* this = os_kmalloc(sizeof(*this) );

   SetXAttrRespMsg_init(this);

   return this;
}

void SetXAttrRespMsg_uninit(NetMessage* this)
{
   SimpleIntMsg_uninit( (NetMessage*)this);
}

void SetXAttrRespMsg_destruct(NetMessage* this)
{
   SetXAttrRespMsg_uninit(this);
   os_kfree(this);
}

// getters & setters
static inline int SetXAttrRespMsg_getValue(SetXAttrRespMsg* this)
{
   return SimpleIntMsg_getValue( (SimpleIntMsg*)this);
}

#endif /*SETXATTRRESPMSG_H_*/

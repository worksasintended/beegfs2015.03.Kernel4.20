#ifndef SETATTRRESPMSG_H_
#define SETATTRRESPMSG_H_

#include <common/net/message/SimpleIntMsg.h>


struct SetAttrRespMsg;
typedef struct SetAttrRespMsg SetAttrRespMsg;

static inline void SetAttrRespMsg_init(SetAttrRespMsg* this);
static inline void SetAttrRespMsg_initFromValue(SetAttrRespMsg* this, int value);
static inline SetAttrRespMsg* SetAttrRespMsg_construct(void);
static inline SetAttrRespMsg* SetAttrRespMsg_constructFromValue(int value);
static inline void SetAttrRespMsg_uninit(NetMessage* this);
static inline void SetAttrRespMsg_destruct(NetMessage* this);

// getters & setters
static inline int SetAttrRespMsg_getValue(SetAttrRespMsg* this);

struct SetAttrRespMsg
{
   SimpleIntMsg simpleIntMsg;
};


void SetAttrRespMsg_init(SetAttrRespMsg* this)
{
   SimpleIntMsg_init( (SimpleIntMsg*)this, NETMSGTYPE_SetAttrResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = SetAttrRespMsg_uninit;
}
   
void SetAttrRespMsg_initFromValue(SetAttrRespMsg* this, int value)
{
   SimpleIntMsg_initFromValue( (SimpleIntMsg*)this, NETMSGTYPE_SetAttrResp, value);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = SetAttrRespMsg_uninit;
}

SetAttrRespMsg* SetAttrRespMsg_construct(void)
{
   struct SetAttrRespMsg* this = os_kmalloc(sizeof(struct SetAttrRespMsg) );
   
   SetAttrRespMsg_init(this);
   
   return this;
}

SetAttrRespMsg* SetAttrRespMsg_constructFromValue(int value)
{
   struct SetAttrRespMsg* this = os_kmalloc(sizeof(struct SetAttrRespMsg) );
   
   SetAttrRespMsg_initFromValue(this, value);
   
   return this;
}

void SetAttrRespMsg_uninit(NetMessage* this)
{
   SimpleIntMsg_uninit( (NetMessage*)this);
}

void SetAttrRespMsg_destruct(NetMessage* this)
{
   SetAttrRespMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


int SetAttrRespMsg_getValue(SetAttrRespMsg* this)
{
   return SimpleIntMsg_getValue( (SimpleIntMsg*)this);
}

#endif /*SETATTRRESPMSG_H_*/

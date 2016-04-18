#ifndef REFRESHSESSIONRESPMSG_H_
#define REFRESHSESSIONRESPMSG_H_

#include <common/net/message/SimpleIntMsg.h>


struct RefreshSessionRespMsg;
typedef struct RefreshSessionRespMsg RefreshSessionRespMsg;

static inline void RefreshSessionRespMsg_init(RefreshSessionRespMsg* this);
static inline void RefreshSessionRespMsg_initFromValue(RefreshSessionRespMsg* this, int value);
static inline RefreshSessionRespMsg* RefreshSessionRespMsg_construct(void);
static inline RefreshSessionRespMsg* RefreshSessionRespMsg_constructFromValue(int value);
static inline void RefreshSessionRespMsg_uninit(NetMessage* this);
static inline void RefreshSessionRespMsg_destruct(NetMessage* this);

// getters & setters
static inline int RefreshSessionRespMsg_getValue(RefreshSessionRespMsg* this);

struct RefreshSessionRespMsg
{
   SimpleIntMsg simpleIntMsg;
};


void RefreshSessionRespMsg_init(RefreshSessionRespMsg* this)
{
   SimpleIntMsg_init( (SimpleIntMsg*)this, NETMSGTYPE_RefreshSessionResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = RefreshSessionRespMsg_uninit;
}
   
void RefreshSessionRespMsg_initFromValue(RefreshSessionRespMsg* this, int value)
{
   SimpleIntMsg_initFromValue( (SimpleIntMsg*)this, NETMSGTYPE_RefreshSessionResp, value);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = RefreshSessionRespMsg_uninit;
}

RefreshSessionRespMsg* RefreshSessionRespMsg_construct()
{
   struct RefreshSessionRespMsg* this = os_kmalloc(sizeof(*this) );
   
   RefreshSessionRespMsg_init(this);
   
   return this;
}

RefreshSessionRespMsg* RefreshSessionRespMsg_constructFromValue(int value)
{
   struct RefreshSessionRespMsg* this = os_kmalloc(sizeof(*this) );
   
   RefreshSessionRespMsg_initFromValue(this, value);
   
   return this;
}

void RefreshSessionRespMsg_uninit(NetMessage* this)
{
   SimpleIntMsg_uninit( (NetMessage*)this);
}

void RefreshSessionRespMsg_destruct(NetMessage* this)
{
   RefreshSessionRespMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


int RefreshSessionRespMsg_getValue(RefreshSessionRespMsg* this)
{
   return SimpleIntMsg_getValue( (SimpleIntMsg*)this);
}


#endif /* REFRESHSESSIONRESPMSG_H_ */

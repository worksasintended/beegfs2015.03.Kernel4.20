#ifndef REFRESHENTRYINFORESPMSG_H_
#define REFRESHENTRYINFORESPMSG_H_

#include <common/net/message/SimpleIntMsg.h>


struct RefreshEntryInfoRespMsg;
typedef struct RefreshEntryInfoRespMsg RefreshEntryInfoRespMsg;

static inline void RefreshEntryInfoRespMsg_init(RefreshEntryInfoRespMsg* this);
static inline RefreshEntryInfoRespMsg* RefreshEntryInfoRespMsg_construct(void);
static inline void RefreshEntryInfoRespMsg_uninit(NetMessage* this);
static inline void RefreshEntryInfoRespMsg_destruct(NetMessage* this);

// getters & setters
static inline int RefreshEntryInfoRespMsg_getResult(RefreshEntryInfoRespMsg* this);

struct RefreshEntryInfoRespMsg
{
   SimpleIntMsg simpleIntMsg;
};


void RefreshEntryInfoRespMsg_init(RefreshEntryInfoRespMsg* this)
{
   SimpleIntMsg_init( (SimpleIntMsg*)this, NETMSGTYPE_RefreshEntryInfoResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = RefreshEntryInfoRespMsg_uninit;
}

RefreshEntryInfoRespMsg* RefreshEntryInfoRespMsg_construct(void)
{
   struct RefreshEntryInfoRespMsg* this = os_kmalloc(sizeof(struct RefreshEntryInfoRespMsg) );

   RefreshEntryInfoRespMsg_init(this);

   return this;
}


void RefreshEntryInfoRespMsg_uninit(NetMessage* this)
{
   SimpleIntMsg_uninit( (NetMessage*)this);
}

void RefreshEntryInfoRespMsg_destruct(NetMessage* this)
{
   RefreshEntryInfoRespMsg_uninit( (NetMessage*)this);

   os_kfree(this);
}


int RefreshEntryInfoRespMsg_getResult(RefreshEntryInfoRespMsg* this)
{
   return SimpleIntMsg_getValue( (SimpleIntMsg*)this);
}

#endif /*REFRESHENTRYINFORESPMSG_H_*/

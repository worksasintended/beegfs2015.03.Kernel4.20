#ifndef HARDLINKRESPMSG_H_
#define HARDLINKRESPMSG_H_

#include <common/net/message/SimpleIntMsg.h>


struct HardlinkRespMsg;
typedef struct HardlinkRespMsg HardlinkRespMsg;

static inline void HardlinkRespMsg_init(HardlinkRespMsg* this);
static inline void HardlinkRespMsg_initFromValue(HardlinkRespMsg* this, int value);
static inline HardlinkRespMsg* HardlinkRespMsg_construct(void);
static inline HardlinkRespMsg* HardlinkRespMsg_constructFromValue(int value);
static inline void HardlinkRespMsg_uninit(NetMessage* this);
static inline void HardlinkRespMsg_destruct(NetMessage* this);

// getters & setters
static inline int HardlinkRespMsg_getValue(HardlinkRespMsg* this);

struct HardlinkRespMsg
{
   SimpleIntMsg simpleIntMsg;
};


void HardlinkRespMsg_init(HardlinkRespMsg* this)
{
   SimpleIntMsg_init( (SimpleIntMsg*)this, NETMSGTYPE_HardlinkResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = HardlinkRespMsg_uninit;
}
   
void HardlinkRespMsg_initFromValue(HardlinkRespMsg* this, int value)
{
   SimpleIntMsg_initFromValue( (SimpleIntMsg*)this, NETMSGTYPE_HardlinkResp, value);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = HardlinkRespMsg_uninit;
}

HardlinkRespMsg* HardlinkRespMsg_construct(void)
{
   struct HardlinkRespMsg* this = os_kmalloc(sizeof(struct HardlinkRespMsg) );
   
   HardlinkRespMsg_init(this);
   
   return this;
}

HardlinkRespMsg* HardlinkRespMsg_constructFromValue(int value)
{
   struct HardlinkRespMsg* this = os_kmalloc(sizeof(struct HardlinkRespMsg) );
   
   HardlinkRespMsg_initFromValue(this, value);
   
   return this;
}

void HardlinkRespMsg_uninit(NetMessage* this)
{
   SimpleIntMsg_uninit( (NetMessage*)this);
}

void HardlinkRespMsg_destruct(NetMessage* this)
{
   HardlinkRespMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


int HardlinkRespMsg_getValue(HardlinkRespMsg* this)
{
   return SimpleIntMsg_getValue( (SimpleIntMsg*)this);
}


#endif /*HARDLINKRESPMSG_H_*/

#ifndef RMDIRRESPMSG_H_
#define RMDIRRESPMSG_H_

#include <common/net/message/SimpleIntMsg.h>


struct RmDirRespMsg;
typedef struct RmDirRespMsg RmDirRespMsg;

static inline void RmDirRespMsg_init(RmDirRespMsg* this);
static inline void RmDirRespMsg_initFromValue(RmDirRespMsg* this, int value);
static inline RmDirRespMsg* RmDirRespMsg_construct(void);
static inline RmDirRespMsg* RmDirRespMsg_constructFromValue(int value);
static inline void RmDirRespMsg_uninit(NetMessage* this);
static inline void RmDirRespMsg_destruct(NetMessage* this);

// getters & setters
static inline int RmDirRespMsg_getValue(RmDirRespMsg* this);

struct RmDirRespMsg
{
   SimpleIntMsg simpleIntMsg;
};


void RmDirRespMsg_init(RmDirRespMsg* this)
{
   SimpleIntMsg_init( (SimpleIntMsg*)this, NETMSGTYPE_RmDirResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = RmDirRespMsg_uninit;
}
   
void RmDirRespMsg_initFromValue(RmDirRespMsg* this, int value)
{
   SimpleIntMsg_initFromValue( (SimpleIntMsg*)this, NETMSGTYPE_RmDirResp, value);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = RmDirRespMsg_uninit;
}

RmDirRespMsg* RmDirRespMsg_construct(void)
{
   struct RmDirRespMsg* this = os_kmalloc(sizeof(struct RmDirRespMsg) );
   
   RmDirRespMsg_init(this);
   
   return this;
}

RmDirRespMsg* RmDirRespMsg_constructFromValue(int value)
{
   struct RmDirRespMsg* this = os_kmalloc(sizeof(struct RmDirRespMsg) );
   
   RmDirRespMsg_initFromValue(this, value);
   
   return this;
}

void RmDirRespMsg_uninit(NetMessage* this)
{
   SimpleIntMsg_uninit( (NetMessage*)this);
}

void RmDirRespMsg_destruct(NetMessage* this)
{
   RmDirRespMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


int RmDirRespMsg_getValue(RmDirRespMsg* this)
{
   return SimpleIntMsg_getValue( (SimpleIntMsg*)this);
}



#endif /*RMDIRRESPMSG_H_*/

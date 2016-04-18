#ifndef RENAMERESPMSG_H_
#define RENAMERESPMSG_H_

#include <common/net/message/SimpleIntMsg.h>


struct RenameRespMsg;
typedef struct RenameRespMsg RenameRespMsg;

static inline void RenameRespMsg_init(RenameRespMsg* this);
static inline void RenameRespMsg_initFromValue(RenameRespMsg* this, int value);
static inline RenameRespMsg* RenameRespMsg_construct(void);
static inline RenameRespMsg* RenameRespMsg_constructFromValue(int value);
static inline void RenameRespMsg_uninit(NetMessage* this);
static inline void RenameRespMsg_destruct(NetMessage* this);

// getters & setters
static inline int RenameRespMsg_getValue(RenameRespMsg* this);

struct RenameRespMsg
{
   SimpleIntMsg simpleIntMsg;
};


void RenameRespMsg_init(RenameRespMsg* this)
{
   SimpleIntMsg_init( (SimpleIntMsg*)this, NETMSGTYPE_RenameResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = RenameRespMsg_uninit;
}
   
void RenameRespMsg_initFromValue(RenameRespMsg* this, int value)
{
   SimpleIntMsg_initFromValue( (SimpleIntMsg*)this, NETMSGTYPE_RenameResp, value);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = RenameRespMsg_uninit;
}

RenameRespMsg* RenameRespMsg_construct(void)
{
   struct RenameRespMsg* this = os_kmalloc(sizeof(struct RenameRespMsg) );
   
   RenameRespMsg_init(this);
   
   return this;
}

RenameRespMsg* RenameRespMsg_constructFromValue(int value)
{
   struct RenameRespMsg* this = os_kmalloc(sizeof(struct RenameRespMsg) );
   
   RenameRespMsg_initFromValue(this, value);
   
   return this;
}

void RenameRespMsg_uninit(NetMessage* this)
{
   SimpleIntMsg_uninit( (NetMessage*)this);
}

void RenameRespMsg_destruct(NetMessage* this)
{
   RenameRespMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


int RenameRespMsg_getValue(RenameRespMsg* this)
{
   return SimpleIntMsg_getValue( (SimpleIntMsg*)this);
}


#endif /*RENAMERESPMSG_H_*/

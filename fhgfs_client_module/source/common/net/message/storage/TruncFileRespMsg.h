#ifndef TRUNCFILERESPMSG_H_
#define TRUNCFILERESPMSG_H_

#include <common/net/message/SimpleIntMsg.h>


struct TruncFileRespMsg;
typedef struct TruncFileRespMsg TruncFileRespMsg;

static inline void TruncFileRespMsg_init(TruncFileRespMsg* this);
static inline void TruncFileRespMsg_initFromValue(TruncFileRespMsg* this, int value);
static inline TruncFileRespMsg* TruncFileRespMsg_construct(void);
static inline TruncFileRespMsg* TruncFileRespMsg_constructFromValue(int value);
static inline void TruncFileRespMsg_uninit(NetMessage* this);
static inline void TruncFileRespMsg_destruct(NetMessage* this);

// getters & setters
static inline int TruncFileRespMsg_getValue(TruncFileRespMsg* this);

struct TruncFileRespMsg
{
   SimpleIntMsg simpleIntMsg;
};


void TruncFileRespMsg_init(TruncFileRespMsg* this)
{
   SimpleIntMsg_init( (SimpleIntMsg*)this, NETMSGTYPE_TruncFileResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = TruncFileRespMsg_uninit;
}
   
void TruncFileRespMsg_initFromValue(TruncFileRespMsg* this, int value)
{
   SimpleIntMsg_initFromValue( (SimpleIntMsg*)this, NETMSGTYPE_TruncFileResp, value);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = TruncFileRespMsg_uninit;
}

TruncFileRespMsg* TruncFileRespMsg_construct(void)
{
   struct TruncFileRespMsg* this = os_kmalloc(sizeof(struct TruncFileRespMsg) );
   
   TruncFileRespMsg_init(this);
   
   return this;
}

TruncFileRespMsg* TruncFileRespMsg_constructFromValue(int value)
{
   struct TruncFileRespMsg* this = os_kmalloc(sizeof(struct TruncFileRespMsg) );
   
   TruncFileRespMsg_initFromValue(this, value);
   
   return this;
}

void TruncFileRespMsg_uninit(NetMessage* this)
{
   SimpleIntMsg_uninit( (NetMessage*)this);
}

void TruncFileRespMsg_destruct(NetMessage* this)
{
   TruncFileRespMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


int TruncFileRespMsg_getValue(TruncFileRespMsg* this)
{
   return SimpleIntMsg_getValue( (SimpleIntMsg*)this);
}



#endif /*TRUNCFILERESPMSG_H_*/

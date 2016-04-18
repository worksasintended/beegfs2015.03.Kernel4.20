#ifndef MKLOCALFILERESPMSG_H_
#define MKLOCALFILERESPMSG_H_

#include <common/net/message/SimpleIntMsg.h>


struct MkLocalFileRespMsg;
typedef struct MkLocalFileRespMsg MkLocalFileRespMsg;

static inline void MkLocalFileRespMsg_init(MkLocalFileRespMsg* this);
static inline void MkLocalFileRespMsg_initFromValue(MkLocalFileRespMsg* this, int value);
static inline MkLocalFileRespMsg* MkLocalFileRespMsg_construct(void);
static inline MkLocalFileRespMsg* MkLocalFileRespMsg_constructFromValue(int value);
static inline void MkLocalFileRespMsg_uninit(NetMessage* this);
static inline void MkLocalFileRespMsg_destruct(NetMessage* this);

// getters & setters
static inline int MkLocalFileRespMsg_getValue(MkLocalFileRespMsg* this);


struct MkLocalFileRespMsg
{
   SimpleIntMsg simpleIntMsg;
};


void MkLocalFileRespMsg_init(MkLocalFileRespMsg* this)
{
   SimpleIntMsg_init( (SimpleIntMsg*)this, NETMSGTYPE_MkLocalFileResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = MkLocalFileRespMsg_uninit;
}
   
void MkLocalFileRespMsg_initFromValue(MkLocalFileRespMsg* this, int value)
{
   SimpleIntMsg_initFromValue( (SimpleIntMsg*)this, NETMSGTYPE_MkLocalFileResp, value);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = MkLocalFileRespMsg_uninit;
}

MkLocalFileRespMsg* MkLocalFileRespMsg_construct(void)
{
   struct MkLocalFileRespMsg* this = os_kmalloc(sizeof(struct MkLocalFileRespMsg) );
   
   MkLocalFileRespMsg_init(this);
   
   return this;
}

MkLocalFileRespMsg* MkLocalFileRespMsg_constructFromValue(int value)
{
   struct MkLocalFileRespMsg* this = os_kmalloc(sizeof(struct MkLocalFileRespMsg) );
   
   MkLocalFileRespMsg_initFromValue(this, value);
   
   return this;
}

void MkLocalFileRespMsg_uninit(NetMessage* this)
{
   SimpleIntMsg_uninit( (NetMessage*)this);
}

void MkLocalFileRespMsg_destruct(NetMessage* this)
{
   MkLocalFileRespMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


int MkLocalFileRespMsg_getValue(MkLocalFileRespMsg* this)
{
   return SimpleIntMsg_getValue( (SimpleIntMsg*)this);
}



#endif /*MKLOCALFILERESPMSG_H_*/

#ifndef CLOSEFILERESPMSG_H_
#define CLOSEFILERESPMSG_H_

#include <common/net/message/SimpleIntMsg.h>


struct CloseFileRespMsg;
typedef struct CloseFileRespMsg CloseFileRespMsg;

static inline void CloseFileRespMsg_init(CloseFileRespMsg* this);
static inline void CloseFileRespMsg_initFromValue(CloseFileRespMsg* this, int value);
static inline CloseFileRespMsg* CloseFileRespMsg_construct(void);
static inline CloseFileRespMsg* CloseFileRespMsg_constructFromValue(int value);
static inline void CloseFileRespMsg_uninit(NetMessage* this);
static inline void CloseFileRespMsg_destruct(NetMessage* this);

// getters & setters
static inline int CloseFileRespMsg_getValue(CloseFileRespMsg* this);

struct CloseFileRespMsg
{
   SimpleIntMsg simpleIntMsg;
};


void CloseFileRespMsg_init(CloseFileRespMsg* this)
{
   SimpleIntMsg_init( (SimpleIntMsg*)this, NETMSGTYPE_CloseFileResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = CloseFileRespMsg_uninit;
}
   
void CloseFileRespMsg_initFromValue(CloseFileRespMsg* this, int value)
{
   SimpleIntMsg_initFromValue( (SimpleIntMsg*)this, NETMSGTYPE_CloseFileResp, value);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = CloseFileRespMsg_uninit;
}

CloseFileRespMsg* CloseFileRespMsg_construct(void)
{
   struct CloseFileRespMsg* this = os_kmalloc(sizeof(struct CloseFileRespMsg) );
   
   CloseFileRespMsg_init(this);
   
   return this;
}

CloseFileRespMsg* CloseFileRespMsg_constructFromValue(int value)
{
   struct CloseFileRespMsg* this = os_kmalloc(sizeof(struct CloseFileRespMsg) );
   
   CloseFileRespMsg_initFromValue(this, value);
   
   return this;
}

void CloseFileRespMsg_uninit(NetMessage* this)
{
   SimpleIntMsg_uninit( (NetMessage*)this);
}

void CloseFileRespMsg_destruct(NetMessage* this)
{
   CloseFileRespMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


int CloseFileRespMsg_getValue(CloseFileRespMsg* this)
{
   return SimpleIntMsg_getValue( (SimpleIntMsg*)this);
}


#endif /*CLOSEFILERESPMSG_H_*/

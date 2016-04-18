#ifndef FSYNCLOCALFILERESPMSG_H_
#define FSYNCLOCALFILERESPMSG_H_

#include "../SimpleInt64Msg.h"


struct FSyncLocalFileRespMsg;
typedef struct FSyncLocalFileRespMsg FSyncLocalFileRespMsg;

static inline void FSyncLocalFileRespMsg_init(FSyncLocalFileRespMsg* this);
static inline void FSyncLocalFileRespMsg_initFromValue(FSyncLocalFileRespMsg* this,
   int64_t value);
static inline FSyncLocalFileRespMsg* FSyncLocalFileRespMsg_construct(void);
static inline FSyncLocalFileRespMsg* FSyncLocalFileRespMsg_constructFromValue(
   int64_t value);
static inline void FSyncLocalFileRespMsg_uninit(NetMessage* this);
static inline void FSyncLocalFileRespMsg_destruct(NetMessage* this);

// getters & setters
static inline int64_t FSyncLocalFileRespMsg_getValue(FSyncLocalFileRespMsg* this);


struct FSyncLocalFileRespMsg
{
   SimpleInt64Msg simpleInt64Msg;
};


void FSyncLocalFileRespMsg_init(FSyncLocalFileRespMsg* this)
{
   SimpleInt64Msg_init( (SimpleInt64Msg*)this, NETMSGTYPE_FSyncLocalFileResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = FSyncLocalFileRespMsg_uninit;
}
   
void FSyncLocalFileRespMsg_initFromValue(FSyncLocalFileRespMsg* this, int64_t value)
{
   SimpleInt64Msg_initFromValue( (SimpleInt64Msg*)this, NETMSGTYPE_FSyncLocalFileResp, value);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = FSyncLocalFileRespMsg_uninit;
}

FSyncLocalFileRespMsg* FSyncLocalFileRespMsg_construct(void)
{
   struct FSyncLocalFileRespMsg* this = os_kmalloc(sizeof(*this) );
   
   FSyncLocalFileRespMsg_init(this);
   
   return this;
}

FSyncLocalFileRespMsg* FSyncLocalFileRespMsg_constructFromValue(int64_t value)
{
   struct FSyncLocalFileRespMsg* this = os_kmalloc(sizeof(*this) );
   
   FSyncLocalFileRespMsg_initFromValue(this, value);
   
   return this;
}

void FSyncLocalFileRespMsg_uninit(NetMessage* this)
{
   SimpleInt64Msg_uninit( (NetMessage*)this);
}

void FSyncLocalFileRespMsg_destruct(NetMessage* this)
{
   FSyncLocalFileRespMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


int64_t FSyncLocalFileRespMsg_getValue(FSyncLocalFileRespMsg* this)
{
   return SimpleInt64Msg_getValue( (SimpleInt64Msg*)this);
}


#endif /*FSYNCLOCALFILERESPMSG_H_*/

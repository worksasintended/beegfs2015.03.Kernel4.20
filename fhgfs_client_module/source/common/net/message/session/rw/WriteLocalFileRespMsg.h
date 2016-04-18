#ifndef WRITELOCALFILERESPMSG_H_
#define WRITELOCALFILERESPMSG_H_

#include <common/net/message/SimpleInt64Msg.h>


struct WriteLocalFileRespMsg;
typedef struct WriteLocalFileRespMsg WriteLocalFileRespMsg;

static inline void WriteLocalFileRespMsg_init(WriteLocalFileRespMsg* this);
static inline void WriteLocalFileRespMsg_initFromValue(WriteLocalFileRespMsg* this,
   int64_t value);
static inline WriteLocalFileRespMsg* WriteLocalFileRespMsg_construct(void);
static inline WriteLocalFileRespMsg* WriteLocalFileRespMsg_constructFromValue(
   int64_t value);
static inline void WriteLocalFileRespMsg_uninit(NetMessage* this);
static inline void WriteLocalFileRespMsg_destruct(NetMessage* this);

// getters & setters
static inline int64_t WriteLocalFileRespMsg_getValue(WriteLocalFileRespMsg* this);

struct WriteLocalFileRespMsg
{
   SimpleInt64Msg simpleInt64Msg;
};


void WriteLocalFileRespMsg_init(WriteLocalFileRespMsg* this)
{
   SimpleInt64Msg_init( (SimpleInt64Msg*)this, NETMSGTYPE_WriteLocalFileResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = WriteLocalFileRespMsg_uninit;
}
   
void WriteLocalFileRespMsg_initFromValue(WriteLocalFileRespMsg* this, int64_t value)
{
   SimpleInt64Msg_initFromValue( (SimpleInt64Msg*)this, NETMSGTYPE_WriteLocalFileResp, value);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = WriteLocalFileRespMsg_uninit;
}

WriteLocalFileRespMsg* WriteLocalFileRespMsg_construct(void)
{
   struct WriteLocalFileRespMsg* this = os_kmalloc(sizeof(struct WriteLocalFileRespMsg) );
   
   WriteLocalFileRespMsg_init(this);
   
   return this;
}

WriteLocalFileRespMsg* WriteLocalFileRespMsg_constructFromValue(int64_t value)
{
   struct WriteLocalFileRespMsg* this = os_kmalloc(sizeof(struct WriteLocalFileRespMsg) );
   
   WriteLocalFileRespMsg_initFromValue(this, value);
   
   return this;
}

void WriteLocalFileRespMsg_uninit(NetMessage* this)
{
   SimpleInt64Msg_uninit( (NetMessage*)this);
}

void WriteLocalFileRespMsg_destruct(NetMessage* this)
{
   WriteLocalFileRespMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


int64_t WriteLocalFileRespMsg_getValue(WriteLocalFileRespMsg* this)
{
   return SimpleInt64Msg_getValue( (SimpleInt64Msg*)this);
}


#endif /*WRITELOCALFILERESPMSG_H_*/

#ifndef SIMPLEINT64MSG_H_
#define SIMPLEINT64MSG_H_

#include "NetMessage.h"

struct SimpleInt64Msg;
typedef struct SimpleInt64Msg SimpleInt64Msg;

static inline void SimpleInt64Msg_init(SimpleInt64Msg* this, unsigned short msgType);
static inline void SimpleInt64Msg_initFromValue(SimpleInt64Msg* this, unsigned short msgType,
   int64_t value);
static inline SimpleInt64Msg* SimpleInt64Msg_construct(unsigned short msgType);
static inline SimpleInt64Msg* SimpleInt64Msg_constructFromValue(unsigned short msgType,
   int64_t value);
static inline void SimpleInt64Msg_uninit(NetMessage* this);
static inline void SimpleInt64Msg_destruct(NetMessage* this);

// virtual functions
extern void SimpleInt64Msg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool SimpleInt64Msg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen);
extern unsigned SimpleInt64Msg_calcMessageLength(NetMessage* this);

// getters & setters
static inline int64_t SimpleInt64Msg_getValue(SimpleInt64Msg* this);

struct SimpleInt64Msg
{
   NetMessage netMessage;
   
   int64_t value;
};


void SimpleInt64Msg_init(SimpleInt64Msg* this, unsigned short msgType)
{
   NetMessage_init( (NetMessage*)this, msgType);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = SimpleInt64Msg_uninit;

   ( (NetMessage*)this)->serializePayload = SimpleInt64Msg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = SimpleInt64Msg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = SimpleInt64Msg_calcMessageLength;
}
   
void SimpleInt64Msg_initFromValue(SimpleInt64Msg* this, unsigned short msgType, int64_t value)
{
   SimpleInt64Msg_init(this, msgType);
   
   this->value = value;
}

SimpleInt64Msg* SimpleInt64Msg_construct(unsigned short msgType)
{
   struct SimpleInt64Msg* this = os_kmalloc(sizeof(struct SimpleInt64Msg) );
   
   SimpleInt64Msg_init(this, msgType);
   
   return this;
}

SimpleInt64Msg* SimpleInt64Msg_constructFromValue(unsigned short msgType, int64_t value)
{
   struct SimpleInt64Msg* this = os_kmalloc(sizeof(*this) );
   
   SimpleInt64Msg_initFromValue(this, msgType, value);
   
   return this;
}

void SimpleInt64Msg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void SimpleInt64Msg_destruct(NetMessage* this)
{
   SimpleInt64Msg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


int64_t SimpleInt64Msg_getValue(SimpleInt64Msg* this)
{
   return this->value;
}

#endif /*SIMPLEINT64MSG_H_*/

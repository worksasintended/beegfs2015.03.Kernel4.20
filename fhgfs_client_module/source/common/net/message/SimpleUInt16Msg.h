#ifndef SIMPLEUINT16MSG_H_
#define SIMPLEUINT16MSG_H_

#include "NetMessage.h"

struct SimpleUInt16Msg;
typedef struct SimpleUInt16Msg SimpleUInt16Msg;

static inline void SimpleUInt16Msg_init(SimpleUInt16Msg* this, unsigned short msgType);
static inline void SimpleUInt16Msg_initFromValue(SimpleUInt16Msg* this, unsigned short msgType,
   uint16_t value);
static inline SimpleUInt16Msg* SimpleUInt16Msg_construct(unsigned short msgType);
static inline SimpleUInt16Msg* SimpleUInt16Msg_constructFromValue(unsigned short msgType,
   uint16_t value);
static inline void SimpleUInt16Msg_uninit(NetMessage* this);
static inline void SimpleUInt16Msg_destruct(NetMessage* this);

// virtual functions
extern void SimpleUInt16Msg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool SimpleUInt16Msg_deserializePayload(NetMessage* this, const char* buf,
   size_t bufLen);
extern unsigned SimpleUInt16Msg_calcMessageLength(NetMessage* this);

// getters & setters
static inline uint16_t SimpleUInt16Msg_getValue(SimpleUInt16Msg* this);


struct SimpleUInt16Msg
{
   NetMessage netMessage;

   uint16_t value;
};


void SimpleUInt16Msg_init(SimpleUInt16Msg* this, unsigned short msgType)
{
   NetMessage_init( (NetMessage*)this, msgType);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = SimpleUInt16Msg_uninit;

   ( (NetMessage*)this)->serializePayload = SimpleUInt16Msg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = SimpleUInt16Msg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = SimpleUInt16Msg_calcMessageLength;
}

void SimpleUInt16Msg_initFromValue(SimpleUInt16Msg* this, unsigned short msgType, uint16_t value)
{
   SimpleUInt16Msg_init(this, msgType);

   this->value = value;
}

SimpleUInt16Msg* SimpleUInt16Msg_construct(unsigned short msgType)
{
   struct SimpleUInt16Msg* this = os_kmalloc(sizeof(*this) );

   SimpleUInt16Msg_init(this, msgType);

   return this;
}

SimpleUInt16Msg* SimpleUInt16Msg_constructFromValue(unsigned short msgType, uint16_t value)
{
   struct SimpleUInt16Msg* this = os_kmalloc(sizeof(*this) );

   SimpleUInt16Msg_initFromValue(this, msgType, value);

   return this;
}

void SimpleUInt16Msg_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void SimpleUInt16Msg_destruct(NetMessage* this)
{
   SimpleUInt16Msg_uninit(this);

   os_kfree(this);
}


uint16_t SimpleUInt16Msg_getValue(SimpleUInt16Msg* this)
{
   return this->value;
}


#endif /* SIMPLEUINT16MSG_H_ */

#ifndef SIMPLEINTMSG_H_
#define SIMPLEINTMSG_H_

#include "NetMessage.h"

struct SimpleIntMsg;
typedef struct SimpleIntMsg SimpleIntMsg;

static inline void SimpleIntMsg_init(SimpleIntMsg* this, unsigned short msgType);
static inline void SimpleIntMsg_initFromValue(SimpleIntMsg* this, unsigned short msgType,
   int value);
static inline SimpleIntMsg* SimpleIntMsg_construct(unsigned short msgType);
static inline SimpleIntMsg* SimpleIntMsg_constructFromValue(unsigned short msgType,
   int value);
static inline void SimpleIntMsg_uninit(NetMessage* this);
static inline void SimpleIntMsg_destruct(NetMessage* this);

// virtual functions
extern void SimpleIntMsg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool SimpleIntMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen);
extern unsigned SimpleIntMsg_calcMessageLength(NetMessage* this);

// getters & setters
static inline int SimpleIntMsg_getValue(SimpleIntMsg* this);


struct SimpleIntMsg
{
   NetMessage netMessage;
   
   int value;
};


void SimpleIntMsg_init(SimpleIntMsg* this, unsigned short msgType)
{
   NetMessage_init( (NetMessage*)this, msgType);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = SimpleIntMsg_uninit;

   ( (NetMessage*)this)->serializePayload = SimpleIntMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = SimpleIntMsg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = SimpleIntMsg_calcMessageLength;
}
   
void SimpleIntMsg_initFromValue(SimpleIntMsg* this, unsigned short msgType, int value)
{
   SimpleIntMsg_init(this, msgType);
   
   this->value = value;
}

SimpleIntMsg* SimpleIntMsg_construct(unsigned short msgType)
{
   struct SimpleIntMsg* this = os_kmalloc(sizeof(*this) );
   
   SimpleIntMsg_init(this, msgType);
   
   return this;
}

SimpleIntMsg* SimpleIntMsg_constructFromValue(unsigned short msgType, int value)
{
   struct SimpleIntMsg* this = os_kmalloc(sizeof(*this) );
   
   SimpleIntMsg_initFromValue(this, msgType, value);
   
   return this;
}

void SimpleIntMsg_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void SimpleIntMsg_destruct(NetMessage* this)
{
   SimpleIntMsg_uninit(this);
   
   os_kfree(this);
}


int SimpleIntMsg_getValue(SimpleIntMsg* this)
{
   return this->value;
}


#endif /*SIMPLEINTMSG_H_*/

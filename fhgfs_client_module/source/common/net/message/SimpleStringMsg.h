#ifndef SIMPLESTRINGMSG_H_
#define SIMPLESTRINGMSG_H_

#include "NetMessage.h"

struct SimpleStringMsg;
typedef struct SimpleStringMsg SimpleStringMsg;

static inline void SimpleStringMsg_init(SimpleStringMsg* this, unsigned short msgType);
static inline void SimpleStringMsg_initFromValue(SimpleStringMsg* this, unsigned short msgType,
   const char* value);
static inline SimpleStringMsg* SimpleStringMsg_construct(unsigned short msgType);
static inline SimpleStringMsg* SimpleStringMsg_constructFromValue(unsigned short msgType,
   const char* value);
static inline void SimpleStringMsg_uninit(NetMessage* this);
static inline void SimpleStringMsg_destruct(NetMessage* this);

// virtual functions
extern void SimpleStringMsg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool SimpleStringMsg_deserializePayload(NetMessage* this, const char* buf,
   size_t bufLen);
extern unsigned SimpleStringMsg_calcMessageLength(NetMessage* this);

// getters & setters
static inline const char* SimpleStringMsg_getValue(SimpleStringMsg* this);

struct SimpleStringMsg
{
   NetMessage netMessage;
   
   const char* value;
   unsigned valueLen;
};


void SimpleStringMsg_init(SimpleStringMsg* this, unsigned short msgType)
{
   NetMessage_init( (NetMessage*)this, msgType);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = SimpleStringMsg_uninit;

   ( (NetMessage*)this)->serializePayload = SimpleStringMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = SimpleStringMsg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = SimpleStringMsg_calcMessageLength;
}
   
void SimpleStringMsg_initFromValue(SimpleStringMsg* this, unsigned short msgType, const char* value)
{
   SimpleStringMsg_init(this, msgType);
   
   this->value = value;
   this->valueLen = os_strlen(value);
}

SimpleStringMsg* SimpleStringMsg_construct(unsigned short msgType)
{
   struct SimpleStringMsg* this = os_kmalloc(sizeof(*this) );
   
   SimpleStringMsg_init(this, msgType);
   
   return this;
}

SimpleStringMsg* SimpleStringMsg_constructFromValue(unsigned short msgType, const char* value)
{
   struct SimpleStringMsg* this = os_kmalloc(sizeof(*this) );
   
   SimpleStringMsg_initFromValue(this, msgType, value);
   
   return this;
}

void SimpleStringMsg_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void SimpleStringMsg_destruct(NetMessage* this)
{
   SimpleStringMsg_uninit(this);
   
   os_kfree(this);
}


const char* SimpleStringMsg_getValue(SimpleStringMsg* this)
{
   return this->value;
}

#endif /* SIMPLESTRINGMSG_H_ */

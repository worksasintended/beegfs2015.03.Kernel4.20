#ifndef SIMPLEINTSTRINGMSG_H_
#define SIMPLEINTSTRINGMSG_H_

#include "NetMessage.h"

struct SimpleIntStringMsg;
typedef struct SimpleIntStringMsg SimpleIntStringMsg;

static inline void SimpleIntStringMsg_init(SimpleIntStringMsg* this, unsigned short msgType);
static inline void SimpleIntStringMsg_initFromValue(SimpleIntStringMsg* this, unsigned short msgType,
   int intValue, const char* strValue);
static inline SimpleIntStringMsg* SimpleIntStringMsg_construct(unsigned short msgType);
static inline SimpleIntStringMsg* SimpleIntStringMsg_constructFromValue(unsigned short msgType,
   int intValue, const char* strValue);
static inline void SimpleIntStringMsg_uninit(NetMessage* this);
static inline void SimpleIntStringMsg_destruct(NetMessage* this);

// virtual functions
extern void SimpleIntStringMsg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool SimpleIntStringMsg_deserializePayload(NetMessage* this, const char* buf,
   size_t bufLen);
extern unsigned SimpleIntStringMsg_calcMessageLength(NetMessage* this);

// getters & setters
static inline int SimpleIntStringMsg_getIntValue(SimpleIntStringMsg* this);
static inline const char* SimpleIntStringMsg_getStrValue(SimpleIntStringMsg* this);


/**
 * Simple message containing an integer value and a string (e.g. int error code and human-readable
 * explantion with more details as string).
 */
struct SimpleIntStringMsg
{
   NetMessage netMessage;

   int intValue;

   const char* strValue;
   unsigned strValueLen;
};


void SimpleIntStringMsg_init(SimpleIntStringMsg* this, unsigned short msgType)
{
   NetMessage_init( (NetMessage*)this, msgType);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = SimpleIntStringMsg_uninit;

   ( (NetMessage*)this)->serializePayload = SimpleIntStringMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = SimpleIntStringMsg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = SimpleIntStringMsg_calcMessageLength;
}

/**
 * @param strValue just a reference, so don't free or modify it while this msg is used.
 */
void SimpleIntStringMsg_initFromValue(SimpleIntStringMsg* this, unsigned short msgType,
   int intValue, const char* strValue)
{
   SimpleIntStringMsg_init(this, msgType);

   this->intValue = intValue;

   this->strValue = strValue;
   this->strValueLen = os_strlen(strValue);
}

SimpleIntStringMsg* SimpleIntStringMsg_construct(unsigned short msgType)
{
   struct SimpleIntStringMsg* this = os_kmalloc(sizeof(*this) );

   if(likely(this) )
      SimpleIntStringMsg_init(this, msgType);

   return this;
}

/**
 * @param strValue just a reference, so don't free or modify it while this msg is used.
 */
SimpleIntStringMsg* SimpleIntStringMsg_constructFromValue(unsigned short msgType, int intValue,
   const char* strValue)
{
   struct SimpleIntStringMsg* this = os_kmalloc(sizeof(*this) );

   if(likely(this) )
      SimpleIntStringMsg_initFromValue(this, msgType, intValue, strValue);

   return this;
}

void SimpleIntStringMsg_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void SimpleIntStringMsg_destruct(NetMessage* this)
{
   SimpleIntStringMsg_uninit(this);

   os_kfree(this);
}


int SimpleIntStringMsg_getIntValue(SimpleIntStringMsg* this)
{
   return this->intValue;
}

const char* SimpleIntStringMsg_getStrValue(SimpleIntStringMsg* this)
{
   return this->strValue;
}



#endif /* SIMPLEINTSTRINGMSG_H_ */

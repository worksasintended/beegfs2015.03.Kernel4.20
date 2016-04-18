#ifndef SIMPLEMSG_H_
#define SIMPLEMSG_H_

#include "NetMessage.h"

/**
 * Note: Simple messages are defined by the header (resp. the msgType) only and
 * require no additional data
 */

struct SimpleMsg;
typedef struct SimpleMsg SimpleMsg;

static inline void SimpleMsg_init(SimpleMsg* this, unsigned short msgType);
static inline SimpleMsg* SimpleMsg_construct(unsigned short msgType);
static inline void SimpleMsg_uninit(NetMessage* this);
static inline void SimpleMsg_destruct(NetMessage* this);

// virtual functions
extern void SimpleMsg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool SimpleMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen);
extern unsigned SimpleMsg_calcMessageLength(NetMessage* this);


struct SimpleMsg
{
   NetMessage netMessage;
};


void SimpleMsg_init(SimpleMsg* this, unsigned short msgType)
{
   NetMessage_init( (NetMessage*)this, msgType);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = SimpleMsg_uninit;

   ( (NetMessage*)this)->serializePayload = SimpleMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = SimpleMsg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = SimpleMsg_calcMessageLength;
}
   
SimpleMsg* SimpleMsg_construct(unsigned short msgType)
{
   struct SimpleMsg* this = os_kmalloc(sizeof(*this) );
   
   SimpleMsg_init(this, msgType);
   
   return this;
}

void SimpleMsg_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void SimpleMsg_destruct(NetMessage* this)
{
   SimpleMsg_uninit(this);
   
   os_kfree(this);
}


#endif /*SIMPLEMSG_H_*/

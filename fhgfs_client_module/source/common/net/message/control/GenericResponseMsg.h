#ifndef GENERICRESPONSEMSG_H_
#define GENERICRESPONSEMSG_H_

#include <common/net/message/SimpleIntStringMsg.h>


/**
 * Note: Remember to keep this in sync with userspace common lib.
 */
enum GenericRespMsgCode
{
   GenericRespMsgCode_TRYAGAIN          =  0, /* requestor shall try again in a few seconds */
   GenericRespMsgCode_INDIRECTCOMMERR   =  1, /* indirect communication error and requestor should
      try again (e.g. msg forwarding to other server failed) */
   GenericRespMsgCode_INDIRECTCOMMERR_NOTAGAIN =  2, /* indirect communication error, and requestor
      shall not try again (e.g. msg forwarding to other server failed and the other server is marked
      offline) */
};
typedef enum GenericRespMsgCode GenericRespMsgCode;


struct GenericResponseMsg;
typedef struct GenericResponseMsg GenericResponseMsg;


static inline void GenericResponseMsg_init(GenericResponseMsg* this);
static inline void GenericResponseMsg_initFromValue(GenericResponseMsg* this,
   GenericRespMsgCode controlCode, const char* logStr);
static inline GenericResponseMsg* GenericResponseMsg_construct(void);
static inline GenericResponseMsg* GenericResponseMsg_constructFromValue(
   GenericRespMsgCode controlCode, const char* logStr);
static inline void GenericResponseMsg_uninit(NetMessage* this);
static inline void GenericResponseMsg_destruct(NetMessage* this);

// getters & setters
static inline GenericRespMsgCode GenericResponseMsg_getControlCode(GenericResponseMsg* this);
static inline const char* GenericResponseMsg_getLogStr(GenericResponseMsg* this);


/**
 * A generic response that can be sent as a reply to any message. This special control message will
 * be handled internally by the requestors MessageTk::requestResponse...() method.
 *
 * This is intended to submit internal information (like asking for a communication retry) to the
 * requestors MessageTk through the control code. So the MessageTk caller on the requestor side
 * will never actually see this GenericResponseMsg.
 *
 * The message string is intended to provide additional human-readable information like why this
 * control code was submitted instead of the actually expected response msg.
 */
struct GenericResponseMsg
{
   SimpleIntStringMsg simpleIntStringMsg;
};


void GenericResponseMsg_init(GenericResponseMsg* this)
{
   SimpleIntStringMsg_init( (SimpleIntStringMsg*)this, NETMSGTYPE_GenericResponse);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = GenericResponseMsg_uninit;
}

/**
 * @param controlCode GenericRespMsgCode_...
 * @param logStr human-readable explanation or background information; (just a reference, so don't
 *    free or modify it while this msg is used).
 */
void GenericResponseMsg_initFromValue(GenericResponseMsg* this, GenericRespMsgCode controlCode,
   const char* logStr)
{
   SimpleIntStringMsg_initFromValue( (SimpleIntStringMsg*)this, NETMSGTYPE_GenericResponse,
      controlCode, logStr);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = GenericResponseMsg_uninit;
}

GenericResponseMsg* GenericResponseMsg_construct(void)
{
   struct GenericResponseMsg* this = os_kmalloc(sizeof(*this) );

   if(likely(this) )
      GenericResponseMsg_init(this);

   return this;
}

/**
 * @param controlCode GenericRespMsgCode_...
 * @param logStr human-readable explanation or background information; (just a reference, so don't
 *    free or modify it while this msg is used).
 */
GenericResponseMsg* GenericResponseMsg_constructFromValue(GenericRespMsgCode controlCode,
   const char* logStr)
{
   struct GenericResponseMsg* this = os_kmalloc(sizeof(*this) );

   if(likely(this) )
      GenericResponseMsg_initFromValue(this, controlCode, logStr);

   return this;
}

void GenericResponseMsg_uninit(NetMessage* this)
{
   SimpleIntStringMsg_uninit( (NetMessage*)this);
}

void GenericResponseMsg_destruct(NetMessage* this)
{
   GenericResponseMsg_uninit( (NetMessage*)this);

   os_kfree(this);
}


GenericRespMsgCode GenericResponseMsg_getControlCode(GenericResponseMsg* this)
{
   return (GenericRespMsgCode)SimpleIntStringMsg_getIntValue( (SimpleIntStringMsg*)this);
}

const char* GenericResponseMsg_getLogStr(GenericResponseMsg* this)
{
   return SimpleIntStringMsg_getStrValue( (SimpleIntStringMsg*)this);
}


#endif /* GENERICRESPONSEMSG_H_ */

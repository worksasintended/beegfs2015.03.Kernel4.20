#ifndef LOCKGRANTEDMSGEX_H_
#define LOCKGRANTEDMSGEX_H_

#include <common/net/message/NetMessage.h>


/**
 * This message is for deserialization (incoming) only, serialization is not implemented!
 */


struct LockGrantedMsgEx;
typedef struct LockGrantedMsgEx LockGrantedMsgEx;

static inline void LockGrantedMsgEx_init(LockGrantedMsgEx* this);
static inline LockGrantedMsgEx* LockGrantedMsgEx_construct(void);
static inline void LockGrantedMsgEx_uninit(NetMessage* this);
static inline void LockGrantedMsgEx_destruct(NetMessage* this);

// virtual functions
extern void LockGrantedMsgEx_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool LockGrantedMsgEx_deserializePayload(NetMessage* this, const char* buf,
   size_t bufLen);
extern unsigned LockGrantedMsgEx_calcMessageLength(NetMessage* this);
extern fhgfs_bool __LockGrantedMsgEx_processIncoming(NetMessage* this, struct App* app,
   fhgfs_sockaddr_in* fromAddr, struct Socket* sock, char* respBuf, size_t bufLen);

// getters & setters
static inline const char* LockGrantedMsgEx_getLockAckID(LockGrantedMsgEx* this);
static inline const char* LockGrantedMsgEx_getAckID(LockGrantedMsgEx* this);
static inline uint16_t LockGrantedMsgEx_getGranterNodeID(LockGrantedMsgEx* this);


struct LockGrantedMsgEx
{
   NetMessage netMessage;

   unsigned lockAckIDLen;
   const char* lockAckID;
   unsigned ackIDLen;
   const char* ackID;
   uint16_t granterNodeID;
};


void LockGrantedMsgEx_init(LockGrantedMsgEx* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_LockGranted);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = LockGrantedMsgEx_uninit;

   ( (NetMessage*)this)->serializePayload = LockGrantedMsgEx_serializePayload;
   ( (NetMessage*)this)->deserializePayload = LockGrantedMsgEx_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = LockGrantedMsgEx_calcMessageLength;

   ( (NetMessage*)this)->processIncoming = __LockGrantedMsgEx_processIncoming;
}

LockGrantedMsgEx* LockGrantedMsgEx_construct(void)
{
   struct LockGrantedMsgEx* this = os_kmalloc(sizeof(*this) );

   LockGrantedMsgEx_init(this);

   return this;
}

void LockGrantedMsgEx_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void LockGrantedMsgEx_destruct(NetMessage* this)
{
   LockGrantedMsgEx_uninit(this);

   os_kfree(this);
}

const char* LockGrantedMsgEx_getLockAckID(LockGrantedMsgEx* this)
{
   return this->lockAckID;
}

const char* LockGrantedMsgEx_getAckID(LockGrantedMsgEx* this)
{
   return this->ackID;
}

uint16_t LockGrantedMsgEx_getGranterNodeID(LockGrantedMsgEx* this)
{
   return this->granterNodeID;
}


#endif /* LOCKGRANTEDMSGEX_H_ */

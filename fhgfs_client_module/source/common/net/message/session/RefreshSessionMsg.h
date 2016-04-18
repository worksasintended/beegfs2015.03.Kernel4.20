#ifndef REFRESHSESSIONMSG_H_
#define REFRESHSESSIONMSG_H_

#include <common/net/message/NetMessage.h>

struct RefreshSessionMsg;
typedef struct RefreshSessionMsg RefreshSessionMsg;

static inline void RefreshSessionMsg_init(RefreshSessionMsg* this);
static inline void RefreshSessionMsg_initFromSession(RefreshSessionMsg* this,
   const char* sessionID);
static inline RefreshSessionMsg* RefreshSessionMsg_construct(void);
static inline RefreshSessionMsg* RefreshSessionMsg_constructFromSession(
   const char* sessionID);
static inline void RefreshSessionMsg_uninit(NetMessage* this);
static inline void RefreshSessionMsg_destruct(NetMessage* this);

// virtual functions
extern void RefreshSessionMsg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool RefreshSessionMsg_deserializePayload(NetMessage* this,
   const char* buf, size_t bufLen);
extern unsigned RefreshSessionMsg_calcMessageLength(NetMessage* this);

// getters & setters
static inline const char* RefreshSessionMsg_getSessionID(RefreshSessionMsg* this);


struct RefreshSessionMsg
{
   NetMessage netMessage;
   
   unsigned sessionIDLen;
   const char* sessionID;
};


void RefreshSessionMsg_init(RefreshSessionMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_RefreshSession);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = RefreshSessionMsg_uninit;

   ( (NetMessage*)this)->serializePayload = RefreshSessionMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = RefreshSessionMsg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = RefreshSessionMsg_calcMessageLength;
}
   
/**
 * @param sessionID just a reference, so do not free it as long as you use this object!
 */
void RefreshSessionMsg_initFromSession(RefreshSessionMsg* this,
   const char* sessionID)
{
   RefreshSessionMsg_init(this);
   
   this->sessionID = sessionID;
   this->sessionIDLen = os_strlen(sessionID);
}

RefreshSessionMsg* RefreshSessionMsg_construct(void)
{
   struct RefreshSessionMsg* this = os_kmalloc(sizeof(struct RefreshSessionMsg) );
   
   RefreshSessionMsg_init(this);
   
   return this;
}

/**
 * @param sessionID just a reference, so do not free it as long as you use this object!
 */
RefreshSessionMsg* RefreshSessionMsg_constructFromSession(
   const char* sessionID)
{
   struct RefreshSessionMsg* this = os_kmalloc(sizeof(struct RefreshSessionMsg) );
   
   RefreshSessionMsg_initFromSession(this, sessionID);
   
   return this;
}

void RefreshSessionMsg_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void RefreshSessionMsg_destruct(NetMessage* this)
{
   RefreshSessionMsg_uninit(this);
   
   os_kfree(this);
}

const char* RefreshSessionMsg_getSessionID(RefreshSessionMsg* this)
{
   return this->sessionID;
}

#endif /* REFRESHSESSIONMSG_H_ */

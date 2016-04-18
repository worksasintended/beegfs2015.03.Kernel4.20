#ifndef FSYNCLOCALFILEMSG_H_
#define FSYNCLOCALFILEMSG_H_

#include <common/net/message/NetMessage.h>


#define FSYNCLOCALFILEMSG_FLAG_NO_SYNC          1 /* if a sync is not needed */
#define FSYNCLOCALFILEMSG_FLAG_SESSION_CHECK    2 /* if session check should be done */
#define FSYNCLOCALFILEMSG_FLAG_BUDDYMIRROR      4 /* given targetID is a buddymirrorgroup ID */
#define FSYNCLOCALFILEMSG_FLAG_BUDDYMIRROR_SECOND 8 /* secondary of group, otherwise primary */


struct FSyncLocalFileMsg;
typedef struct FSyncLocalFileMsg FSyncLocalFileMsg;

static inline void FSyncLocalFileMsg_init(FSyncLocalFileMsg* this);
static inline void FSyncLocalFileMsg_initFromSession(FSyncLocalFileMsg* this,
   const char* sessionID, const char* fileHandleID, uint16_t targetID);
static inline FSyncLocalFileMsg* FSyncLocalFileMsg_construct(void);
static inline FSyncLocalFileMsg* FSyncLocalFileMsg_constructFromSession(
   const char* sessionID, const char* fileHandleID, uint16_t targetID);
static inline void FSyncLocalFileMsg_uninit(NetMessage* this);
static inline void FSyncLocalFileMsg_destruct(NetMessage* this);

// virtual functions
extern void FSyncLocalFileMsg_serializePayload(NetMessage* this, char* buf);
extern unsigned FSyncLocalFileMsg_calcMessageLength(NetMessage* this);



/**
 * Note: This message supports only serialization, deserialization is not implemented.
 */
struct FSyncLocalFileMsg
{
   NetMessage netMessage;
   
   const char* sessionID;
   unsigned sessionIDLen;
   const char* fileHandleID;
   unsigned fileHandleIDLen;
   uint16_t targetID;
};

void FSyncLocalFileMsg_init(FSyncLocalFileMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_FSyncLocalFile);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = FSyncLocalFileMsg_uninit;

   ( (NetMessage*)this)->serializePayload = FSyncLocalFileMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = _NetMessage_deserializeDummy;
   ( (NetMessage*)this)->calcMessageLength = FSyncLocalFileMsg_calcMessageLength;
}
   
/**
 * @param sessionID just a reference, so do not free it as long as you use this object!
 * @param fileHandleID just a reference, so do not free it as long as you use this object!
 */
void FSyncLocalFileMsg_initFromSession(FSyncLocalFileMsg* this, const char* sessionID,
   const char* fileHandleID, uint16_t targetID)
{
   FSyncLocalFileMsg_init(this);
   
   this->sessionID = sessionID;
   this->sessionIDLen = os_strlen(sessionID);
   
   this->fileHandleID = fileHandleID;
   this->fileHandleIDLen = os_strlen(fileHandleID);

   this->targetID = targetID;
}

FSyncLocalFileMsg* FSyncLocalFileMsg_construct(void)
{
   struct FSyncLocalFileMsg* this = os_kmalloc(sizeof(*this) );
   
   FSyncLocalFileMsg_init(this);
   
   return this;
}

/**
 * @param sessionID just a reference, so do not free it as long as you use this object!
 * @param fileHandleID just a reference, so do not free it as long as you use this object!
 */
FSyncLocalFileMsg* FSyncLocalFileMsg_constructFromSession(const char* sessionID,
   const char* fileHandleID, uint16_t targetID)
{
   struct FSyncLocalFileMsg* this = os_kmalloc(sizeof(*this) );
   
   FSyncLocalFileMsg_initFromSession(this, sessionID, fileHandleID, targetID);
   
   return this;
}

void FSyncLocalFileMsg_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void FSyncLocalFileMsg_destruct(NetMessage* this)
{
   FSyncLocalFileMsg_uninit(this);
   
   os_kfree(this);
}

#endif /*FSYNCLOCALFILEMSG_H_*/

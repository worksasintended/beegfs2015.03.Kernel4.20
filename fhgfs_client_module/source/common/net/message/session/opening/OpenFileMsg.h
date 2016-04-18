#ifndef OPENFILEMSG_H_
#define OPENFILEMSG_H_

#include <common/storage/EntryInfo.h>
#include <common/net/message/NetMessage.h>


#define OPENFILEMSG_FLAG_USE_QUOTA        1 /* if the message contains quota informations */


struct OpenFileMsg;
typedef struct OpenFileMsg OpenFileMsg;

static inline void OpenFileMsg_init(OpenFileMsg* this);
static inline void OpenFileMsg_initFromSession(OpenFileMsg* this,
   const char* sessionID, const EntryInfo* entryInfo, unsigned accessFlags);
static inline OpenFileMsg* OpenFileMsg_construct(void);
static inline OpenFileMsg* OpenFileMsg_constructFromSession(
   const char* sessionID, const EntryInfo* entryInfo, unsigned accessFlags);
static inline void OpenFileMsg_uninit(NetMessage* this);
static inline void OpenFileMsg_destruct(NetMessage* this);

// virtual functions
extern void OpenFileMsg_serializePayload(NetMessage* this, char* buf);
extern unsigned OpenFileMsg_calcMessageLength(NetMessage* this);


struct OpenFileMsg
{
   NetMessage netMessage;
   
   const char* sessionID;         // not owned by this object
   unsigned sessionIDLen;
   const EntryInfo* entryInfoPtr; // not owned by this object
   unsigned accessFlags;
};


void OpenFileMsg_init(OpenFileMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_OpenFile);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = OpenFileMsg_uninit;

   ( (NetMessage*)this)->serializePayload = OpenFileMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = _NetMessage_deserializeDummy;
   ( (NetMessage*)this)->calcMessageLength = OpenFileMsg_calcMessageLength;
}
   
/**
 * @param sessionID just a reference, so do not free it as long as you use this object!
 * @param entryInfoPtr just a reference, so do not free it as long as you use this object!
 * @param accessFlags OPENFILE_ACCESS_... flags
 */
void OpenFileMsg_initFromSession(OpenFileMsg* this,
   const char* sessionID, const EntryInfo* entryInfo, unsigned accessFlags)
{
   OpenFileMsg_init(this);
   
   this->sessionID = sessionID;
   this->sessionIDLen = os_strlen(sessionID);
   
   this->entryInfoPtr = entryInfo;

   this->accessFlags = accessFlags;
}

OpenFileMsg* OpenFileMsg_construct(void)
{
   struct OpenFileMsg* this = os_kmalloc(sizeof(*this) );
   
   OpenFileMsg_init(this);
   
   return this;
}

/**
 * @param sessionID just a reference, so do not free it as long as you use this object!
 * @param entryID just a reference, so do not free it as long as you use this object!
 * @param accessFlags OPENFILE_ACCESS_... flags
 */
OpenFileMsg* OpenFileMsg_constructFromSession(
   const char* sessionID, const EntryInfo* entryInfo, unsigned accessFlags)
{
   struct OpenFileMsg* this = os_kmalloc(sizeof(*this) );
   
   OpenFileMsg_initFromSession(this, sessionID, entryInfo, accessFlags);
   
   return this;
}

void OpenFileMsg_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void OpenFileMsg_destruct(NetMessage* this)
{
   OpenFileMsg_uninit(this);
   
   os_kfree(this);
}


#endif /*OPENFILEMSG_H_*/

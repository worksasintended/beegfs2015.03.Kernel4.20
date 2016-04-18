#ifndef CLOSEFILEMSG_H_
#define CLOSEFILEMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/EntryInfo.h>


#define CLOSEFILEMSG_FLAG_EARLYRESPONSE      1 /* send response before chunk files close */
#define CLOSEFILEMSG_FLAG_CANCELAPPENDLOCKS  2 /* cancel append locks of this file handle */


/**
 * This message supports sending (serialization) only, i.e. no deserialization implemented!!
 */

struct CloseFileMsg;
typedef struct CloseFileMsg CloseFileMsg;

static inline void CloseFileMsg_init(CloseFileMsg* this);
static inline void CloseFileMsg_initFromSession(CloseFileMsg* this,
   const char* sessionID, const char* fileHandleID, const EntryInfo* entryInfo, int maxUsedNodeIndex);
static inline CloseFileMsg* CloseFileMsg_construct(void);
static inline CloseFileMsg* CloseFileMsg_constructFromSession(
   const char* sessionID, const char* fileHandleID, const EntryInfo* entryInfo, int maxUsedNodeIndex);
static inline void CloseFileMsg_uninit(NetMessage* this);
static inline void CloseFileMsg_destruct(NetMessage* this);

// virtual functions
extern void CloseFileMsg_serializePayload(NetMessage* this, char* buf);
extern unsigned CloseFileMsg_calcMessageLength(NetMessage* this);

// getters & setters
static inline const char* CloseFileMsg_getSessionID(CloseFileMsg* this);
static inline const char* CloseFileMsg_getFileHandleID(CloseFileMsg* this);
static inline int CloseFileMsg_getMaxUsedNodeIndex(CloseFileMsg* this);


struct CloseFileMsg
{
   NetMessage netMessage;
   
   unsigned sessionIDLen;
   const char* sessionID;
   unsigned fileHandleIDLen;
   const char* fileHandleID;
   int maxUsedNodeIndex;

   // for serialization
   const EntryInfo* entryInfoPtr; // not owned by this object
};


void CloseFileMsg_init(CloseFileMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_CloseFile);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = CloseFileMsg_uninit;

   ( (NetMessage*)this)->serializePayload = CloseFileMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = _NetMessage_deserializeDummy;
   ( (NetMessage*)this)->calcMessageLength = CloseFileMsg_calcMessageLength;
}
   
/**
 * @param sessionID just a reference, so do not free it as long as you use this object!
 * @param fileHandleID just a reference, so do not free it as long as you use this object!
 * @param entryInfo just a reference, so do not free it as long as you use this object!
 */
void CloseFileMsg_initFromSession(CloseFileMsg* this, const char* sessionID,
   const char* fileHandleID, const EntryInfo* entryInfo, int maxUsedNodeIndex)
{
   CloseFileMsg_init(this);
   
   this->sessionID = sessionID;
   this->sessionIDLen = os_strlen(sessionID);
   
   this->fileHandleID = fileHandleID;
   this->fileHandleIDLen = os_strlen(fileHandleID);
   
   this->entryInfoPtr = entryInfo;

   this->maxUsedNodeIndex = maxUsedNodeIndex;
}

CloseFileMsg* CloseFileMsg_construct(void)
{
   struct CloseFileMsg* this = os_kmalloc(sizeof(*this) );
   
   CloseFileMsg_init(this);
   
   return this;
}

/**
 * @param sessionID just a reference, so do not free it as long as you use this object!
 * @param fileHandleID just a reference, so do not free it as long as you use this object!
 */
CloseFileMsg* CloseFileMsg_constructFromSession(const char* sessionID, const char* fileHandleID,
   const EntryInfo* entryInfo, int maxUsedNodeIndex)
{
   struct CloseFileMsg* this = os_kmalloc(sizeof(struct CloseFileMsg) );
   
   CloseFileMsg_initFromSession(this, sessionID, fileHandleID, entryInfo, maxUsedNodeIndex);
   
   return this;
}

void CloseFileMsg_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void CloseFileMsg_destruct(NetMessage* this)
{
   CloseFileMsg_uninit(this);
   
   os_kfree(this);
}


const char* CloseFileMsg_getSessionID(CloseFileMsg* this)
{
   return this->sessionID;
}

const char* CloseFileMsg_getFileHandleID(CloseFileMsg* this)
{
   return this->fileHandleID;
}

int CloseFileMsg_getMaxUsedNodeIndex(CloseFileMsg* this)
{
   return this->maxUsedNodeIndex;
}


#endif /*CLOSEFILEMSG_H_*/

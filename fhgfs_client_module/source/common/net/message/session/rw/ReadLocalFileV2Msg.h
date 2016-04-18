#ifndef READLOCALFILEV2MSG_H_
#define READLOCALFILEV2MSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/PathInfo.h>


#define READLOCALFILEMSG_FLAG_SESSION_CHECK       1 /* if session check infos should be done */
#define READLOCALFILEMSG_FLAG_DISABLE_IO          2 /* disable read syscall for net bench */
#define READLOCALFILEMSG_FLAG_BUDDYMIRROR         4 /* given targetID is a buddymirrorgroup ID */
#define READLOCALFILEMSG_FLAG_BUDDYMIRROR_SECOND  8 /* secondary of group, otherwise primary */


struct ReadLocalFileV2Msg;
typedef struct ReadLocalFileV2Msg ReadLocalFileV2Msg;

static inline void ReadLocalFileV2Msg_init(ReadLocalFileV2Msg* this);
static inline void ReadLocalFileV2Msg_initFromSession(ReadLocalFileV2Msg* this,
   const char* sessionID, const char* fileHandleID, uint16_t targetID, PathInfo* pathInfoPtr,
   unsigned accessFlags, int64_t offset, int64_t count);
static inline ReadLocalFileV2Msg* ReadLocalFileV2Msg_construct(void);
static inline ReadLocalFileV2Msg* ReadLocalFileV2Msg_constructFromSession(
   const char* sessionID, const char* fileHandleID, uint16_t targetID, PathInfo* pathInfoPtr,
   unsigned accessFlags, int64_t offset, int64_t count);
static inline void ReadLocalFileV2Msg_uninit(NetMessage* this);
static inline void ReadLocalFileV2Msg_destruct(NetMessage* this);

// virtual functions
extern void ReadLocalFileV2Msg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool ReadLocalFileV2Msg_deserializePayload(NetMessage* this, const char* buf,
   size_t bufLen);
extern unsigned ReadLocalFileV2Msg_calcMessageLength(NetMessage* this);

// getters & setters
static inline const char* ReadLocalFileV2Msg_getSessionID(ReadLocalFileV2Msg* this);
static inline const char* ReadLocalFileV2Msg_getFileHandleID(ReadLocalFileV2Msg* this);
static inline uint16_t ReadLocalFileV2Msg_getTargetID(ReadLocalFileV2Msg* this);
static inline unsigned ReadLocalFileV2Msg_getAccessFlags(ReadLocalFileV2Msg* this);
static inline int64_t ReadLocalFileV2Msg_getOffset(ReadLocalFileV2Msg* this);
static inline int64_t ReadLocalFileV2Msg_getCount(ReadLocalFileV2Msg* this);


struct ReadLocalFileV2Msg
{
   NetMessage netMessage;
   
   int64_t offset;
   int64_t count;
   unsigned accessFlags;
   const char* sessionID;
   unsigned sessionIDLen;
   const char* fileHandleID;
   unsigned fileHandleIDLen;
   PathInfo* pathInfoPtr;
   uint16_t targetID;
};


void ReadLocalFileV2Msg_init(ReadLocalFileV2Msg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_ReadLocalFileV2);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = ReadLocalFileV2Msg_uninit;

   ( (NetMessage*)this)->serializePayload   = ReadLocalFileV2Msg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = _NetMessage_deserializeDummy;
   ( (NetMessage*)this)->calcMessageLength  = ReadLocalFileV2Msg_calcMessageLength;
}

/**
 * @param sessionID just a reference, so do not free it as long as you use this object!
 */
void ReadLocalFileV2Msg_initFromSession(ReadLocalFileV2Msg* this,
   const char* sessionID, const char* fileHandleID, uint16_t targetID, PathInfo* pathInfoPtr,
   unsigned accessFlags, int64_t offset, int64_t count)
{
   ReadLocalFileV2Msg_init(this);
   
   this->sessionID = sessionID;
   this->sessionIDLen = os_strlen(sessionID);
   
   this->fileHandleID = fileHandleID;
   this->fileHandleIDLen = os_strlen(fileHandleID);

   this->targetID = targetID;

   this->pathInfoPtr = pathInfoPtr;

   this->accessFlags = accessFlags;

   this->offset = offset;
   this->count = count;
}

ReadLocalFileV2Msg* ReadLocalFileV2Msg_construct(void)
{
   struct ReadLocalFileV2Msg* this = os_kmalloc(sizeof(*this) );
   
   if(likely(this) )
      ReadLocalFileV2Msg_init(this);
   
   return this;
}

/**
 * @param sessionID just a reference, so do not free it as long as you use this object!
 */
ReadLocalFileV2Msg* ReadLocalFileV2Msg_constructFromSession(
   const char* sessionID, const char* fileHandleID, uint16_t targetID, PathInfo* pathInfoPtr,
   unsigned accessFlags, int64_t offset, int64_t count)
{
   struct ReadLocalFileV2Msg* this = os_kmalloc(sizeof(*this) );
   
   if(likely(this) )
      ReadLocalFileV2Msg_initFromSession(this, sessionID, fileHandleID, targetID, pathInfoPtr,
         accessFlags, offset, count);
   
   return this;
}

void ReadLocalFileV2Msg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void ReadLocalFileV2Msg_destruct(NetMessage* this)
{
   ReadLocalFileV2Msg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


const char* ReadLocalFileV2Msg_getSessionID(ReadLocalFileV2Msg* this)
{
   return this->sessionID;
}

const char* ReadLocalFileV2Msg_getFileHandleID(ReadLocalFileV2Msg* this)
{
   return this->fileHandleID;
}

uint16_t ReadLocalFileV2Msg_getTargetID(ReadLocalFileV2Msg* this)
{
   return this->targetID;
}

unsigned ReadLocalFileV2Msg_getAccessFlags(ReadLocalFileV2Msg* this)
{
   return this->accessFlags;
}

int64_t ReadLocalFileV2Msg_getOffset(ReadLocalFileV2Msg* this)
{
   return this->offset;
}

int64_t ReadLocalFileV2Msg_getCount(ReadLocalFileV2Msg* this)
{
   return this->count;
}



#endif /*READLOCALFILEV2MSG_H_*/

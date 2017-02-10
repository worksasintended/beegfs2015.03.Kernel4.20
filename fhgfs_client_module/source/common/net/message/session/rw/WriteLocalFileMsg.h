#ifndef WRITELOCALFILEMSG_H_
#define WRITELOCALFILEMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/StorageErrors.h>
#include <common/storage/PathInfo.h>
#include <app/log/Logger.h>
#include <app/App.h>
#include <os/iov_iter.h>


#define WRITELOCALFILEMSG_FLAG_SESSION_CHECK        1 /* if session check should be done */
#define WRITELOCALFILEMSG_FLAG_USE_QUOTA            2 /* if msg contains quota info */
#define WRITELOCALFILEMSG_FLAG_DISABLE_IO           4 /* disable write syscall for net bench mode */
#define WRITELOCALFILEMSG_FLAG_BUDDYMIRROR          8 /* given targetID is a buddymirrorgroup ID */
#define WRITELOCALFILEMSG_FLAG_BUDDYMIRROR_SECOND  16 /* secondary of group, otherwise primary */
#define WRITELOCALFILEMSG_FLAG_BUDDYMIRROR_FORWARD 32 /* forward msg to secondary */


struct WriteLocalFileMsg;
typedef struct WriteLocalFileMsg WriteLocalFileMsg;


static inline void WriteLocalFileMsg_init(WriteLocalFileMsg* this);
static inline void WriteLocalFileMsg_initFromSession(WriteLocalFileMsg* this,
   const char* sessionID, const char* fileHandleID, uint16_t targetID, PathInfo* pathInfo,
   unsigned accessFlags, int64_t offset, int64_t count);
static inline WriteLocalFileMsg* WriteLocalFileMsg_construct(void);
static inline WriteLocalFileMsg* WriteLocalFileMsg_constructFromSession(
   const char* sessionID, const char* fileHandleID, uint16_t targetID, PathInfo* pathInfo,
   unsigned accessFlags, int64_t offset, int64_t count);
static inline void WriteLocalFileMsg_uninit(NetMessage* this);
static inline void WriteLocalFileMsg_destruct(NetMessage* this);

// virtual functions
extern void WriteLocalFileMsg_serializePayload(NetMessage* this, char* buf);
extern unsigned WriteLocalFileMsg_calcMessageLength(NetMessage* this);

// inliners
static inline FhgfsOpsErr WriteLocalFileMsg_sendDataPartInstant(App* app, struct iov_iter* data,
   size_t partLen, size_t* partSentLen, Socket* sock);

// getters & setters
static inline const char* WriteLocalFileMsg_getSessionID(WriteLocalFileMsg* this);
static inline const char* WriteLocalFileMsg_getFileHandleID(WriteLocalFileMsg* this);
static inline uint16_t WriteLocalFileMsg_getTargetID(WriteLocalFileMsg* this);
static inline unsigned WriteLocalFileMsg_getAccessFlags(WriteLocalFileMsg* this);
static inline int64_t WriteLocalFileMsg_getOffset(WriteLocalFileMsg* this);
static inline int64_t WriteLocalFileMsg_getCount(WriteLocalFileMsg* this);

static inline unsigned WriteLocalFileMsg_getUserID(WriteLocalFileMsg* this);
static inline unsigned WriteLocalFileMsg_getGroupID(WriteLocalFileMsg* this);
static inline void WriteLocalFileMsg_setUserdataForQuota(WriteLocalFileMsg* this, unsigned userID,
   unsigned groupID);


/**
 * Note: This message supports only serialization, deserialization is not implemented.
 */
struct WriteLocalFileMsg
{
   NetMessage netMessage;

   int64_t offset;
   int64_t count;
   unsigned accessFlags;
   const char* sessionID;
   unsigned sessionIDLen;
   const char* fileHandleID;
   unsigned fileHandleIDLen;
   uint16_t targetID;
   PathInfo* pathInfo;
   unsigned userID;
   unsigned groupID;
};


void WriteLocalFileMsg_init(WriteLocalFileMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_WriteLocalFile);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = WriteLocalFileMsg_uninit;

   ( (NetMessage*)this)->serializePayload = WriteLocalFileMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = _NetMessage_deserializeDummy;
   ( (NetMessage*)this)->calcMessageLength = WriteLocalFileMsg_calcMessageLength;
}

/**
 * @param sessionID just a reference, so do not free it as long as you use this object!
 * @param fileHandleID just a reference, so do not free it as long as you use this object!
 * @param pathInfo just a reference, so do not free it as long as you use this object!
 */
void WriteLocalFileMsg_initFromSession(WriteLocalFileMsg* this,
   const char* sessionID, const char* fileHandleID, uint16_t targetID, PathInfo* pathInfo,
   unsigned accessFlags,
   int64_t offset, int64_t count)
{
   WriteLocalFileMsg_init(this);

   this->sessionID = sessionID;
   this->sessionIDLen = os_strlen(sessionID);

   this->fileHandleID = fileHandleID;
   this->fileHandleIDLen = os_strlen(fileHandleID);

   this->targetID = targetID;
   this->pathInfo = pathInfo;

   this->accessFlags = accessFlags;

   this->offset = offset;
   this->count = count;

   /* only used when corresponding quota flag is set
   this->userID = 0;
   this->groupID = 0;
   */
}

WriteLocalFileMsg* WriteLocalFileMsg_construct(void)
{
   struct WriteLocalFileMsg* this = os_kmalloc(sizeof(*this) );

   if(likely(this) )
      WriteLocalFileMsg_init(this);

   return this;
}

/**
 * @param sessionID just a reference, so do not free it as long as you use this object!
 * @param fileHandleID just a reference, so do not free it as long as you use this object!
 * @param pathInfo just a reference, so do not free it as long as you use this object!
 */
WriteLocalFileMsg* WriteLocalFileMsg_constructFromSession(
   const char* sessionID, const char* fileHandleID, uint16_t targetID, PathInfo* pathInfo,
   unsigned accessFlags, int64_t offset, int64_t count)
{
   struct WriteLocalFileMsg* this = os_kmalloc(sizeof(*this) );

   if(likely(this) )
      WriteLocalFileMsg_initFromSession(this, sessionID, fileHandleID, targetID, pathInfo,
         accessFlags, offset, count);

   return this;
}

void WriteLocalFileMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void WriteLocalFileMsg_destruct(NetMessage* this)
{
   WriteLocalFileMsg_uninit( (NetMessage*)this);

   os_kfree(this);
}



/**
 * Uses MSG_DONTWAIT to send the data non-blocking.
 *
 * Note: The length parameter differs from the one of WriteLocalFileMsg_sendDataPartExact
 *
 * @partSentLen will automatically be increased on success
 */
FhgfsOpsErr WriteLocalFileMsg_sendDataPartInstant(App* app, struct iov_iter* data,
   size_t partLen, size_t* partSentLen, Socket* sock)
{
   ssize_t sendRes = 0;

   while (data->count)
   {
      struct iovec iov;

      iov = iov_iter_iovec(data);
      sendRes = sock->send(sock, iov.iov_base, iov.iov_len, BEEGFS_MSG_DONTWAIT);

      if (sendRes < 0)
         break;

      *partSentLen += sendRes;
      iov_iter_advance(data, sendRes);
   }

   if(sendRes > 0)
      return FhgfsOpsErr_SUCCESS;
   else
   if(sendRes == -EAGAIN)
   { // send would block (nothing was sent)
      return FhgfsOpsErr_SUCCESS;
   }
   else
   if(sendRes == -EFAULT)
   { // bad buffer address given
      return FhgfsOpsErr_ADDRESSFAULT;
   }
   else
   { // error
      Logger* log = App_getLogger(app);
      const char* logContext = "WriteLocalFileMsg_sendDataPartInstant";

      Logger_logFormatted(log, Log_WARNING, logContext, "SocketError. ErrCode: %lld",
         (long long)sendRes);
      Logger_logFormatted(log, Log_SPAM, logContext, "partLen/missing: %lld/%lld",
         (long long)partLen, (long long)data->count);

      return FhgfsOpsErr_COMMUNICATION;
   }

}


const char* WriteLocalFileMsg_getSessionID(WriteLocalFileMsg* this)
{
   return this->sessionID;
}

const char* WriteLocalFileMsg_getFileHandleID(WriteLocalFileMsg* this)
{
   return this->fileHandleID;
}

uint16_t WriteLocalFileMsg_getTargetID(WriteLocalFileMsg* this)
{
   return this->targetID;
}

unsigned WriteLocalFileMsg_getAccessFlags(WriteLocalFileMsg* this)
{
   return this->accessFlags;
}

int64_t WriteLocalFileMsg_getOffset(WriteLocalFileMsg* this)
{
   return this->offset;
}

int64_t WriteLocalFileMsg_getCount(WriteLocalFileMsg* this)
{
   return this->count;
}

unsigned WriteLocalFileMsg_getUserID(WriteLocalFileMsg* this)
{
   return this->userID;
}

unsigned WriteLocalFileMsg_getGroupID(WriteLocalFileMsg* this)
{
   return this->groupID;
}

void WriteLocalFileMsg_setUserdataForQuota(WriteLocalFileMsg* this, unsigned userID,
   unsigned groupID)
{
   NetMessage* thisCast = (NetMessage*) this;

   NetMessage_addMsgHeaderFeatureFlag(thisCast, WRITELOCALFILEMSG_FLAG_USE_QUOTA);

   this->userID = userID;
   this->groupID = groupID;
}

#endif /*WRITELOCALFILEMSG_H_*/

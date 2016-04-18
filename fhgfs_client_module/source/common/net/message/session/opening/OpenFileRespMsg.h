#ifndef OPENFILERESPMSG_H_
#define OPENFILERESPMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/PathInfo.h>
#include <common/storage/striping/StripePattern.h>
#include <common/Common.h>


struct OpenFileRespMsg;
typedef struct OpenFileRespMsg OpenFileRespMsg;

static inline void OpenFileRespMsg_init(OpenFileRespMsg* this);
static inline OpenFileRespMsg* OpenFileRespMsg_construct(void);
static inline void OpenFileRespMsg_uninit(NetMessage* this);
static inline void OpenFileRespMsg_destruct(NetMessage* this);

// virtual functions
extern void OpenFileRespMsg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool OpenFileRespMsg_deserializePayload(NetMessage* this,
   const char* buf, size_t bufLen);
extern unsigned OpenFileRespMsg_calcMessageLength(NetMessage* this);

// inliners
static inline StripePattern* OpenFileRespMsg_createPattern(OpenFileRespMsg* this);

// getters & setters
static inline int OpenFileRespMsg_getResult(OpenFileRespMsg* this);
static inline const char* OpenFileRespMsg_getFileHandleID(OpenFileRespMsg* this);
static inline const PathInfo* OpenFileRespMsg_getPathInfo(OpenFileRespMsg* this);

struct OpenFileRespMsg
{
   NetMessage netMessage;
   
   int result;
   unsigned fileHandleIDLen;
   const char* fileHandleID;

   PathInfo pathInfo;

   // for serialization
   StripePattern* pattern; // not owned by this object!
   
   // for deserialization
   struct StripePatternHeader patternHeader;
   const char* patternStart;
};


void OpenFileRespMsg_init(OpenFileRespMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_OpenFileResp);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = OpenFileRespMsg_uninit;

   ( (NetMessage*)this)->serializePayload    = _NetMessage_serializeDummy;
   ( (NetMessage*)this)->deserializePayload  = OpenFileRespMsg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength   = _NetMessage_calcMessageLengthDummy;
}

OpenFileRespMsg* OpenFileRespMsg_construct(void)
{
   struct OpenFileRespMsg* this = os_kmalloc(sizeof(*this) );

   OpenFileRespMsg_init(this);

   return this;
}

   
void OpenFileRespMsg_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void OpenFileRespMsg_destruct(NetMessage* this)
{
   OpenFileRespMsg_uninit(this);
   
   os_kfree(this);
}

/**
 * @return must be deleted by the caller; might be of type STRIPEPATTERN_Invalid
 */
StripePattern* OpenFileRespMsg_createPattern(OpenFileRespMsg* this)
{
   return StripePattern_createFromBuf(this->patternStart, &this->patternHeader);
}

int OpenFileRespMsg_getResult(OpenFileRespMsg* this)
{
   return this->result;
}

const char* OpenFileRespMsg_getFileHandleID(OpenFileRespMsg* this)
{
   return this->fileHandleID;
}

const PathInfo* OpenFileRespMsg_getPathInfo(OpenFileRespMsg* this)
{
   return &this->pathInfo;
}

#endif /*OPENFILERESPMSG_H_*/

#ifndef GETXATTRRESPMSG_H_
#define GETXATTRRESPMSG_H_

#include <common/net/message/NetMessage.h>

struct GetXAttrRespMsg;
typedef struct GetXAttrRespMsg GetXAttrRespMsg;

static inline void GetXAttrRespMsg_init(GetXAttrRespMsg* this);
static inline GetXAttrRespMsg* GetXAttrRespMsg_construct(void);
static inline void GetXAttrRespMsg_uninit(NetMessage* this);
static inline void GetXAttrRespMsg_destruct(NetMessage* this);

// virtual functions
extern fhgfs_bool GetXAttrRespMsg_deserializePayload(NetMessage* this, const char* buf,
      size_t bufLen);

// getters and setters
static inline char* GetXAttrRespMsg_getValueBuf(GetXAttrRespMsg* this);
static inline int GetXAttrRespMsg_getReturnCode(GetXAttrRespMsg* this);
static inline int GetXAttrRespMsg_getSize(GetXAttrRespMsg* this);

struct GetXAttrRespMsg
{
   NetMessage netMessage;

   unsigned valueBufLen; // only used for deserialization
   char* valueBuf;
   int size;
   int returnCode;
};

void GetXAttrRespMsg_init(GetXAttrRespMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_GetXAttrResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = GetXAttrRespMsg_uninit;

   ( (NetMessage*)this)->serializePayload   = _NetMessage_serializeDummy;
   ( (NetMessage*)this)->deserializePayload = GetXAttrRespMsg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength  = _NetMessage_calcMessageLengthDummy;
}

GetXAttrRespMsg* GetXAttrRespMsg_construct(void)
{
   struct GetXAttrRespMsg* this = os_kmalloc(sizeof(*this) );

   GetXAttrRespMsg_init(this);

   return this;
}

void GetXAttrRespMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void GetXAttrRespMsg_destruct(NetMessage* this)
{
   GetXAttrRespMsg_destruct( (NetMessage*)this);

   os_kfree(this);
}

char* GetXAttrRespMsg_getValueBuf(GetXAttrRespMsg* this)
{
   return this->valueBuf;
}

int GetXAttrRespMsg_getReturnCode(GetXAttrRespMsg* this)
{
   return this->returnCode;
}

int GetXAttrRespMsg_getSize(GetXAttrRespMsg* this)
{
   return this->size;
}

#endif /*GETXATTRRESPMSG_H_*/

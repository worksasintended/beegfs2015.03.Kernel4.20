#ifndef LISTXATTRRESPMSG_H_
#define LISTXATTRRESPMSG_H_

#include <common/net/message/NetMessage.h>

struct ListXAttrRespMsg;
typedef struct ListXAttrRespMsg ListXAttrRespMsg;

static inline void ListXAttrRespMsg_init(ListXAttrRespMsg* this);
static inline ListXAttrRespMsg* ListXAttrRespMsg_construct(void);
static inline void ListXAttrRespMsg_uninit(NetMessage* this);
static inline void ListXAttrRespMsg_destruct(NetMessage* this);

// virtual functions
extern fhgfs_bool ListXAttrRespMsg_deserializePayload(NetMessage* this, const char* buf,
      size_t bufLen);

// getters and setters
static inline char* ListXAttrRespMsg_getValueBuf(ListXAttrRespMsg* this);
static inline int ListXAttrRespMsg_getReturnCode(ListXAttrRespMsg* this);
static inline int ListXAttrRespMsg_getSize(ListXAttrRespMsg* this);

struct ListXAttrRespMsg
{
   NetMessage netMessage;

   unsigned valueElemNum;
   char* valueBuf;
   int size;
   int returnCode;
};

void ListXAttrRespMsg_init(ListXAttrRespMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_ListXAttrResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = ListXAttrRespMsg_uninit;

   ( (NetMessage*)this)->serializePayload   = _NetMessage_serializeDummy;
   ( (NetMessage*)this)->deserializePayload = ListXAttrRespMsg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength  = _NetMessage_calcMessageLengthDummy;
}

ListXAttrRespMsg* ListXAttrRespMsg_construct(void)
{
   struct ListXAttrRespMsg* this = os_kmalloc(sizeof(*this) );

   ListXAttrRespMsg_init(this);

   return this;
}

void ListXAttrRespMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void ListXAttrRespMsg_destruct(NetMessage* this)
{
   ListXAttrRespMsg_destruct( (NetMessage*)this);

   os_kfree(this);
}

char* ListXAttrRespMsg_getValueBuf(ListXAttrRespMsg* this)
{
   return this->valueBuf;
}

int ListXAttrRespMsg_getReturnCode(ListXAttrRespMsg* this)
{
   return this->returnCode;
}

int ListXAttrRespMsg_getSize(ListXAttrRespMsg* this)
{
   return this->size;
}

#endif /*LISTXATTRRESPMSG_H_*/

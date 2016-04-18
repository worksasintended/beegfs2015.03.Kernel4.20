#ifndef MKFILERESPMSG_H_
#define MKFILERESPMSG_H_

#include <common/storage/EntryInfo.h>
#include <common/net/message/NetMessage.h>

struct MkFileRespMsg;
typedef struct MkFileRespMsg MkFileRespMsg;

static inline void MkFileRespMsg_init(MkFileRespMsg* this);
static inline MkFileRespMsg* MkFileRespMsg_construct(void);
static inline void MkFileRespMsg_uninit(NetMessage* this);
static inline void MkFileRespMsg_destruct(NetMessage* this);

// virtual functions
extern void MkFileRespMsg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool MkFileRespMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen);
extern unsigned MkFileRespMsg_calcMessageLength(NetMessage* this);

// getters & setters
static inline int MkFileRespMsg_getResult(MkFileRespMsg* this);
static inline const EntryInfo* MkFileRespMsg_getEntryInfo(MkFileRespMsg* this);


struct MkFileRespMsg
{
   NetMessage netMessage;

   int result;
   EntryInfo entryInfo;
};


void MkFileRespMsg_init(MkFileRespMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_MkFileResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = MkFileRespMsg_uninit;

   ( (NetMessage*)this)->serializePayload   = _NetMessage_serializeDummy;
   ( (NetMessage*)this)->deserializePayload = MkFileRespMsg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength  = _NetMessage_calcMessageLengthDummy;
}

MkFileRespMsg* MkFileRespMsg_construct(void)
{
   struct MkFileRespMsg* this = os_kmalloc(sizeof(*this) );

   MkFileRespMsg_init(this);

   return this;
}

void MkFileRespMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void MkFileRespMsg_destruct(NetMessage* this)
{
   MkFileRespMsg_uninit( (NetMessage*)this);

   os_kfree(this);
}

int MkFileRespMsg_getResult(MkFileRespMsg* this)
{
   return this->result;
}

const EntryInfo* MkFileRespMsg_getEntryInfo(MkFileRespMsg* this)
{
   return &this->entryInfo;
}

#endif /*MKFILERESPMSG_H_*/

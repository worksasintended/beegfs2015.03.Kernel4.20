#ifndef MKDIRRESPMSG_H_
#define MKDIRRESPMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/EntryInfo.h>

struct MkDirRespMsg;
typedef struct MkDirRespMsg MkDirRespMsg;

static inline void MkDirRespMsg_init(MkDirRespMsg* this);
static inline MkDirRespMsg* MkDirRespMsg_construct(void);
static inline void MkDirRespMsg_uninit(NetMessage* this);
static inline void MkDirRespMsg_destruct(NetMessage* this);

// virtual functions
extern fhgfs_bool MkDirRespMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen);

// getters & setters
static inline int MkDirRespMsg_getResult(MkDirRespMsg* this);
static inline const EntryInfo* MkDirRespMsg_getEntryInfo(MkDirRespMsg* this);


struct MkDirRespMsg
{
   NetMessage netMessage;

   int result;

   // for deserialization
   EntryInfo entryInfo;
};


void MkDirRespMsg_init(MkDirRespMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_MkDirResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = MkDirRespMsg_uninit;

   ( (NetMessage*)this)->serializePayload   = _NetMessage_serializeDummy;
   ( (NetMessage*)this)->deserializePayload = MkDirRespMsg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength  = _NetMessage_calcMessageLengthDummy;
}

MkDirRespMsg* MkDirRespMsg_construct(void)
{
   struct MkDirRespMsg* this = os_kmalloc(sizeof(*this) );

   MkDirRespMsg_init(this);

   return this;
}

void MkDirRespMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void MkDirRespMsg_destruct(NetMessage* this)
{
   MkDirRespMsg_uninit( (NetMessage*)this);

   os_kfree(this);
}

int MkDirRespMsg_getResult(MkDirRespMsg* this)
{
   return this->result;
}

const EntryInfo* MkDirRespMsg_getEntryInfo(MkDirRespMsg* this)
{
   return &this->entryInfo;
}

#endif /*MKDIRRESPMSG_H_*/

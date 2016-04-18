#ifndef STATSTORAGEPATHRESPMSG_H_
#define STATSTORAGEPATHRESPMSG_H_

#include <common/net/message/NetMessage.h>

/**
 * Only supports deserialization. Serialization is not impl'ed.
 */

struct StatStoragePathRespMsg;
typedef struct StatStoragePathRespMsg StatStoragePathRespMsg;

static inline void StatStoragePathRespMsg_init(StatStoragePathRespMsg* this);
static inline StatStoragePathRespMsg* StatStoragePathRespMsg_construct(void);
static inline void StatStoragePathRespMsg_uninit(NetMessage* this);
static inline void StatStoragePathRespMsg_destruct(NetMessage* this);

// virtual functions
extern void StatStoragePathRespMsg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool StatStoragePathRespMsg_deserializePayload(NetMessage* this, const char* buf,
   size_t bufLen);
extern unsigned StatStoragePathRespMsg_calcMessageLength(NetMessage* this);

// getters & setters
static inline int StatStoragePathRespMsg_getResult(StatStoragePathRespMsg* this);
static inline int64_t StatStoragePathRespMsg_getSizeTotal(StatStoragePathRespMsg* this);
static inline int64_t StatStoragePathRespMsg_getSizeFree(StatStoragePathRespMsg* this);
static inline int64_t StatStoragePathRespMsg_getInodesTotal(StatStoragePathRespMsg* this);
static inline int64_t StatStoragePathRespMsg_getInodesFree(StatStoragePathRespMsg* this);


struct StatStoragePathRespMsg
{
   NetMessage netMessage;
   
   int result;
   int64_t sizeTotal;
   int64_t sizeFree;
   int64_t inodesTotal;
   int64_t inodesFree;
};


void StatStoragePathRespMsg_init(StatStoragePathRespMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_StatStoragePathResp);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = StatStoragePathRespMsg_uninit;

   ( (NetMessage*)this)->serializePayload = StatStoragePathRespMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = StatStoragePathRespMsg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = StatStoragePathRespMsg_calcMessageLength;
}
   
StatStoragePathRespMsg* StatStoragePathRespMsg_construct(void)
{
   struct StatStoragePathRespMsg* this = os_kmalloc(sizeof(struct StatStoragePathRespMsg) );
   
   StatStoragePathRespMsg_init(this);
   
   return this;
}

void StatStoragePathRespMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void StatStoragePathRespMsg_destruct(NetMessage* this)
{
   StatStoragePathRespMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


int StatStoragePathRespMsg_getResult(StatStoragePathRespMsg* this)
{
   return this->result;
}

int64_t StatStoragePathRespMsg_getSizeTotal(StatStoragePathRespMsg* this)
{
   return this->sizeTotal;
}

int64_t StatStoragePathRespMsg_getSizeFree(StatStoragePathRespMsg* this)
{
   return this->sizeFree;
}

int64_t StatStoragePathRespMsg_getInodesTotal(StatStoragePathRespMsg* this)
{
   return this->inodesTotal;
}

int64_t StatStoragePathRespMsg_getInodesFree(StatStoragePathRespMsg* this)
{
   return this->inodesFree;
}

#endif /*STATSTORAGEPATHRESPMSG_H_*/

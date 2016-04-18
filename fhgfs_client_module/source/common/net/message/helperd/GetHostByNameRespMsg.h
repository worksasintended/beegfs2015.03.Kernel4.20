#ifndef GETHOSTBYNAMERESPMSG_H_
#define GETHOSTBYNAMERESPMSG_H_

#include <common/net/message/NetMessage.h>


struct GetHostByNameRespMsg;
typedef struct GetHostByNameRespMsg GetHostByNameRespMsg;

static inline void GetHostByNameRespMsg_init(GetHostByNameRespMsg* this);
static inline void GetHostByNameRespMsg_initFromHostAddr(GetHostByNameRespMsg* this,
   const char* hostAddr);
static inline GetHostByNameRespMsg* GetHostByNameRespMsg_construct(void);
static inline GetHostByNameRespMsg* GetHostByNameRespMsg_constructFromHostAddr(
   const char* hostAddr);
static inline void GetHostByNameRespMsg_uninit(NetMessage* this);
static inline void GetHostByNameRespMsg_destruct(NetMessage* this);

// virtual functions
extern void GetHostByNameRespMsg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool GetHostByNameRespMsg_deserializePayload(NetMessage* this,
   const char* buf, size_t bufLen);
extern unsigned GetHostByNameRespMsg_calcMessageLength(NetMessage* this);

// getters & setters
static inline const char* GetHostByNameRespMsg_getHostAddr(GetHostByNameRespMsg* this);


struct GetHostByNameRespMsg
{
   NetMessage netMessage;

   unsigned hostAddrLen;
   const char* hostAddr;
};


void GetHostByNameRespMsg_init(GetHostByNameRespMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_GetHostByNameResp);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = GetHostByNameRespMsg_uninit;

   ( (NetMessage*)this)->serializePayload = GetHostByNameRespMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = GetHostByNameRespMsg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = GetHostByNameRespMsg_calcMessageLength;
}
   
/**
 * @param hostAddr just a reference, so do not free it as long as you use this object!
 */
void GetHostByNameRespMsg_initFromHostAddr(GetHostByNameRespMsg* this,
   const char* hostAddr)
{
   GetHostByNameRespMsg_init(this);
   
   this->hostAddr = hostAddr;
   this->hostAddrLen = os_strlen(hostAddr);
}

GetHostByNameRespMsg* GetHostByNameRespMsg_construct(void)
{
   struct GetHostByNameRespMsg* this = os_kmalloc(sizeof(struct GetHostByNameRespMsg) );
   
   GetHostByNameRespMsg_init(this);
   
   return this;
}

/**
 * @param hostAddr just a reference, so do not free it as long as you use this object!
 */
GetHostByNameRespMsg* GetHostByNameRespMsg_constructFromHostAddr(
   const char* hostAddr)
{
   struct GetHostByNameRespMsg* this = os_kmalloc(sizeof(struct GetHostByNameRespMsg) );
   
   GetHostByNameRespMsg_initFromHostAddr(this, hostAddr);
   
   return this;
}

void GetHostByNameRespMsg_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void GetHostByNameRespMsg_destruct(NetMessage* this)
{
   GetHostByNameRespMsg_uninit(this);
   
   os_kfree(this);
}

const char* GetHostByNameRespMsg_getHostAddr(GetHostByNameRespMsg* this)
{
   return this->hostAddr;
}

#endif /*GETHOSTBYNAMERESPMSG_H_*/

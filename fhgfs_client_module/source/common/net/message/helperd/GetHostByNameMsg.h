#ifndef GETHOSTBYNAMEMSG_H_
#define GETHOSTBYNAMEMSG_H_

#include <common/net/message/NetMessage.h>


struct GetHostByNameMsg;
typedef struct GetHostByNameMsg GetHostByNameMsg;

static inline void GetHostByNameMsg_init(GetHostByNameMsg* this);
static inline void GetHostByNameMsg_initFromHostname(GetHostByNameMsg* this,
   const char* hostname);
static inline GetHostByNameMsg* GetHostByNameMsg_construct(void);
static inline GetHostByNameMsg* GetHostByNameMsg_constructFromHostname(
   const char* hostname);
static inline void GetHostByNameMsg_uninit(NetMessage* this);
static inline void GetHostByNameMsg_destruct(NetMessage* this);

// virtual functions
extern void GetHostByNameMsg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool GetHostByNameMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen);
extern unsigned GetHostByNameMsg_calcMessageLength(NetMessage* this);

// getters & setters
static inline const char* GetHostByNameMsg_getHostname(GetHostByNameMsg* this);


struct GetHostByNameMsg
{
   NetMessage netMessage;

   unsigned hostnameLen;
   const char* hostname;
};


void GetHostByNameMsg_init(GetHostByNameMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_GetHostByName);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = GetHostByNameMsg_uninit;

   ( (NetMessage*)this)->serializePayload = GetHostByNameMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = GetHostByNameMsg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = GetHostByNameMsg_calcMessageLength;
}
   
/**
 * @param hostname just a reference, so do not free it as long as you use this object!
 */
void GetHostByNameMsg_initFromHostname(GetHostByNameMsg* this,
   const char* hostname)
{
   GetHostByNameMsg_init(this);
   
   this->hostname = hostname;
   this->hostnameLen = os_strlen(hostname);
}

GetHostByNameMsg* GetHostByNameMsg_construct(void)
{
   struct GetHostByNameMsg* this = os_kmalloc(sizeof(struct GetHostByNameMsg) );
   
   GetHostByNameMsg_init(this);
   
   return this;
}

/**
 * @param hostname just a reference, so do not free it as long as you use this object!
 */
GetHostByNameMsg* GetHostByNameMsg_constructFromHostname(
   const char* hostname)
{
   struct GetHostByNameMsg* this = os_kmalloc(sizeof(struct GetHostByNameMsg) );
   
   GetHostByNameMsg_initFromHostname(this, hostname);
   
   return this;
}

void GetHostByNameMsg_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void GetHostByNameMsg_destruct(NetMessage* this)
{
   GetHostByNameMsg_uninit(this);
   
   os_kfree(this);
}

const char* GetHostByNameMsg_getHostname(GetHostByNameMsg* this)
{
   return this->hostname;
}

#endif /*GETHOSTBYNAMEMSG_H_*/

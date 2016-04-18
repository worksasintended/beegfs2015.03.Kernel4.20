#ifndef REMOVEXATTRMSG_H_
#define REMOVEXATTRMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/EntryInfo.h>

struct RemoveXAttrMsg;
typedef struct RemoveXAttrMsg RemoveXAttrMsg;

static inline void RemoveXAttrMsg_init(RemoveXAttrMsg* this);
static inline void RemoveXAttrMsg_initFromEntryInfoAndName(RemoveXAttrMsg* this,
      const EntryInfo* entryInfo, const char* name);
static inline RemoveXAttrMsg* RemoveXAttrMsg_construct(void);
static inline RemoveXAttrMsg* RemoveXAttrMsg_constructFromEntryInfoAndName(
      const EntryInfo* entryInfo, const char* name);
static inline void RemoveXAttrMsg_uninit(NetMessage* this);
static inline void RemoveXAttrMsg_destruct(NetMessage* this);

// virtual functions
extern void RemoveXAttrMsg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool RemoveXAttrMsg_deserializePayload(NetMessage* this, const char* buf,
      size_t bufLen);
extern unsigned RemoveXAttrMsg_calcMessageLength(NetMessage* this);

struct RemoveXAttrMsg
{
   NetMessage netMessage;

   const EntryInfo* entryInfoPtr;
   const char* name;
};

void RemoveXAttrMsg_init(RemoveXAttrMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_RemoveXAttr);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = RemoveXAttrMsg_uninit;

   ( (NetMessage*)this)->serializePayload = RemoveXAttrMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = _NetMessage_deserializeDummy;
   ( (NetMessage*)this)->calcMessageLength = RemoveXAttrMsg_calcMessageLength;
}

void RemoveXAttrMsg_initFromEntryInfoAndName(RemoveXAttrMsg* this, const EntryInfo* entryInfo,
      const char* name)
{
   RemoveXAttrMsg_init(this);
   this->entryInfoPtr = entryInfo;
   this->name = name;
}

RemoveXAttrMsg* RemoveXAttrMsg_construct(void)
{
   struct RemoveXAttrMsg* this = os_kmalloc(sizeof(*this) );

   RemoveXAttrMsg_init(this);

   return this;
}

RemoveXAttrMsg* RemoveXAttrMsg_constructFromEntryInfoAndName(const EntryInfo* entryInfo,
      const char* name)
{
   struct RemoveXAttrMsg* this = os_kmalloc(sizeof(*this) );

   RemoveXAttrMsg_initFromEntryInfoAndName(this, entryInfo, name);

   return this;
}

void RemoveXAttrMsg_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void RemoveXAttrMsg_destruct(NetMessage* this)
{
   RemoveXAttrMsg_uninit(this);
   os_kfree(this);
}

#endif /*REMOVEXATTRMSG_H_*/

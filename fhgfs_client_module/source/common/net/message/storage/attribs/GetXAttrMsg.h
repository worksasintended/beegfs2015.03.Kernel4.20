#ifndef GETXATTRMSG_H_
#define GETXATTRMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/EntryInfo.h>

struct GetXAttrMsg;
typedef struct GetXAttrMsg GetXAttrMsg;

static inline void GetXAttrMsg_init(GetXAttrMsg* this);
static inline void GetXAttrMsg_initFromEntryInfoNameAndSize(GetXAttrMsg* this,
      const EntryInfo* entryInfo, const char* name, ssize_t size);
static inline GetXAttrMsg* GetXAttrMsg_construct(void);
static inline GetXAttrMsg* GetXAttrMsg_constructFromEntryInfoNameAndSize(const EntryInfo* entryInfo,
      const char* name, ssize_t size);
static inline void GetXAttrMsg_uninit(NetMessage* this);
static inline void GetXAttrMsg_destruct(NetMessage* this);

// virtual functions
extern void GetXAttrMsg_serializePayload(NetMessage* this, char* buf);
extern unsigned GetXAttrMsg_calcMessageLength(NetMessage* this);


struct GetXAttrMsg
{
   NetMessage netMessage;

   const EntryInfo* entryInfoPtr;

   const char* name;
   int size;
};

void GetXAttrMsg_init(GetXAttrMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_GetXAttr);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = GetXAttrMsg_uninit;

   ( (NetMessage*)this)->serializePayload = GetXAttrMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = _NetMessage_deserializeDummy;
   ( (NetMessage*)this)->calcMessageLength = GetXAttrMsg_calcMessageLength;
}

void GetXAttrMsg_initFromEntryInfoNameAndSize(GetXAttrMsg* this, const EntryInfo* entryInfo,
      const char* name, ssize_t size)
{
   GetXAttrMsg_init(this);
   this->entryInfoPtr = entryInfo;
   this->name = name;
   this->size = size;
}

GetXAttrMsg* GetXAttrMsg_construct(void)
{
   struct GetXAttrMsg* this = os_kmalloc(sizeof(*this) );

   GetXAttrMsg_init(this);

   return this;
}

GetXAttrMsg* GetXAttrMsg_constructFromEntryInfoNameAndSize(const EntryInfo* entryInfo,
      const char* name, ssize_t size)
{
   struct GetXAttrMsg* this = os_kmalloc(sizeof(*this) );

   GetXAttrMsg_initFromEntryInfoNameAndSize(this, entryInfo, name, size);

   return this;
}

void GetXAttrMsg_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void GetXAttrMsg_destruct(NetMessage* this)
{
   GetXAttrMsg_uninit(this);
   os_kfree(this);
}


#endif /*GETXATTRMSG_H_*/

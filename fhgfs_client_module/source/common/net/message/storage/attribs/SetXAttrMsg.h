#ifndef SETXATTRMSG_H_
#define SETXATTRMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/EntryInfo.h>

struct SetXAttrMsg;
typedef struct SetXAttrMsg SetXAttrMsg;

static inline void SetXAttrMsg_init(SetXAttrMsg* this);
static inline void SetXAttrMsg_initFromEntryInfoNameValueAndSize(SetXAttrMsg* this,
      const EntryInfo* entryInfo, const char* name, const char* value, const size_t size,
      const int flags);
static inline SetXAttrMsg* SetXAttrMsg_construct(void);
static inline SetXAttrMsg* SetXAttrMsg_constructFromEntryInfoNameValueAndSize(
      const EntryInfo* entryInfo, const char* name, const char* value, size_t size,
      const int flags);
static inline void SetXAttrMsg_uninit(NetMessage* this);
static inline void SetXAttrMsg_destruct(NetMessage* this);

// virtual functions
extern void SetXAttrMsg_serializePayload(NetMessage* this, char* buf);
extern unsigned SetXAttrMsg_calcMessageLength(NetMessage* this);

struct SetXAttrMsg
{
   NetMessage netMessage;

   const EntryInfo* entryInfoPtr;

   const char* name;
   const char* value;
   size_t size;
   int flags;
};

void SetXAttrMsg_init(SetXAttrMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_SetXAttr);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = SetXAttrMsg_uninit;

   ( (NetMessage*)this)->serializePayload = SetXAttrMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = _NetMessage_deserializeDummy;
   ( (NetMessage*)this)->calcMessageLength = SetXAttrMsg_calcMessageLength;
}

void SetXAttrMsg_initFromEntryInfoNameValueAndSize(SetXAttrMsg* this, const EntryInfo* entryInfo,
      const char* name, const char* value, const size_t size, const int flags)
{
   SetXAttrMsg_init(this);
   this->entryInfoPtr = entryInfo;
   this->name = name;
   this->value = value;
   this->size = size;
   this->flags = flags;
}

SetXAttrMsg* SetXAttrMsg_construct(void)
{
   struct SetXAttrMsg* this = os_kmalloc(sizeof(*this) );

   SetXAttrMsg_init(this);

   return this;
}

SetXAttrMsg* SetXAttrMsg_constructFromEntryInfoNameValueAndSize(const EntryInfo* entryInfo,
      const char* name, const char* value, const size_t size, const int flags)
{
   struct SetXAttrMsg* this = os_kmalloc(sizeof(*this) );

   SetXAttrMsg_initFromEntryInfoNameValueAndSize(this, entryInfo, name, value, size, flags);

   return this;
}

void SetXAttrMsg_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void SetXAttrMsg_destruct(NetMessage* this)
{
   SetXAttrMsg_uninit(this);
   os_kfree(this);
}

#endif /*SETXATTRMSG_H_*/

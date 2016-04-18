#ifndef LISTXATTRMSG_H_
#define LISTXATTRMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/EntryInfo.h>

struct ListXAttrMsg;
typedef struct ListXAttrMsg ListXAttrMsg;

static inline void ListXAttrMsg_init(ListXAttrMsg* this);
static inline void ListXAttrMsg_initFromEntryInfoAndSize(ListXAttrMsg* this,
      const EntryInfo* entryInfo, ssize_t size);
static inline ListXAttrMsg* ListXAttrMsg_construct(void);
static inline ListXAttrMsg* ListXAttrMsg_constructFromEntryInfoAndSize(const EntryInfo* entryInfo,
      ssize_t size);
static inline void ListXAttrMsg_uninit(NetMessage* this);
static inline void ListXAttrMsg_destruct(NetMessage* this);

// virtual functions
extern void ListXAttrMsg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool ListXAttrMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen);
extern unsigned ListXAttrMsg_calcMessageLength(NetMessage* this);


struct ListXAttrMsg
{
   NetMessage netMessage;

   const EntryInfo* entryInfoPtr; // not owned by this object
   int size; // buffer size for the list buffer of the listxattr call
};

void ListXAttrMsg_init(ListXAttrMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_ListXAttr);


   // assign virtual functions
   ( (NetMessage*)this)->uninit = ListXAttrMsg_uninit;

   ( (NetMessage*)this)->serializePayload = ListXAttrMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = _NetMessage_deserializeDummy;
   ( (NetMessage*)this)->calcMessageLength = ListXAttrMsg_calcMessageLength;
}

/**
 * @param entryID just a reference, so do not free it as long as you use this object!
 */
void ListXAttrMsg_initFromEntryInfoAndSize(ListXAttrMsg* this, const EntryInfo* entryInfo,
      ssize_t size)
{
   ListXAttrMsg_init(this);
   this->entryInfoPtr = entryInfo;
   this->size = size;
}

ListXAttrMsg* ListXAttrMsg_construct(void)
{
   struct ListXAttrMsg* this = os_kmalloc(sizeof(*this) );

   ListXAttrMsg_init(this);

   return this;
}

/**
 * @param entryID just a reference, so do not free it as long as you use this object!
 */
ListXAttrMsg* ListXAttrMsg_constructFromEntryInfoAndSize(const EntryInfo* entryInfo, ssize_t size)
{
   struct ListXAttrMsg* this = os_kmalloc(sizeof(*this) );

   ListXAttrMsg_initFromEntryInfoAndSize(this, entryInfo, size);

   return this;
}

void ListXAttrMsg_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void ListXAttrMsg_destruct(NetMessage* this)
{
   ListXAttrMsg_uninit(this);
   os_kfree(this);
}

#endif /*LISTXATTRMSG_H_*/

#ifndef LISTDIRFROMOFFSETMSG_H_
#define LISTDIRFROMOFFSETMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/Path.h>
#include <common/storage/EntryInfo.h>

/**
 * This message supports only serialization. (deserialization not implemented)
 */

struct ListDirFromOffsetMsg;
typedef struct ListDirFromOffsetMsg ListDirFromOffsetMsg;

static inline void ListDirFromOffsetMsg_init(ListDirFromOffsetMsg* this);
static inline void ListDirFromOffsetMsg_initFromEntryInfo(ListDirFromOffsetMsg* this,
   const EntryInfo* entryInfo, int64_t serverOffset, unsigned maxOutNames, fhgfs_bool filterDots);
static inline ListDirFromOffsetMsg* ListDirFromOffsetMsg_construct(void);
static inline ListDirFromOffsetMsg* ListDirFromOffsetMsg_constructFromEntryInfo
(EntryInfo *entryInfo, int64_t serverOffset, unsigned maxOutNames, fhgfs_bool filterDots);
static inline void ListDirFromOffsetMsg_uninit(NetMessage* this);
static inline void ListDirFromOffsetMsg_destruct(NetMessage* this);

// virtual functions
extern void ListDirFromOffsetMsg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool ListDirFromOffsetMsg_deserializePayload(NetMessage* this, const char* buf,
   size_t bufLen);
extern unsigned ListDirFromOffsetMsg_calcMessageLength(NetMessage* this);



struct ListDirFromOffsetMsg
{
   NetMessage netMessage;

   int64_t serverOffset;
   unsigned maxOutNames;
   fhgfs_bool filterDots;

   // for serialization
   const EntryInfo* entryInfoPtr; // not owned by this object!

   // for deserialization
   EntryInfo entryInfo;
};


void ListDirFromOffsetMsg_init(ListDirFromOffsetMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_ListDirFromOffset);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = ListDirFromOffsetMsg_uninit;

   ( (NetMessage*)this)->serializePayload = ListDirFromOffsetMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = _NetMessage_deserializeDummy;
   ( (NetMessage*)this)->calcMessageLength = ListDirFromOffsetMsg_calcMessageLength;
}
   
/**
 * @param entryInfo just a reference, so do not free it as long as you use this object!
 * @param filterDots true if you don't want "." and ".." in the result list.
 */
void ListDirFromOffsetMsg_initFromEntryInfo(ListDirFromOffsetMsg* this, const EntryInfo* entryInfo,
   int64_t serverOffset, unsigned maxOutNames, fhgfs_bool filterDots)
{
   ListDirFromOffsetMsg_init(this);
   
   this->entryInfoPtr = entryInfo;

   this->serverOffset = serverOffset;

   this->maxOutNames = maxOutNames;

   this->filterDots = filterDots;
}

ListDirFromOffsetMsg* ListDirFromOffsetMsg_construct(void)
{
   struct ListDirFromOffsetMsg* this = os_kmalloc(sizeof(*this) );
   
   if(likely(this) )
      ListDirFromOffsetMsg_init(this);
   
   return this;
}

/**
 * @param path just a reference, so do not free it as long as you use this object!
 */
ListDirFromOffsetMsg* ListDirFromOffsetMsg_constructFromEntryInfo(EntryInfo* entryInfo,
   int64_t serverOffset, unsigned maxOutNames, fhgfs_bool filterDots)
{
   struct ListDirFromOffsetMsg* this = os_kmalloc(sizeof(*this) );
   
   if(likely(this) )
      ListDirFromOffsetMsg_initFromEntryInfo(this, entryInfo, serverOffset, maxOutNames,
         filterDots);
   
   return this;
}

void ListDirFromOffsetMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void ListDirFromOffsetMsg_destruct(NetMessage* this)
{
   ListDirFromOffsetMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}

#endif /*LISTDIRFROMOFFSETMSG_H_*/

#ifndef REFRESHENTRYINFO_H_
#define REFRESHENTRYINFO_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/EntryInfo.h>


struct RefreshEntryInfoMsg;
typedef struct RefreshEntryInfoMsg RefreshEntryInfoMsg;

static inline void RefreshEntryInfoMsg_init(RefreshEntryInfoMsg* this);
static inline void RefreshEntryInfoMsg_initFromEntryInfo(RefreshEntryInfoMsg* this, const EntryInfo* entryInfo);
static inline RefreshEntryInfoMsg* RefreshEntryInfoMsg_construct(void);
static inline RefreshEntryInfoMsg* RefreshEntryInfoMsg_constructFromEntryInfo(const EntryInfo* entryInfo);
static inline void RefreshEntryInfoMsg_uninit(NetMessage* this);
static inline void RefreshEntryInfoMsg_destruct(NetMessage* this);

// virtual functions
extern void RefreshEntryInfoMsg_serializePayload(NetMessage* this, char* buf);
extern unsigned RefreshEntryInfoMsg_calcMessageLength(NetMessage* this);


struct RefreshEntryInfoMsg
{
   NetMessage netMessage;

   const EntryInfo* entryInfoPtr; // not owned by this object
};


void RefreshEntryInfoMsg_init(RefreshEntryInfoMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_RefreshEntryInfo);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = RefreshEntryInfoMsg_uninit;

   ( (NetMessage*)this)->serializePayload    = RefreshEntryInfoMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload  = _NetMessage_deserializeDummy;
   ( (NetMessage*)this)->calcMessageLength   = RefreshEntryInfoMsg_calcMessageLength;
}

/**
 * @param entryInfo just a reference, so do not free it as long as you use this object!
 */
void RefreshEntryInfoMsg_initFromEntryInfo(RefreshEntryInfoMsg* this, const EntryInfo* entryInfo)
{
   RefreshEntryInfoMsg_init(this);

   this->entryInfoPtr = entryInfo;
}

RefreshEntryInfoMsg* RefreshEntryInfoMsg_construct(void)
{
   struct RefreshEntryInfoMsg* this = os_kmalloc(sizeof(*this) );

   RefreshEntryInfoMsg_init(this);

   return this;
}

/**
 * @param entryID just a reference, so do not free it as long as you use this object!
 */
RefreshEntryInfoMsg* RefreshEntryInfoMsg_constructFromEntryInfo(const EntryInfo* entryInfo)
{
   struct RefreshEntryInfoMsg* this = os_kmalloc(sizeof(*this) );

   RefreshEntryInfoMsg_initFromEntryInfo(this, entryInfo);

   return this;
}

void RefreshEntryInfoMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void RefreshEntryInfoMsg_destruct(NetMessage* this)
{
   RefreshEntryInfoMsg_uninit( (NetMessage*)this);

   os_kfree(this);
}


#endif /*REFRESHENTRYINFO_H_*/

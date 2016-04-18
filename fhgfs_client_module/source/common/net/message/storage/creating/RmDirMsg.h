#ifndef RMDIRMSG_H_
#define RMDIRMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/Path.h>
#include <common/storage/EntryInfo.h>


struct RmDirMsg;
typedef struct RmDirMsg RmDirMsg;

static inline void RmDirMsg_init(RmDirMsg* this);
static inline void RmDirMsg_initFromEntryInfo(RmDirMsg* this, const EntryInfo* parentInfo,
   const char* delDirName);
static inline RmDirMsg* RmDirMsg_construct(void);
static inline RmDirMsg* RmDirMsg_constructFromEntryInfo(const EntryInfo* parentInfo,
   const char* delDirName);
static inline void RmDirMsg_uninit(NetMessage* this);
static inline void RmDirMsg_destruct(NetMessage* this);

// virtual functions
extern void RmDirMsg_serializePayload(NetMessage* this, char* buf);
extern unsigned RmDirMsg_calcMessageLength(NetMessage* this);

// inliners   

struct RmDirMsg
{
   NetMessage netMessage;
   
   // for serialization
   const EntryInfo* parentInfo; // not owned by this object!
   const char* delDirName;      // not owned by this object!
   unsigned delDirNameLen;

   // for deserialization
};


void RmDirMsg_init(RmDirMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_RmDir);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = RmDirMsg_uninit;

   ( (NetMessage*)this)->serializePayload = RmDirMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = _NetMessage_deserializeDummy;
   ( (NetMessage*)this)->calcMessageLength = RmDirMsg_calcMessageLength;
}
   
/**
 * @param entryInfo  just a reference, so do not free it as long as you use this object!
 * @param delDirName just a reference, so do not free it as long as you use this object!
 */
void RmDirMsg_initFromEntryInfo(RmDirMsg* this, const EntryInfo* parentInfo, const char* delDirName)
{
   RmDirMsg_init(this);
   
   this->parentInfo = parentInfo;

   this->delDirName = delDirName;
   this->delDirNameLen = os_strlen(delDirName);
}

RmDirMsg* RmDirMsg_construct(void)
{
   struct RmDirMsg* this = os_kmalloc(sizeof(struct RmDirMsg) );
   
   RmDirMsg_init(this);
   
   return this;
}

/**
 * @param path just a reference, so do not free it as long as you use this object!
 */
RmDirMsg* RmDirMsg_constructFromEntryInfo(const EntryInfo* parentInfo, const char* delDirName)
{
   struct RmDirMsg* this = os_kmalloc(sizeof(struct RmDirMsg) );
   
   RmDirMsg_initFromEntryInfo(this, parentInfo, delDirName);
   
   return this;
}

void RmDirMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void RmDirMsg_destruct(NetMessage* this)
{
   RmDirMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


#endif /*RMDIRMSG_H_*/

#ifndef STATMSG_H_
#define STATMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/Path.h>
#include <common/toolkit/MetadataTk.h>

//#define STATMSG_COMPAT_FLAG_GET_PARENTOWNERNODEID     1 /* deprecated */
#define STATMSG_COMPAT_FLAG_GET_PARENTINFO            2 /* caller wants to have parentOwnerNodeID
                                                         * and parentEntryID */


struct StatMsg;
typedef struct StatMsg StatMsg;

static inline void StatMsg_init(StatMsg* this);
static inline void StatMsg_initFromEntryInfo(StatMsg* this, const EntryInfo* entryInfo);
static inline StatMsg* StatMsg_construct(void);
static inline StatMsg* StatMsg_constructFromEntryInfo(const EntryInfo* entryInfo);
static inline void StatMsg_uninit(NetMessage* this);
static inline void StatMsg_destruct(NetMessage* this);
static inline void StatMsg_addParentInfoRequest(StatMsg* this);

// virtual functions
extern void StatMsg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool StatMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen);
extern unsigned StatMsg_calcMessageLength(NetMessage* this);


struct StatMsg
{
   NetMessage netMessage;

   const EntryInfo* entryInfoPtr; // not owed by this object
};


void StatMsg_init(StatMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_Stat);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = StatMsg_uninit;

   ( (NetMessage*)this)->serializePayload = StatMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = _NetMessage_deserializeDummy;
   ( (NetMessage*)this)->calcMessageLength = StatMsg_calcMessageLength;
}
   
/**
 * @param entryInfo just a reference, so do not free it as long as you use this object!
 */
void StatMsg_initFromEntryInfo(StatMsg* this, const EntryInfo* entryInfo)
{
   StatMsg_init(this);
   
   this->entryInfoPtr = entryInfo;
}

StatMsg* StatMsg_construct(void)
{
   struct StatMsg* this = os_kmalloc(sizeof(*this) );
   
   StatMsg_init(this);
   
   return this;
}

/**
 * @param entryID just a reference, so do not free it as long as you use this object!
 */
StatMsg* StatMsg_constructFromEntryInfo(const EntryInfo* entryInfo)
{
   struct StatMsg* this = os_kmalloc(sizeof(*this) );
   
   StatMsg_initFromEntryInfo(this, entryInfo);
   
   return this;
}

void StatMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void StatMsg_destruct(NetMessage* this)
{
   StatMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}

void StatMsg_addParentInfoRequest(StatMsg* this)
{
   NetMessage* netMsg =  (NetMessage*) this;

   NetMessage_addMsgHeaderCompatFeatureFlag(netMsg, STATMSG_COMPAT_FLAG_GET_PARENTINFO);
}


#endif /*STATMSG_H_*/

#ifndef HARDLINKMSG_H_
#define HARDLINKMSG_H_

#include <common/storage/EntryInfo.h>
#include <common/net/message/NetMessage.h>
#include <common/storage/Path.h>

/* This is the HardlinkMsg class, deserialization is not implemented, as the client is not
 * supposed to deserialize the message. There is also only a basic constructor, if a full
 * constructor is needed, it can be easily added later on */

struct HardlinkMsg;
typedef struct HardlinkMsg HardlinkMsg;

static inline void HardlinkMsg_init(HardlinkMsg* this);
static inline void HardlinkMsg_initFromEntryInfo(HardlinkMsg* this, EntryInfo* fromDirInfo,
   const char* fromName, unsigned fromLen, EntryInfo* fromInfo, EntryInfo* toDirInfo,
   const char* toName, unsigned toNameLen);

static inline HardlinkMsg* HardlinkMsg_construct(void);
static inline void HardlinkMsg_uninit(NetMessage* this);
static inline void HardlinkMsg_destruct(NetMessage* this);

// virtual functions
extern void HardlinkMsg_serializePayload(NetMessage* this, char* buf);
extern unsigned HardlinkMsg_calcMessageLength(NetMessage* this);


struct HardlinkMsg
{
   NetMessage netMessage;
   
   // for serialization

   const char* fromName;    // not owned by this object!
   EntryInfo* fromInfo;     // not owned by this object!

   unsigned fromNameLen;
   EntryInfo* fromDirInfo; // not owned by this object!

   const char* toName;    // not owned by this object!
   unsigned toNameLen;
   EntryInfo* toDirInfo;  // not owned by this object!
};


void HardlinkMsg_init(HardlinkMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_Hardlink);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = HardlinkMsg_uninit;

   ( (NetMessage*)this)->serializePayload   = HardlinkMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = _NetMessage_deserializeDummy;
   ( (NetMessage*)this)->calcMessageLength  = HardlinkMsg_calcMessageLength;
}
   
/**
 * @param fromName just a reference, so do not free it as long as you use this object!
 * @param fromDirInfo just a reference, so do not free it as long as you use this object!
 * @param toName just a reference, so do not free it as long as you use this object!
 * @param toDirInfo just a reference, so do not free it as long as you use this object!
 */
void HardlinkMsg_initFromEntryInfo(HardlinkMsg* this, EntryInfo* fromDirInfo,
   const char* fromName, unsigned fromLen, EntryInfo* fromInfo, EntryInfo* toDirInfo,
   const char* toName, unsigned toNameLen)
{
   HardlinkMsg_init(this);
   
   this->fromName     = fromName;
   this->fromInfo     = fromInfo;

   this->fromNameLen  = fromLen;
   this->fromDirInfo  = fromDirInfo;

   this->toName       = toName;
   this->toNameLen    = toNameLen;
   this->toDirInfo    = toDirInfo;
}

HardlinkMsg* HardlinkMsg_construct(void)
{
   struct HardlinkMsg* this = os_kmalloc(sizeof(struct HardlinkMsg) );
   
   HardlinkMsg_init(this);
   
   return this;
}

void HardlinkMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void HardlinkMsg_destruct(NetMessage* this)
{
   HardlinkMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


#endif /*HARDLINKMSG_H_*/

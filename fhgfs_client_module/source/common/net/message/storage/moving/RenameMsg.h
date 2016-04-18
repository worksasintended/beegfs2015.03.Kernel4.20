#ifndef RENAMEMSG_H_
#define RENAMEMSG_H_

#include <common/storage/EntryInfo.h>
#include <common/net/message/NetMessage.h>
#include <common/storage/Path.h>

/* This is the RenameMsg class, deserialization is not implemented, as the client is not supposed to
   deserialize the message. There is also only a basic constructor, if a full constructor is
   needed, it can be easily added later on */

struct RenameMsg;
typedef struct RenameMsg RenameMsg;

static inline void RenameMsg_init(RenameMsg* this);
static inline void RenameMsg_initFromEntryInfo(RenameMsg* this, const char* oldName,
   unsigned oldLen, DirEntryType entryType, EntryInfo* fromDirInfo, const char* newName,
   unsigned newLen, EntryInfo* toDirInfo);

static inline RenameMsg* RenameMsg_construct(void);
static inline void RenameMsg_uninit(NetMessage* this);
static inline void RenameMsg_destruct(NetMessage* this);

// virtual functions
extern void RenameMsg_serializePayload(NetMessage* this, char* buf);
extern unsigned RenameMsg_calcMessageLength(NetMessage* this);


struct RenameMsg
{
   NetMessage netMessage;
   
   // for serialization

   const char* oldName;    // not owned by this object!
   unsigned oldNameLen;
   DirEntryType entryType;
   EntryInfo* fromDirInfo; // not owned by this object!

   const char* newName;    // not owned by this object!
   unsigned newNameLen;
   EntryInfo* toDirInfo;  // not owned by this object!
};


void RenameMsg_init(RenameMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_Rename);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = RenameMsg_uninit;

   ( (NetMessage*)this)->serializePayload   = RenameMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = _NetMessage_deserializeDummy;
   ( (NetMessage*)this)->calcMessageLength  = RenameMsg_calcMessageLength;
}
   
/**
 * @param oldName just a reference, so do not free it as long as you use this object!
 * @param fromDirInfo just a reference, so do not free it as long as you use this object!
 * @param newName just a reference, so do not free it as long as you use this object!
 * @param toDirInfo just a reference, so do not free it as long as you use this object!
 */
void RenameMsg_initFromEntryInfo(RenameMsg* this, const char* oldName, unsigned oldLen,
   DirEntryType entryType, EntryInfo* fromDirInfo, const char* newName, unsigned newLen,
   EntryInfo* toDirInfo)
{
   RenameMsg_init(this);
   
   this->oldName     = oldName;
   this->oldNameLen  = oldLen;
   this->entryType   = entryType;
   this->fromDirInfo = fromDirInfo;

   this->newName     = newName;
   this->newNameLen  = newLen;
   this->toDirInfo   = toDirInfo;
}

RenameMsg* RenameMsg_construct(void)
{
   struct RenameMsg* this = os_kmalloc(sizeof(struct RenameMsg) );
   
   RenameMsg_init(this);
   
   return this;
}

void RenameMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void RenameMsg_destruct(NetMessage* this)
{
   RenameMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


#endif /*RENAMEMSG_H_*/

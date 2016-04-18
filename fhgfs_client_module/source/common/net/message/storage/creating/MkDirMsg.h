#ifndef MKDIRMSG_H_
#define MKDIRMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/Path.h>
#include <net/filesystem/FhgfsOpsRemoting.h>


/**
 * this message supports only serialization, deserialization is not implemented.
 */

#define MKDIRMSG_FLAG_UMASK 2 /* Message contains separate umask data */


struct MkDirMsg;
typedef struct MkDirMsg MkDirMsg;

static inline void MkDirMsg_init(MkDirMsg* this);
static inline void MkDirMsg_initFromEntryInfo(MkDirMsg* this, const EntryInfo* parentInfo,
   struct CreateInfo* createInfo);
static inline MkDirMsg* MkDirMsg_construct(void);
static inline void MkDirMsg_uninit(NetMessage* this);
static inline void MkDirMsg_destruct(NetMessage* this);

// virtual functions
extern void MkDirMsg_serializePayload(NetMessage* this, char* buf);
extern unsigned MkDirMsg_calcMessageLength(NetMessage* this);



struct MkDirMsg
{
   NetMessage netMessage;

   unsigned userID;
   unsigned groupID;
   int mode;
   int umask;

   // for serialization
   const EntryInfo* parentInfo; // not owned by this object!

   const char* newDirName;
   unsigned newDirNameLen;

   UInt16List* preferredNodes; // not owned by this object!
};


void MkDirMsg_init(MkDirMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_MkDir);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = MkDirMsg_uninit;

   ( (NetMessage*)this)->serializePayload = MkDirMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = _NetMessage_deserializeDummy;
   ( (NetMessage*)this)->calcMessageLength = MkDirMsg_calcMessageLength;
}
   
/**
 * @param path just a reference, so do not free it as long as you use this object!
 */
void MkDirMsg_initFromEntryInfo(MkDirMsg* this, const EntryInfo* parentInfo,
   struct CreateInfo* createInfo)
{
   MkDirMsg_init(this);
   
   this->parentInfo = parentInfo;
   this->newDirName = createInfo->entryName;
   this->newDirNameLen = os_strlen(createInfo->entryName);

   this->userID  = createInfo->userID;
   this->groupID = createInfo->groupID;
   this->mode    = createInfo->mode;
   this->umask   = createInfo->umask;
   this->preferredNodes = createInfo->preferredMetaTargets;

   if (createInfo->umask != -1)
      NetMessage_addMsgHeaderFeatureFlag(&this->netMessage, MKDIRMSG_FLAG_UMASK);
}

MkDirMsg* MkDirMsg_construct(void)
{
   struct MkDirMsg* this = os_kmalloc(sizeof(*this) );
   
   MkDirMsg_init(this);
   
   return this;
}

void MkDirMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void MkDirMsg_destruct(NetMessage* this)
{
   MkDirMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}



#endif /*MKDIRMSG_H_*/

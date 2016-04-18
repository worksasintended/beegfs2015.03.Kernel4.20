#ifndef MKFILEMSG_H_
#define MKFILEMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/Path.h>
#include <net/filesystem/FhgfsOpsRemoting.h>


#define MKFILEMSG_FLAG_STRIPEHINTS        1 /* msg contains extra stripe hints */
#define MKFILEMSG_FLAG_UMASK              2 /* msg contains separate umask data */


struct MkFileMsg;
typedef struct MkFileMsg MkFileMsg;

static inline void MkFileMsg_init(MkFileMsg* this);
static inline void MkFileMsg_initFromEntryInfo(MkFileMsg* this, const EntryInfo* parentInfo,
   struct CreateInfo* createInfo);

static inline MkFileMsg* MkFileMsg_construct(void);
static inline MkFileMsg* MkFileMsg_constructFromEntryInfo(const EntryInfo* parentInfo,
   struct CreateInfo* createInfo);
static inline void MkFileMsg_uninit(NetMessage* this);
static inline void MkFileMsg_destruct(NetMessage* this);

// virtual functions
extern void MkFileMsg_serializePayload(NetMessage* this, char* buf);
extern unsigned MkFileMsg_calcMessageLength(NetMessage* this);

// getters & setters
static inline void MkFileMsg_setStripeHints(MkFileMsg* this,
   unsigned numtargets, unsigned chunksize);


/*
 * note: this message supports serialization only, deserialization is not implemented
 */
struct MkFileMsg
{
   NetMessage netMessage;
   
   unsigned userID;
   unsigned groupID;
   int mode;
   int umask;
   
   unsigned numtargets;
   unsigned chunksize;

   // for serialization
   const EntryInfo* parentInfoPtr;
   const char* newFileName;
   unsigned newFileNameLen;
   UInt16List* preferredTargets; // not owned by this object!

   // for deserialization
   EntryInfo entryInfo;
   unsigned prefNodesElemNum;
   const char* prefNodesListStart;
   unsigned prefNodesBufLen;
};


void MkFileMsg_init(MkFileMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_MkFile);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = MkFileMsg_uninit;

   ( (NetMessage*)this)->serializePayload   = MkFileMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = _NetMessage_deserializeDummy;
   ( (NetMessage*)this)->calcMessageLength  = MkFileMsg_calcMessageLength;
}
   
/**
 * @param parentInfo just a reference, so do not free it as long as you use this object!
 */
void MkFileMsg_initFromEntryInfo(MkFileMsg* this, const EntryInfo* parentInfo,
   struct CreateInfo* createInfo)
{
   MkFileMsg_init(this);
   
   this->parentInfoPtr    = parentInfo;
   this->newFileName      = createInfo->entryName;
   this->newFileNameLen   = os_strlen(createInfo->entryName);
   this->userID           = createInfo->userID;
   this->groupID          = createInfo->groupID;
   this->mode             = createInfo->mode;
   this->umask            = createInfo->umask;
   this->preferredTargets = createInfo->preferredStorageTargets;

   if (createInfo->umask != -1)
      NetMessage_addMsgHeaderFeatureFlag(&this->netMessage, MKFILEMSG_FLAG_UMASK);
}

MkFileMsg* MkFileMsg_construct(void)
{
   struct MkFileMsg* this = os_kmalloc(sizeof(struct MkFileMsg) );
   
   if(likely(this) )
      MkFileMsg_init(this);
   
   return this;
}

/**
 * @param path just a reference, so do not free it as long as you use this object!
 */
MkFileMsg* MkFileMsg_constructFromEntryInfo(const EntryInfo* entryInfo,
   struct CreateInfo* createInfo)
{
   struct MkFileMsg* this = os_kmalloc(sizeof(struct MkFileMsg) );
   
   MkFileMsg_initFromEntryInfo(this, entryInfo, createInfo);
   
   return this;
}

void MkFileMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void MkFileMsg_destruct(NetMessage* this)
{
   MkFileMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}

/**
 * Note: Adds MKFILEMSG_FLAG_STRIPEHINTS.
 */
void MkFileMsg_setStripeHints(MkFileMsg* this, unsigned numtargets, unsigned chunksize)
{
   NetMessage_addMsgHeaderFeatureFlag( (NetMessage*)this, MKFILEMSG_FLAG_STRIPEHINTS);

   this->numtargets = numtargets;
   this->chunksize = chunksize;
}

#endif /*MKFILEMSG_H_*/

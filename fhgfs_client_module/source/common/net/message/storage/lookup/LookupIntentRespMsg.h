#ifndef LOOKUPINTENTRESPMSG_H_
#define LOOKUPINTENTRESPMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/StorageErrors.h>
#include <common/storage/striping/StripePattern.h>
#include <common/storage/StatData.h>
#include <common/storage/StorageDefinitions.h>
#include <common/storage/PathInfo.h>
#include <common/storage/EntryInfo.h>

/**
 * This message supports only deserialization; serialization is not implemented!
 */


/* Keep these flags in sync with the userspace msg flags:
   fhgfs_common/source/common/net/message/storage/LookupIntentRespMsg.h */

#define LOOKUPINTENTRESPMSG_FLAG_REVALIDATE         1 /* revalidate entry, cancel if invalid */
#define LOOKUPINTENTRESPMSG_FLAG_CREATE             2 /* create file response */
#define LOOKUPINTENTRESPMSG_FLAG_OPEN               4 /* open file response */
#define LOOKUPINTENTRESPMSG_FLAG_STAT               8 /* stat file response */


struct LookupIntentRespMsg;
typedef struct LookupIntentRespMsg LookupIntentRespMsg;


static inline void LookupIntentRespMsg_init(LookupIntentRespMsg* this);
static inline LookupIntentRespMsg* LookupIntentRespMsg_construct(void);
static inline void LookupIntentRespMsg_uninit(NetMessage* this);
static inline void LookupIntentRespMsg_destruct(NetMessage* this);

// virtual functions
extern fhgfs_bool LookupIntentRespMsg_deserializePayload(NetMessage* this, const char* buf,
   size_t bufLen);

// getters & setters
static inline int LookupIntentRespMsg_getResponseFlags(LookupIntentRespMsg* this);

static inline FhgfsOpsErr LookupIntentRespMsg_getLookupResult(LookupIntentRespMsg* this);
static inline EntryInfo* LookupIntentRespMsg_getEntryInfo(LookupIntentRespMsg* this);

static inline FhgfsOpsErr LookupIntentRespMsg_getRevalidateResult(LookupIntentRespMsg* this);

static inline FhgfsOpsErr LookupIntentRespMsg_getOpenResult(LookupIntentRespMsg* this);
static inline const char* LookupIntentRespMsg_getFileHandleID(LookupIntentRespMsg* this);

static inline FhgfsOpsErr LookupIntentRespMsg_getCreateResult(LookupIntentRespMsg* this);

static inline FhgfsOpsErr LookupIntentRespMsg_getStatResult(LookupIntentRespMsg* this);
static inline StatData* LookupIntentRespMsg_getStatData(LookupIntentRespMsg* this);

static inline void LookupIntentRespMsg_toEntryInfo(LookupIntentRespMsg* this, EntryInfo*
   outEntryInfo);
static inline void LookupIntentRespMsg_toPathInfo(LookupIntentRespMsg* this, PathInfo* outPathInfo);

struct LookupIntentRespMsg
{
   NetMessage netMessage;

   int responseFlags; // combination of LOOKUPINTENTRESPMSG_FLAG_...

   int lookupResult;

   EntryInfo entryInfo; // only deserialized if either lookup or create was successful

   int statResult;
   StatData statData;

   int revalidateResult; // FhgfsOpsErr_SUCCESS if still valid, any other value otherwise

   int createResult; // FhgfsOpsErr_...

   int openResult;
   unsigned fileHandleIDLen;
   const char* fileHandleID;

   // for deserialization
   struct StripePatternHeader patternHeader; // (open file data)
   const char* patternStart; // (open file data)
   PathInfo pathInfo;
};


void LookupIntentRespMsg_init(LookupIntentRespMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_LookupIntentResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = LookupIntentRespMsg_uninit;

   ( (NetMessage*)this)->serializePayload   = _NetMessage_serializeDummy;
   ( (NetMessage*)this)->deserializePayload = LookupIntentRespMsg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength  = _NetMessage_calcMessageLengthDummy;
}

LookupIntentRespMsg* LookupIntentRespMsg_construct(void)
{
   struct LookupIntentRespMsg* this = os_kmalloc(sizeof(*this) );

   LookupIntentRespMsg_init(this);

   return this;
}

void LookupIntentRespMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void LookupIntentRespMsg_destruct(NetMessage* this)
{
   LookupIntentRespMsg_uninit( (NetMessage*)this);

   os_kfree(this);
}

int LookupIntentRespMsg_getResponseFlags(LookupIntentRespMsg* this)
{
   return this->responseFlags;
}


FhgfsOpsErr LookupIntentRespMsg_getLookupResult(LookupIntentRespMsg* this)
{
   return (FhgfsOpsErr)this->lookupResult;
}

EntryInfo* LookupIntentRespMsg_getEntryInfo(LookupIntentRespMsg* this)
{
   return &this->entryInfo;
}


FhgfsOpsErr LookupIntentRespMsg_getRevalidateResult(LookupIntentRespMsg* this)
{
   return (FhgfsOpsErr)this->revalidateResult;
}

FhgfsOpsErr LookupIntentRespMsg_getOpenResult(LookupIntentRespMsg* this)
{
   return (FhgfsOpsErr)this->openResult;
}

const char* LookupIntentRespMsg_getFileHandleID(LookupIntentRespMsg* this)
{
   return this->fileHandleID;
}

FhgfsOpsErr LookupIntentRespMsg_getCreateResult(LookupIntentRespMsg* this)
{
   return (FhgfsOpsErr)this->createResult;
}

FhgfsOpsErr LookupIntentRespMsg_getStatResult(LookupIntentRespMsg* this)
{
   return (FhgfsOpsErr)this->statResult;
}

StatData* LookupIntentRespMsg_getStatData(LookupIntentRespMsg* this)
{
   return &this->statData;
}


/**
 * Initialize EntryInfo with values from LookupIntentRespMsg
 */
void LookupIntentRespMsg_toEntryInfo(LookupIntentRespMsg* this, EntryInfo* outEntryInfo)
{
   EntryInfo_dup(&this->entryInfo, outEntryInfo);
}

/**
 * Initialize EntryInfo with values from LookupIntentRespMsg
 */
void LookupIntentRespMsg_toPathInfo(LookupIntentRespMsg* this, PathInfo* outPathInfo)
{
   PathInfo_dup(&this->pathInfo, outPathInfo);
}

#endif /* LOOKUPINTENTRESPMSG_H_ */

#ifndef LOOKUPINTENTMSG_H_
#define LOOKUPINTENTMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/EntryInfo.h>


/**
 * This message supports only serialization; deserialization is not implemented!
 */


/* intentFlags as payload
   Keep these flags in sync with the userspace msg flags:
   fhgfs_common/source/common/net/message/storage/LookupIntentMsg.h */
#define LOOKUPINTENTMSG_FLAG_REVALIDATE         1 /* revalidate entry, cancel if invalid */
#define LOOKUPINTENTMSG_FLAG_CREATE             2 /* create file */
#define LOOKUPINTENTMSG_FLAG_CREATEEXCLUSIVE    4 /* exclusive file creation */
#define LOOKUPINTENTMSG_FLAG_OPEN               8 /* open file */
#define LOOKUPINTENTMSG_FLAG_STAT              16 /* stat file */


// feature flags as header flags
#define LOOKUPINTENTMSG_FLAG_USE_QUOTA          1 /* if the message contains quota informations */


struct LookupIntentMsg;
typedef struct LookupIntentMsg LookupIntentMsg;


static inline void LookupIntentMsg_init(LookupIntentMsg* this);
static inline void LookupIntentMsg_initFromName(LookupIntentMsg* this, const EntryInfo* parentInfo,
   const char* entryName);
static inline void LookupIntentMsg_initFromEntryInfo(LookupIntentMsg* this,
   const EntryInfo* parentInfo, const char* entryName, const EntryInfo* entryInfo);
static inline LookupIntentMsg* LookupIntentMsg_construct(void);
static inline LookupIntentMsg* LookupIntentMsg_constructFromName(const EntryInfo* parentInfo,
   const char* entryName);
static inline void LookupIntentMsg_uninit(NetMessage* this);
static inline void LookupIntentMsg_destruct(NetMessage* this);

// virtual functions
extern void LookupIntentMsg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool LookupIntentMsg_deserializePayload(NetMessage* this, const char* buf,
   size_t bufLen);
extern unsigned LookupIntentMsg_calcMessageLength(NetMessage* this);

// inliners
static inline void LookupIntentMsg_addIntentCreate(LookupIntentMsg* this,
   unsigned userID, unsigned groupID, int mode, UInt16List* preferredTargets);
static inline void LookupIntentMsg_addIntentCreateExclusive(LookupIntentMsg* this);
static inline void LookupIntentMsg_addIntentOpen(LookupIntentMsg* this, const char* sessionID,
   unsigned accessFlags);
static inline void LookupIntentMsg_addIntentStat(LookupIntentMsg* this);


struct LookupIntentMsg
{
   NetMessage netMessage;

   int intentFlags; // combination of LOOKUPINTENTMSG_FLAG_...

   const char* entryName; // (lookup data), set for all but revalidate
   unsigned entryNameLen; // (lookup data), set for all but revalidate

   const EntryInfo* entryInfoPtr; // (revalidation data)

   unsigned userID; // (file creation data)
   unsigned groupID; // (file creation data)
   int mode; // file creation mode permission bits  (file creation data)

   unsigned sessionIDLen; // (file open data)
   const char* sessionID; // (file open data)
   unsigned accessFlags; // OPENFILE_ACCESS_... flags (file open data)

   // for serialization
   const EntryInfo* parentInfoPtr; // not owned by this object (lookup/open/creation data)
   UInt16List* preferredTargets; // not owned by this object! (file creation data)
};


void LookupIntentMsg_init(LookupIntentMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_LookupIntent);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = LookupIntentMsg_uninit;

   ( (NetMessage*)this)->serializePayload = LookupIntentMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = _NetMessage_deserializeDummy;

   ( (NetMessage*)this)->calcMessageLength = LookupIntentMsg_calcMessageLength;
}

/**
 * This just prepares the basic lookup. Use the additional addIntent...() methods to do
 * more than just the lookup.
 *
 * @param parentEntryID just a reference, so do not free it as long as you use this object!
 * @param entryName just a reference, so do not free it as long as you use this object!
 */
void LookupIntentMsg_initFromName(LookupIntentMsg* this, const EntryInfo* parentInfo,
   const char* entryName)
{
   LookupIntentMsg_init(this);

   this->intentFlags = 0;

   this->parentInfoPtr = parentInfo;

   this->entryName = entryName;
   this->entryNameLen = os_strlen(entryName);
}

/**
 * Initialize from entryInfo - supposed to be used for revalidate intent
 */
void LookupIntentMsg_initFromEntryInfo(LookupIntentMsg* this, const EntryInfo* parentInfo,
   const char* entryName, const EntryInfo* entryInfo)
{
   LookupIntentMsg_init(this);

   this->intentFlags = LOOKUPINTENTMSG_FLAG_REVALIDATE;

   this->parentInfoPtr = parentInfo;

   this->entryName = entryName;
   this->entryNameLen = os_strlen(entryName);

   this->entryInfoPtr = entryInfo;
}


LookupIntentMsg* LookupIntentMsg_construct(void)
{
   struct LookupIntentMsg* this = os_kmalloc(sizeof(*this) );

   LookupIntentMsg_init(this);

   return this;
}

/**
 * This just prepares the basic lookup. Use the additional addIntent...() methods to do
 * more than just the lookup.
 *
 * @param parentInfo just a reference, so do not free it as long as you use this object!
 * @param entryName just a reference, so do not free it as long as you use this object!
 */
LookupIntentMsg* LookupIntentMsg_constructFromName(const EntryInfo* parentInfo,
   const char* entryName)
{
   struct LookupIntentMsg* this = os_kmalloc(sizeof(*this) );

   LookupIntentMsg_initFromName(this, parentInfo, entryName);

   return this;
}

void LookupIntentMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void LookupIntentMsg_destruct(NetMessage* this)
{
   LookupIntentMsg_uninit( (NetMessage*)this);

   os_kfree(this);
}

void LookupIntentMsg_addIntentCreate(LookupIntentMsg* this,
   unsigned userID, unsigned groupID, int mode, UInt16List* preferredTargets)
{
   this->intentFlags |= LOOKUPINTENTMSG_FLAG_CREATE;

   this->userID = userID;
   this->groupID = groupID;
   this->mode = mode;
   this->preferredTargets = preferredTargets;
}

void LookupIntentMsg_addIntentCreateExclusive(LookupIntentMsg* this)
{
   this->intentFlags |= LOOKUPINTENTMSG_FLAG_CREATEEXCLUSIVE;
}

/**
 * @param accessFlags OPENFILE_ACCESS_... flags
 */
void LookupIntentMsg_addIntentOpen(LookupIntentMsg* this, const char* sessionID,
   unsigned accessFlags)
{
   this->intentFlags |= LOOKUPINTENTMSG_FLAG_OPEN;

   this->sessionID = sessionID;
   this->sessionIDLen = os_strlen(sessionID);

   this->accessFlags = accessFlags;
}

void LookupIntentMsg_addIntentStat(LookupIntentMsg* this)
{
   this->intentFlags |= LOOKUPINTENTMSG_FLAG_STAT;
}



#endif /* LOOKUPINTENTMSG_H_ */

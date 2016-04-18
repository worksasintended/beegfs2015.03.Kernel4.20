#ifndef FLOCKRANGEMSG_H_
#define FLOCKRANGEMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/EntryInfo.h>

/**
 * This message is for serialiazation (outgoing) only, deserialization is not implemented!!
 */

struct FLockRangeMsg;
typedef struct FLockRangeMsg FLockRangeMsg;

static inline void FLockRangeMsg_init(FLockRangeMsg* this);
static inline void FLockRangeMsg_initFromSession(FLockRangeMsg* this, const char* clientID,
   const char* fileHandleID, const EntryInfo* entryInfo,
   int ownerPID, int lockTypeFlags, uint64_t start, uint64_t end, const char* lockAckID);
static inline FLockRangeMsg* FLockRangeMsg_construct(void);
static inline FLockRangeMsg* FLockRangeMsg_constructFromSession(const char* clientID,
   const char* fileHandleID, const EntryInfo* entryInfo,
   int ownerPID, int lockTypeFlags, uint64_t start, uint64_t end, const char* lockAckID);
static inline void FLockRangeMsg_uninit(NetMessage* this);
static inline void FLockRangeMsg_destruct(NetMessage* this);

// virtual functions
extern void FLockRangeMsg_serializePayload(NetMessage* this, char* buf);
extern unsigned FLockRangeMsg_calcMessageLength(NetMessage* this);


struct FLockRangeMsg
{
   NetMessage netMessage;

   const char* clientID;
   unsigned clientIDLen;
   const char* fileHandleID;
   unsigned fileHandleIDLen;
   const EntryInfo* entryInfoPtr; // not owned by this object
   int ownerPID; // pid on client (just informative, because shared on fork() )
   int lockTypeFlags; // ENTRYLOCKTYPE_...
   uint64_t start;
   uint64_t end;
   const char* lockAckID; // ID for ack message when log is granted
   unsigned lockAckIDLen;
};


void FLockRangeMsg_init(FLockRangeMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_FLockRange);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = FLockRangeMsg_uninit;

   ( (NetMessage*)this)->serializePayload = FLockRangeMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = _NetMessage_deserializeDummy;
   ( (NetMessage*)this)->calcMessageLength = FLockRangeMsg_calcMessageLength;
}

/**
 * @param sessionID just a reference, so do not free it as long as you use this object!
 * @param fileHandleID just a reference, so do not free it as long as you use this object!
 * @param entryInfo just a reference, so do not free it as long as you use this object!
 */
void FLockRangeMsg_initFromSession(FLockRangeMsg* this, const char* clientID,
   const char* fileHandleID, const EntryInfo* entryInfo,
   int ownerPID, int lockTypeFlags, uint64_t start, uint64_t end, const char* lockAckID)
{
   FLockRangeMsg_init(this);

   this->clientID = clientID;
   this->clientIDLen = os_strlen(clientID);

   this->fileHandleID = fileHandleID;
   this->fileHandleIDLen = os_strlen(fileHandleID);

   this->entryInfoPtr = entryInfo;

   this->ownerPID = ownerPID;
   this->lockTypeFlags = lockTypeFlags;

   this->start = start;
   this->end = end;

   this->lockAckID = lockAckID;
   this->lockAckIDLen = os_strlen(lockAckID);
}

FLockRangeMsg* FLockRangeMsg_construct(void)
{
   struct FLockRangeMsg* this = os_kmalloc(sizeof(struct FLockRangeMsg) );

   FLockRangeMsg_init(this);

   return this;
}

/**
 * @param sessionID just a reference, so do not free it as long as you use this object!
 * @param fileHandleID just a reference, so do not free it as long as you use this object!
 */
FLockRangeMsg* FLockRangeMsg_constructFromSession(const char* clientID,
   const char* fileHandleID, const EntryInfo* entryInfo,
   int ownerPID, int lockTypeFlags, uint64_t start, uint64_t end, const char* lockAckID)
{
   struct FLockRangeMsg* this = os_kmalloc(sizeof(*this) );

   FLockRangeMsg_initFromSession(this, clientID, fileHandleID, entryInfo, ownerPID, lockTypeFlags,
      start, end, lockAckID);

   return this;
}

void FLockRangeMsg_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void FLockRangeMsg_destruct(NetMessage* this)
{
   FLockRangeMsg_uninit(this);

   os_kfree(this);
}


#endif /* FLOCKRANGEMSG_H_ */

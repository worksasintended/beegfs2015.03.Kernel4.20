#ifndef FLOCKENTRYMSG_H_
#define FLOCKENTRYMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/EntryInfo.h>

/**
 * This message is for serialiazation (outgoing) only, deserialization is not implemented!!
 */

struct FLockEntryMsg;
typedef struct FLockEntryMsg FLockEntryMsg;

static inline void FLockEntryMsg_init(FLockEntryMsg* this);
static inline void FLockEntryMsg_initFromSession(FLockEntryMsg* this, const char* clientID,
   const char* fileHandleID, const EntryInfo* entryInfo, int64_t clientFD, int ownerPID,
   int lockTypeFlags, const char* lockAckID);
static inline FLockEntryMsg* FLockEntryMsg_construct(void);
static inline FLockEntryMsg* FLockEntryMsg_constructFromSession(const char* clientID,
   const char* fileHandleID, const EntryInfo* entryInfo, int64_t clientFD, int ownerPID,
   int lockTypeFlags, const char* lockAckID);
static inline void FLockEntryMsg_uninit(NetMessage* this);
static inline void FLockEntryMsg_destruct(NetMessage* this);

// virtual functions
extern void FLockEntryMsg_serializePayload(NetMessage* this, char* buf);
extern unsigned FLockEntryMsg_calcMessageLength(NetMessage* this);


struct FLockEntryMsg
{
   NetMessage netMessage;

   const char* clientID;
   unsigned clientIDLen;
   const char* fileHandleID;
   unsigned fileHandleIDLen;
   const EntryInfo* entryInfoPtr; // not owned by this object
   int64_t clientFD; /* client-wide unique identifier (typically not really the fd number)
                      * corresponds to 'struct file_lock* fileLock->fl_file on the client side'
                      */

   int ownerPID; // pid on client (just informative, because shared on fork() )
   int lockTypeFlags; // ENTRYLOCKTYPE_...
   const char* lockAckID; // ID for ack message when log is granted
   unsigned lockAckIDLen;
};


void FLockEntryMsg_init(FLockEntryMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_FLockEntry);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = FLockEntryMsg_uninit;

   ( (NetMessage*)this)->serializePayload = FLockEntryMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = _NetMessage_deserializeDummy;
   ( (NetMessage*)this)->calcMessageLength = FLockEntryMsg_calcMessageLength;
}

/**
 * @param sessionID just a reference, so do not free it as long as you use this object!
 * @param fileHandleID just a reference, so do not free it as long as you use this object!
 * @param entryInfo just a reference, so do not free it as long as you use this object!
 */
void FLockEntryMsg_initFromSession(FLockEntryMsg* this, const char* clientID,
   const char* fileHandleID, const EntryInfo* entryInfo, int64_t clientFD, int ownerPID,
   int lockTypeFlags, const char* lockAckID)
{
   FLockEntryMsg_init(this);

   this->clientID = clientID;
   this->clientIDLen = os_strlen(clientID);

   this->fileHandleID = fileHandleID;
   this->fileHandleIDLen = os_strlen(fileHandleID);

   this->entryInfoPtr = entryInfo;

   this->clientFD = clientFD;
   this->ownerPID = ownerPID;
   this->lockTypeFlags = lockTypeFlags;

   this->lockAckID = lockAckID;
   this->lockAckIDLen = os_strlen(lockAckID);
}

FLockEntryMsg* FLockEntryMsg_construct(void)
{
   struct FLockEntryMsg* this = os_kmalloc(sizeof(*this) );

   FLockEntryMsg_init(this);

   return this;
}

/**
 * @param sessionID just a reference, so do not free it as long as you use this object!
 * @param fileHandleID just a reference, so do not free it as long as you use this object!
 */
FLockEntryMsg* FLockEntryMsg_constructFromSession(const char* clientID,
   const char* fileHandleID, const EntryInfo* entryInfo, int64_t clientFD, int ownerPID,
   int lockTypeFlags, const char* lockAckID)
{
   struct FLockEntryMsg* this = os_kmalloc(sizeof(*this) );

   FLockEntryMsg_initFromSession(this, clientID, fileHandleID, entryInfo, clientFD, ownerPID,
      lockTypeFlags, lockAckID);

   return this;
}

void FLockEntryMsg_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void FLockEntryMsg_destruct(NetMessage* this)
{
   FLockEntryMsg_uninit(this);

   os_kfree(this);
}


#endif /* FLOCKENTRYMSG_H_ */

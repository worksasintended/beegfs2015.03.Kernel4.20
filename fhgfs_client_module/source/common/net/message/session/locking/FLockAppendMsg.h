#ifndef FLOCKAPPENDMSG_H_
#define FLOCKAPPENDMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/EntryInfo.h>

/**
 * This message is for serialiazation (outgoing) only, deserialization is not implemented!!
 */

struct FLockAppendMsg;
typedef struct FLockAppendMsg FLockAppendMsg;

static inline void FLockAppendMsg_init(FLockAppendMsg* this);
static inline void FLockAppendMsg_initFromSession(FLockAppendMsg* this, const char* clientID,
   const char* fileHandleID, const EntryInfo* entryInfo, int64_t clientFD, int ownerPID,
   int lockTypeFlags, const char* lockAckID);
static inline FLockAppendMsg* FLockAppendMsg_construct(void);
static inline FLockAppendMsg* FLockAppendMsg_constructFromSession(const char* clientID,
   const char* fileHandleID, const EntryInfo* entryInfo, int64_t clientFD, int ownerPID,
   int lockTypeFlags, const char* lockAckID);
static inline void FLockAppendMsg_uninit(NetMessage* this);
static inline void FLockAppendMsg_destruct(NetMessage* this);

// virtual functions
extern void FLockAppendMsg_serializePayload(NetMessage* this, char* buf);
extern unsigned FLockAppendMsg_calcMessageLength(NetMessage* this);


struct FLockAppendMsg
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


void FLockAppendMsg_init(FLockAppendMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_FLockAppend);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = FLockAppendMsg_uninit;

   ( (NetMessage*)this)->serializePayload = FLockAppendMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = _NetMessage_deserializeDummy;
   ( (NetMessage*)this)->calcMessageLength = FLockAppendMsg_calcMessageLength;
}

/**
 * @param sessionID just a reference, so do not free it as long as you use this object!
 * @param fileHandleID just a reference, so do not free it as long as you use this object!
 * @param entryInfo just a reference, so do not free it as long as you use this object!
 */
void FLockAppendMsg_initFromSession(FLockAppendMsg* this, const char* clientID,
   const char* fileHandleID, const EntryInfo* entryInfo, int64_t clientFD, int ownerPID,
   int lockTypeFlags, const char* lockAckID)
{
   FLockAppendMsg_init(this);

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

FLockAppendMsg* FLockAppendMsg_construct(void)
{
   struct FLockAppendMsg* this = os_kmalloc(sizeof(*this) );

   FLockAppendMsg_init(this);

   return this;
}

/**
 * @param sessionID just a reference, so do not free it as long as you use this object!
 * @param fileHandleID just a reference, so do not free it as long as you use this object!
 */
FLockAppendMsg* FLockAppendMsg_constructFromSession(const char* clientID,
   const char* fileHandleID, const EntryInfo* entryInfo, int64_t clientFD, int ownerPID,
   int lockTypeFlags, const char* lockAckID)
{
   struct FLockAppendMsg* this = os_kmalloc(sizeof(*this) );

   FLockAppendMsg_initFromSession(this, clientID, fileHandleID, entryInfo, clientFD, ownerPID,
      lockTypeFlags, lockAckID);

   return this;
}

void FLockAppendMsg_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void FLockAppendMsg_destruct(NetMessage* this)
{
   FLockAppendMsg_uninit(this);

   os_kfree(this);
}


#endif /* FLOCKAPPENDMSG_H_ */

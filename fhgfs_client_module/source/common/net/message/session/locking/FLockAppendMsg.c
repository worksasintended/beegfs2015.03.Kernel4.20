#include "FLockAppendMsg.h"


void FLockAppendMsg_serializePayload(NetMessage* this, char* buf)
{
   FLockAppendMsg* thisCast = (FLockAppendMsg*)this;

   size_t bufPos = 0;

   // clientFD
   bufPos += Serialization_serializeInt64(&buf[bufPos], thisCast->clientFD);

   // ownerPID
   bufPos += Serialization_serializeInt(&buf[bufPos], thisCast->ownerPID);

   // lockTypeFlags
   bufPos += Serialization_serializeInt(&buf[bufPos], thisCast->lockTypeFlags);

   // entryInfo
   bufPos += EntryInfo_serialize(thisCast->entryInfoPtr, &buf[bufPos]);

   // clientID
   bufPos += Serialization_serializeStrAlign4(&buf[bufPos], thisCast->clientIDLen,
      thisCast->clientID);

   // fileHandleID
   bufPos += Serialization_serializeStrAlign4(&buf[bufPos], thisCast->fileHandleIDLen,
      thisCast->fileHandleID);

   // lockAckID
   bufPos += Serialization_serializeStrAlign4(&buf[bufPos], thisCast->lockAckIDLen,
      thisCast->lockAckID);
}

unsigned FLockAppendMsg_calcMessageLength(NetMessage* this)
{
   FLockAppendMsg* thisCast = (FLockAppendMsg*)this;

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenInt64()                              + // clientFD
      Serialization_serialLenInt()                                + // ownerPID
      Serialization_serialLenInt()                                + // lockTypeFlags
      EntryInfo_serialLen(thisCast->entryInfoPtr)                 + // entryInfo
      Serialization_serialLenStrAlign4(thisCast->clientIDLen)     + // clientID
      Serialization_serialLenStrAlign4(thisCast->fileHandleIDLen) + // fileHandleID
      Serialization_serialLenStrAlign4(thisCast->lockAckIDLen);     // lockAckID
}



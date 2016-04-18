#include "FLockRangeMsg.h"


void FLockRangeMsg_serializePayload(NetMessage* this, char* buf)
{
   FLockRangeMsg* thisCast = (FLockRangeMsg*)this;

   size_t bufPos = 0;

   // start
   bufPos += Serialization_serializeUInt64(&buf[bufPos], thisCast->start);

   // end
   bufPos += Serialization_serializeUInt64(&buf[bufPos], thisCast->end);

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

unsigned FLockRangeMsg_calcMessageLength(NetMessage* this)
{
   FLockRangeMsg* thisCast = (FLockRangeMsg*)this;

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenUInt64() + // start
      Serialization_serialLenUInt64() + // end
      Serialization_serialLenInt()    + // ownerPID
      Serialization_serialLenInt()    + // lockTypeFlags
      EntryInfo_serialLen(thisCast->entryInfoPtr)                 + // entryInfo
      Serialization_serialLenStrAlign4(thisCast->clientIDLen)     + // clientID
      Serialization_serialLenStrAlign4(thisCast->fileHandleIDLen) + // fileHandleID
      Serialization_serialLenStrAlign4(thisCast->lockAckIDLen);     // lockAckID
}



#include "ReadLocalFileV2Msg.h"


void ReadLocalFileV2Msg_serializePayload(NetMessage* this, char* buf)
{
   ReadLocalFileV2Msg* thisCast = (ReadLocalFileV2Msg*)this;

   size_t bufPos = 0;
   
   // offset
   bufPos += Serialization_serializeInt64(&buf[bufPos], thisCast->offset);

   // count
   bufPos += Serialization_serializeInt64(&buf[bufPos], thisCast->count);

   // accessFlags
   bufPos += Serialization_serializeUInt(&buf[bufPos], thisCast->accessFlags);

   // fileHandleID
   bufPos += Serialization_serializeStrAlign4(&buf[bufPos], thisCast->fileHandleIDLen,
      thisCast->fileHandleID);

   // sessionID
   bufPos += Serialization_serializeStrAlign4(&buf[bufPos], thisCast->sessionIDLen,
      thisCast->sessionID);

   // pathInfo
   bufPos += PathInfo_serialize(thisCast->pathInfoPtr, &buf[bufPos]);

   // targetID
   bufPos += Serialization_serializeUShort(&buf[bufPos], thisCast->targetID);
}

unsigned ReadLocalFileV2Msg_calcMessageLength(NetMessage* this)
{
   ReadLocalFileV2Msg* thisCast = (ReadLocalFileV2Msg*)this;

   return NETMSG_HEADER_LENGTH                                       +
      Serialization_serialLenInt64()                                 + // offset
      Serialization_serialLenInt64()                                 + // count
      Serialization_serialLenUInt()                                  + // accessFlags
      Serialization_serialLenStrAlign4(thisCast->fileHandleIDLen)    +
      Serialization_serialLenStrAlign4(thisCast->sessionIDLen)       +
      PathInfo_serialLen(thisCast->pathInfoPtr)                      + // pathInfo
      Serialization_serialLenUShort();                                 // targetID
}


#include "HardlinkMsg.h"


void HardlinkMsg_serializePayload(NetMessage* this, char* buf)
{
   HardlinkMsg* thisCast = (HardlinkMsg*)this;

   size_t bufPos = 0;

   // fromInfo
   bufPos += EntryInfo_serialize(thisCast->fromInfo, &buf[bufPos]);

   // toDirInfo
   bufPos += EntryInfo_serialize(thisCast->toDirInfo, &buf[bufPos]);
   
   // toName
   bufPos += Serialization_serializeStrAlign4(&buf[bufPos],
      thisCast->toNameLen, thisCast->toName);

   // fromDirInfo
   bufPos += EntryInfo_serialize(thisCast->fromDirInfo, &buf[bufPos]);

   // fromName
   bufPos += Serialization_serializeStrAlign4(&buf[bufPos],
      thisCast->fromNameLen, thisCast->fromName);

}

unsigned HardlinkMsg_calcMessageLength(NetMessage* this)
{
   HardlinkMsg* thisCast = (HardlinkMsg*)this;

   return NETMSG_HEADER_LENGTH +
      EntryInfo_serialLen(thisCast->fromInfo)                  + // fromInfo
      EntryInfo_serialLen(thisCast->toDirInfo)                 + // toDirInfo
      Serialization_serialLenStrAlign4(thisCast->toNameLen)    + // toName
      EntryInfo_serialLen(thisCast->fromDirInfo)               + // fromDirInfo
      Serialization_serialLenStrAlign4(thisCast->fromNameLen);   // fromName
}



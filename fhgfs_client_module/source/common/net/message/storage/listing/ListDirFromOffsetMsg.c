#include "ListDirFromOffsetMsg.h"


void ListDirFromOffsetMsg_serializePayload(NetMessage* this, char* buf)
{
   ListDirFromOffsetMsg* thisCast = (ListDirFromOffsetMsg*)this;

   size_t bufPos = 0;
   
   // serverOffset
   bufPos += Serialization_serializeInt64(&buf[bufPos], thisCast->serverOffset);
   
   // maxOutNames
   bufPos += Serialization_serializeUInt(&buf[bufPos], thisCast->maxOutNames);

   // EntryInfo
   bufPos += EntryInfo_serialize(thisCast->entryInfoPtr, &buf[bufPos]);

   // filterDots
   bufPos += Serialization_serializeBool(&buf[bufPos], thisCast->filterDots);
}

unsigned ListDirFromOffsetMsg_calcMessageLength(NetMessage* this)
{
   ListDirFromOffsetMsg* thisCast = (ListDirFromOffsetMsg*)this;

   return NETMSG_HEADER_LENGTH +
      EntryInfo_serialLen(thisCast->entryInfoPtr) +
      Serialization_serialLenInt64() + // serverOffset
      Serialization_serialLenUInt() + // maxOutNames
      Serialization_serialLenBool(); // filterDots
}



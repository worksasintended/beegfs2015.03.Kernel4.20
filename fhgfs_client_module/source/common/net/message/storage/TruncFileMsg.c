#include "TruncFileMsg.h"


void TruncFileMsg_serializePayload(NetMessage* this, char* buf)
{
   TruncFileMsg* thisCast = (TruncFileMsg*)this;

   size_t bufPos = 0;
   
   // filesize
   bufPos += Serialization_serializeInt64(&buf[bufPos], thisCast->filesize);
   
   // entryInfo
   bufPos += EntryInfo_serialize(thisCast->entryInfoPtr, &buf[bufPos]);
}

unsigned TruncFileMsg_calcMessageLength(NetMessage* this)
{
   TruncFileMsg* thisCast = (TruncFileMsg*)this;

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenInt64() + // filesize
      EntryInfo_serialLen(thisCast->entryInfoPtr); // entryInfo
}



#include "StatMsg.h"


void StatMsg_serializePayload(NetMessage* this, char* buf)
{
   StatMsg* thisCast = (StatMsg*)this;

   size_t bufPos = 0;

   // entryInfo
   bufPos += EntryInfo_serialize(thisCast->entryInfoPtr, &buf[bufPos]);
}

unsigned StatMsg_calcMessageLength(NetMessage* this)
{
   StatMsg* thisCast = (StatMsg*)this;

   return NETMSG_HEADER_LENGTH +
      EntryInfo_serialLen(thisCast->entryInfoPtr); // entryInfo
}



#include "RefreshEntryInfoMsg.h"


void RefreshEntryInfoMsg_serializePayload(NetMessage* this, char* buf)
{
   RefreshEntryInfoMsg* thisCast = (RefreshEntryInfoMsg*)this;
   size_t bufPos = 0;

   // entryInfo
   bufPos += EntryInfo_serialize(thisCast->entryInfoPtr, &buf[bufPos]);
}

unsigned RefreshEntryInfoMsg_calcMessageLength(NetMessage* this)
{
   RefreshEntryInfoMsg* thisCast = (RefreshEntryInfoMsg*)this;

   return NETMSG_HEADER_LENGTH +
      EntryInfo_serialLen(thisCast->entryInfoPtr); // entryInfo
}

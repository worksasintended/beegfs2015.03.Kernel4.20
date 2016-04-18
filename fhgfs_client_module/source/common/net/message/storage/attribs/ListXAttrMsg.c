#include "ListXAttrMsg.h"

void ListXAttrMsg_serializePayload(NetMessage* this, char* buf)
{
   ListXAttrMsg* thisCast = (ListXAttrMsg*)this;

   size_t bufPos = 0;

   bufPos += EntryInfo_serialize(thisCast->entryInfoPtr, &buf[bufPos] );
   bufPos += Serialization_serializeInt(&buf[bufPos], thisCast->size);
}

unsigned ListXAttrMsg_calcMessageLength(NetMessage* this)
{
   ListXAttrMsg* thisCast = (ListXAttrMsg*)this;

   return NETMSG_HEADER_LENGTH +
         EntryInfo_serialLen(thisCast->entryInfoPtr) + // entryInfo
         Serialization_serialLenInt(); // size
}

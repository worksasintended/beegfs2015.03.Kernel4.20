#include "GetXAttrMsg.h"

void GetXAttrMsg_serializePayload(NetMessage* this, char* buf)
{
   GetXAttrMsg* thisCast = (GetXAttrMsg*)this;

   size_t bufPos = 0;

   bufPos += EntryInfo_serialize(thisCast->entryInfoPtr, &buf[bufPos]);
   bufPos += Serialization_serializeStr(&buf[bufPos], strlen(thisCast->name), thisCast->name);
   bufPos += Serialization_serializeInt(&buf[bufPos], thisCast->size);
}

unsigned GetXAttrMsg_calcMessageLength(NetMessage* this)
{
   GetXAttrMsg* thisCast = (GetXAttrMsg*)this;

   return NETMSG_HEADER_LENGTH +
         EntryInfo_serialLen(thisCast->entryInfoPtr) + // entryInfo
         Serialization_serialLenStr(strlen(thisCast->name) ) + // name
         Serialization_serialLenInt(); // size
}

#include "SetXAttrMsg.h"

void SetXAttrMsg_serializePayload(NetMessage* this, char* buf)
{
   SetXAttrMsg* thisCast = (SetXAttrMsg*)this;

   size_t bufPos = 0;

   bufPos += EntryInfo_serialize(thisCast->entryInfoPtr, &buf[bufPos]);
   bufPos += Serialization_serializeStr(&buf[bufPos], strlen(thisCast->name), thisCast->name);
   bufPos += Serialization_serializeCharArray(&buf[bufPos], thisCast->size, thisCast->value);
   bufPos += Serialization_serializeInt(&buf[bufPos], thisCast->flags);
}

unsigned SetXAttrMsg_calcMessageLength(NetMessage* this)
{
   SetXAttrMsg* thisCast = (SetXAttrMsg*)this;

   return NETMSG_HEADER_LENGTH +
         EntryInfo_serialLen(thisCast->entryInfoPtr) + // entryInfo
         Serialization_serialLenStr(strlen(thisCast->name) ) + // name
         Serialization_serialLenCharArray(thisCast->size) + // value
         Serialization_serialLenInt(); // flags
}

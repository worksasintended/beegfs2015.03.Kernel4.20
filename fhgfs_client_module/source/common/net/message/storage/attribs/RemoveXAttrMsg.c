#include "RemoveXAttrMsg.h"

void RemoveXAttrMsg_serializePayload(NetMessage* this, char* buf)
{
   RemoveXAttrMsg* thisCast = (RemoveXAttrMsg*)this;

   size_t bufPos = 0;

   bufPos += EntryInfo_serialize(thisCast->entryInfoPtr, &buf[bufPos]);
   bufPos += Serialization_serializeStr(&buf[bufPos], strlen(thisCast->name), thisCast->name);
}

unsigned RemoveXAttrMsg_calcMessageLength(NetMessage* this)
{
   RemoveXAttrMsg* thisCast = (RemoveXAttrMsg*)this;

   return NETMSG_HEADER_LENGTH +
         EntryInfo_serialLen(thisCast->entryInfoPtr) + // entryInfo
         Serialization_serialLenStr(strlen(thisCast->name) ); // name
}

#include "SetAttrMsg.h"


void SetAttrMsg_serializePayload(NetMessage* this, char* buf)
{
   SetAttrMsg* thisCast = (SetAttrMsg*)this;

   size_t bufPos = 0;
   
   // validAttribs
   bufPos += Serialization_serializeInt(&buf[bufPos], thisCast->validAttribs);

   // mode
   bufPos += Serialization_serializeInt(&buf[bufPos], thisCast->attribs.mode);

   // modificationTimeSecs
   bufPos += Serialization_serializeInt64(&buf[bufPos], thisCast->attribs.modificationTimeSecs);

   // lastAccessTimeSecs
   bufPos += Serialization_serializeInt64(&buf[bufPos], thisCast->attribs.lastAccessTimeSecs);

   // userID
   bufPos += Serialization_serializeUInt(&buf[bufPos], thisCast->attribs.userID);

   // groupID
   bufPos += Serialization_serializeUInt(&buf[bufPos], thisCast->attribs.groupID);

   // entryInfo
   bufPos += EntryInfo_serialize(thisCast->entryInfoPtr, &buf[bufPos]);
}


unsigned SetAttrMsg_calcMessageLength(NetMessage* this)
{
   SetAttrMsg* thisCast = (SetAttrMsg*)this;

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenInt() + // validAttribs
      Serialization_serialLenInt() + // mode
      Serialization_serialLenInt64() + // modificationTimeSecs
      Serialization_serialLenInt64() + // lastAccessTimeSecs
      Serialization_serialLenUInt() + // userID
      Serialization_serialLenUInt() + // groupID
      EntryInfo_serialLen(thisCast->entryInfoPtr); // entryInfo
}



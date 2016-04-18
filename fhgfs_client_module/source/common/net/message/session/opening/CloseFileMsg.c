#include "CloseFileMsg.h"


void CloseFileMsg_serializePayload(NetMessage* this, char* buf)
{
   CloseFileMsg* thisCast = (CloseFileMsg*)this;

   size_t bufPos = 0;
   
   // sessionID
   bufPos += Serialization_serializeStrAlign4(&buf[bufPos], thisCast->sessionIDLen,
      thisCast->sessionID);
   
   // fileHandleID
   bufPos += Serialization_serializeStrAlign4(&buf[bufPos], thisCast->fileHandleIDLen,
      thisCast->fileHandleID);

   // entryInfo
   bufPos += EntryInfo_serialize(thisCast->entryInfoPtr, &buf[bufPos]);

   // maxUsedNodeIndex
   bufPos += Serialization_serializeInt(&buf[bufPos], thisCast->maxUsedNodeIndex);
}


unsigned CloseFileMsg_calcMessageLength(NetMessage* this)
{
   CloseFileMsg* thisCast = (CloseFileMsg*)this;

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenStrAlign4(thisCast->sessionIDLen)    +  // sessionID
      Serialization_serialLenStrAlign4(thisCast->fileHandleIDLen) +  // fileHandleID
      EntryInfo_serialLen(thisCast->entryInfoPtr)                 +  // entryInfo
      Serialization_serialLenInt();                                  // maxUsedNodeIndex
}



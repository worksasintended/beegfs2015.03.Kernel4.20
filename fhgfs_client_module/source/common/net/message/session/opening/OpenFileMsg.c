#include "OpenFileMsg.h"


void OpenFileMsg_serializePayload(NetMessage* this, char* buf)
{
   OpenFileMsg* thisCast = (OpenFileMsg*)this;

   size_t bufPos = 0;
   
   // accessFlags
   bufPos += Serialization_serializeUInt(&buf[bufPos], thisCast->accessFlags);

   // sessionID
   bufPos += Serialization_serializeStrAlign4(&buf[bufPos], thisCast->sessionIDLen,
      thisCast->sessionID);
  
   // entryInfo
   bufPos += EntryInfo_serialize(thisCast->entryInfoPtr, &buf[bufPos]);
}


unsigned OpenFileMsg_calcMessageLength(NetMessage* this)
{
   OpenFileMsg* thisCast = (OpenFileMsg*)this;

   return NETMSG_HEADER_LENGTH                                 +
      Serialization_serialLenUInt()                            + // accessFlags
      Serialization_serialLenStrAlign4(thisCast->sessionIDLen) + // sessionID
      EntryInfo_serialLen(thisCast->entryInfoPtr);               // entryInfo
}



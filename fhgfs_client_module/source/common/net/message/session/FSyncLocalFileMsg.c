#include "FSyncLocalFileMsg.h"


void FSyncLocalFileMsg_serializePayload(NetMessage* this, char* buf)
{
   FSyncLocalFileMsg* thisCast = (FSyncLocalFileMsg*)this;

   size_t bufPos = 0;
   
   // sessionID
   bufPos += Serialization_serializeStrAlign4(&buf[bufPos], thisCast->sessionIDLen,
      thisCast->sessionID);
   
   // fileHandleID
   bufPos += Serialization_serializeStrAlign4(&buf[bufPos], thisCast->fileHandleIDLen,
      thisCast->fileHandleID);

   // targetID
   bufPos += Serialization_serializeUShort(&buf[bufPos], thisCast->targetID);
}

unsigned FSyncLocalFileMsg_calcMessageLength(NetMessage* this)
{
   FSyncLocalFileMsg* thisCast = (FSyncLocalFileMsg*)this;

   unsigned retVal = NETMSG_HEADER_LENGTH +
      Serialization_serialLenStrAlign4(thisCast->sessionIDLen) +
      Serialization_serialLenStrAlign4(thisCast->fileHandleIDLen) +
      Serialization_serialLenUShort(); // targetID

   return retVal;
}



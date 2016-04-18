#include "RefreshSessionMsg.h"


void RefreshSessionMsg_serializePayload(NetMessage* this, char* buf)
{
   RefreshSessionMsg* thisCast = (RefreshSessionMsg*)this;

   size_t bufPos = 0;
   
   // sessionID
   bufPos += Serialization_serializeStr(&buf[bufPos], thisCast->sessionIDLen, thisCast->sessionID);
}

fhgfs_bool RefreshSessionMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   RefreshSessionMsg* thisCast = (RefreshSessionMsg*)this;

   size_t bufPos = 0;

   unsigned sessionBufLen;
   
   // sessionID
   if(!Serialization_deserializeStr(&buf[bufPos], bufLen-bufPos,
      &thisCast->sessionIDLen, &thisCast->sessionID, &sessionBufLen) )
      return fhgfs_false;
   
   bufPos += sessionBufLen;
   
   return fhgfs_true;   
}

unsigned RefreshSessionMsg_calcMessageLength(NetMessage* this)
{
   RefreshSessionMsg* thisCast = (RefreshSessionMsg*)this;

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenStr(thisCast->sessionIDLen); // sessionID
}



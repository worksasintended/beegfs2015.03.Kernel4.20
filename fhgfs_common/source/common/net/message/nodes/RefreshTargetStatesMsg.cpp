#include "RefreshTargetStatesMsg.h"

void RefreshTargetStatesMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // ackID
   bufPos += Serialization::serializeStr(&buf[bufPos], ackIDLen, ackID);
}

bool RefreshTargetStatesMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   // ackID

   unsigned ackBufLen;

   if(!Serialization::deserializeStr(&buf[bufPos], bufLen-bufPos,
      &ackIDLen, &ackID, &ackBufLen) )
      return false;

   bufPos += ackBufLen;

   return true;
}




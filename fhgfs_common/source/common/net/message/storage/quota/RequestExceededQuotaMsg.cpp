#include <common/toolkit/serialization/Serialization.h>
#include "RequestExceededQuotaMsg.h"


void RequestExceededQuotaMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // quotaDataType
   bufPos += Serialization::serializeInt(&buf[bufPos], this->quotaDataType);

   // exceededType
   bufPos += Serialization::serializeInt(&buf[bufPos], this->exceededType);
}

bool RequestExceededQuotaMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   // quotaDataType

   unsigned idTypeBufLen;

   if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos,
      &this->quotaDataType, &idTypeBufLen) )
      return false;

   bufPos += idTypeBufLen;


   // exceededType

   unsigned exceededTypeBufLen;

   if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos,
      &this->exceededType, &exceededTypeBufLen) )
      return false;

   bufPos += exceededTypeBufLen;

   return true;
}


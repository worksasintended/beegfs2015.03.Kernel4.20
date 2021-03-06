#include "SetQuotaMsg.h"

void SetQuotaMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // quotaData
   bufPos += Serialization::serializeQuotaDataList(&buf[bufPos], &this->quotaData);
}

bool SetQuotaMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   // quotaData

   if(!Serialization::deserializeQuotaDataListPreprocess(&buf[bufPos], bufLen-bufPos,
      &this->quotaDataElemNum, &this->quotaDataStart, &this->quotaDataBufLen) )
      return false;

   bufPos += this->quotaDataBufLen;

   return true;
}

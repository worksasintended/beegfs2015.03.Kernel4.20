#include "SetExceededQuotaMsg.h"


void SetExceededQuotaMsg::serializeContent(char* buf, size_t *outBufPos)
{
   size_t bufPos = 0;

   // quotaDataType
   bufPos += Serialization::serializeInt(&buf[bufPos], this->quotaDataType);

   // exceededType
   bufPos += Serialization::serializeInt(&buf[bufPos], this->exceededType);

   //idList
   bufPos += Serialization::serializeUIntList(&buf[bufPos], &this->exceededQuotaIDs);

   *outBufPos = bufPos;
}

bool SetExceededQuotaMsg::deserializeContent(const char* buf, size_t bufLen, size_t *outBufPos)
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


   // idList

   if(!Serialization::deserializeUIntListPreprocess(&buf[bufPos], bufLen-bufPos,
      &this->idListElemNum, &this->idListListStart, &this->idListBufLen) )
      return false;

   bufPos += this->idListBufLen;

   *outBufPos = bufPos;

   return true;
}

void SetExceededQuotaMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   serializeContent(buf, &bufPos);
}

bool SetExceededQuotaMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   return deserializeContent(buf, bufLen, &bufPos);
}

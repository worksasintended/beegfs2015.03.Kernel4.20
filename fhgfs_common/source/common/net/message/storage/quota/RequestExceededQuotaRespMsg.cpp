#include "RequestExceededQuotaRespMsg.h"


void RequestExceededQuotaRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // base class
   SetExceededQuotaMsg::serializeContent(buf, &bufPos);

   // error
   if(isMsgHeaderCompatFeatureFlagSet(REQUESTEXCEEDEDQUOTARESPMSG_FLAG_QUOTA_CONFIG) )
      bufPos += Serialization::serializeInt(&buf[bufPos], this->error);
}

bool RequestExceededQuotaRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   bool retVal = false;
   size_t bufPos = 0;


   // base class

   retVal = SetExceededQuotaMsg::deserializeContent(buf, bufLen, &bufPos);


   // error

   if(isMsgHeaderCompatFeatureFlagSet(REQUESTEXCEEDEDQUOTARESPMSG_FLAG_QUOTA_CONFIG) )
   {
      unsigned errorBufLen;

      if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos,
         &this->error, &errorBufLen) )
         return false;

      bufPos += errorBufLen;
   }
   else
   {
      this->error = FhgfsOpsErr_NOTSUPP;        // mark error value is not supported by mgmtd
   }

   return retVal;
}

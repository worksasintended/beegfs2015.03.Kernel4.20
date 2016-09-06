#include "GetQuotaInfoRespMsg.h"

void GetQuotaInfoRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // quotaData
   bufPos += Serialization::serializeQuotaDataList(&buf[bufPos], this->quotaData);

   if(isMsgHeaderCompatFeatureFlagSet(MsgCompatFlags::QUOTA_INODE_SUPPORT) )
   {
      // quotaInodeSupport
      bufPos += Serialization::serializeUInt(&buf[bufPos], this->quotaInodeSupport);
   }
}

bool GetQuotaInfoRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   // quotaData

   if(!Serialization::deserializeQuotaDataListPreprocess(&buf[bufPos], bufLen-bufPos,
      &this->quotaDataElemNum, &this->quotaDataStart, &this->quotaDataBufLen) )
      return false;

   bufPos += this->quotaDataBufLen;


   if(isMsgHeaderCompatFeatureFlagSet(MsgCompatFlags::QUOTA_INODE_SUPPORT) )
   {
      // quotaInodeSupport

      unsigned quotaInodeSupportBufLen;

      if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos,
         &this->quotaInodeSupport, &quotaInodeSupportBufLen) )
         return false;

      bufPos += quotaInodeSupportBufLen;
   }

   return true;
}

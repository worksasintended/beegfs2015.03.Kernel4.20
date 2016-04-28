#include "SetLocalAttrRespMsg.h"

bool SetLocalAttrRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   // result
   unsigned resultFieldLen;
   int intResult;
   if (!Serialization::deserializeInt(&buf[bufPos], bufLen - bufPos, &intResult, &resultFieldLen))
   {
      return false;
   }

   result = (FhgfsOpsErr) intResult;

   bufPos += resultFieldLen;

   if (isMsgHeaderCompatFeatureFlagSet(SETLOCALATTRRESPMSG_COMPAT_FLAG_HAS_ATTRS))
   {
      // filesize
      unsigned filesizeBufLen;
      if (!Serialization::deserializeInt64(&buf[bufPos], bufLen - bufPos, &filesize,
         &filesizeBufLen))
         return false;

      bufPos += filesizeBufLen;

      // numBlocks
      unsigned numBlocksBufLen;
      if (!Serialization::deserializeInt64(&buf[bufPos], bufLen - bufPos, &numBlocks,
         &numBlocksBufLen))
         return false;

      bufPos += numBlocksBufLen;

      // modificationTimeSecs
      unsigned modificationTimeSecsBufLen;
      if (!Serialization::deserializeInt64(&buf[bufPos], bufLen - bufPos, &modificationTimeSecs,
         &modificationTimeSecsBufLen))
         return false;

      bufPos += modificationTimeSecsBufLen;

      // lastAccessTimeSecs
      unsigned lastAccessTimeSecsBufLen;
      if (!Serialization::deserializeInt64(&buf[bufPos], bufLen - bufPos, &lastAccessTimeSecs,
         &lastAccessTimeSecsBufLen))
         return false;

      bufPos += lastAccessTimeSecsBufLen;

      // storageVersion
      unsigned storageVersionBufLen;
      if (!Serialization::deserializeUInt64(&buf[bufPos], bufLen - bufPos, &storageVersion,
         &storageVersionBufLen))
      {
         return false;
      }

      bufPos += storageVersionBufLen;
   }

   return true;
}

void SetLocalAttrRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // result
   bufPos += Serialization::serializeInt(&buf[bufPos], (int) result);

   if (isMsgHeaderCompatFeatureFlagSet(SETLOCALATTRRESPMSG_COMPAT_FLAG_HAS_ATTRS))
   {
      // filesize
      bufPos += Serialization::serializeInt64(&buf[bufPos], filesize);

      // numBlocks
      bufPos += Serialization::serializeInt64(&buf[bufPos], numBlocks);

      // modificationTimeSecs
      bufPos += Serialization::serializeInt64(&buf[bufPos], modificationTimeSecs);

      // lastAccessTimeSecs
      bufPos += Serialization::serializeInt64(&buf[bufPos], lastAccessTimeSecs);

      // storageVersion
      bufPos += Serialization::serializeUInt64(&buf[bufPos], storageVersion);
   }
}

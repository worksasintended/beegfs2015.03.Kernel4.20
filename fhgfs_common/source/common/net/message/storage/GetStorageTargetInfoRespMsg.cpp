#include "GetStorageTargetInfoRespMsg.h"

bool GetStorageTargetInfoRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   // targetInfoList
   unsigned targetInfoListBufLen;

   if (!Serialization::deserializeStorageTargetInfoListPreprocess(&buf[bufPos], bufLen - bufPos,
      &targetInfoListStart, &targetInfoListElemNum, &targetInfoListBufLen))
      return false;

   bufPos += targetInfoListBufLen;

   return true;
}


void GetStorageTargetInfoRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // targetInfoList
   bufPos += Serialization::serializeStorageTargetInfoList(&buf[bufPos], targetInfoList);
}

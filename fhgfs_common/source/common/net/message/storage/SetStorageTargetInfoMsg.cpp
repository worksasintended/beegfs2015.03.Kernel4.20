#include "SetStorageTargetInfoMsg.h"

bool SetStorageTargetInfoMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   { // nodeType
      unsigned nodeTypeBufLen;

      if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos,
         &nodeType, &nodeTypeBufLen) )
         return false;

      bufPos += nodeTypeBufLen;
   }

   { // targetInfoList
      unsigned targetInfoListBufLen;

      if (!Serialization::deserializeStorageTargetInfoListPreprocess(&buf[bufPos], bufLen - bufPos,
               &targetInfoListStart, &targetInfoListElemNum, &targetInfoListBufLen))
         return false;

      bufPos += targetInfoListBufLen;
   }

   return true;
}

void SetStorageTargetInfoMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // nodeType
   bufPos += Serialization::serializeInt(&buf[bufPos], nodeType);

   // targetInfoList
   bufPos += Serialization::serializeStorageTargetInfoList(&buf[bufPos], targetInfoList);
}

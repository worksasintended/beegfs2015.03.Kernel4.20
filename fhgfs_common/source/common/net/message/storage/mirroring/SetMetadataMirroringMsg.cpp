#include "SetMetadataMirroringMsg.h"

void SetMetadataMirroringMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // entryInfo
   bufPos += entryInfoPtr->serialize(&buf[bufPos]);
}

bool SetMetadataMirroringMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   { // entryInfo
      unsigned entryBufLen;

      bool deserRes = entryInfo.deserialize(&buf[bufPos], bufLen-bufPos, &entryBufLen);
      if(unlikely(!deserRes) )
         return false;

      bufPos += entryBufLen;
   }

   return true;
}




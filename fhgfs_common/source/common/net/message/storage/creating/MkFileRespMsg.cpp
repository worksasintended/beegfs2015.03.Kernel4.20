#include <common/storage/StorageErrors.h>
#include "MkFileRespMsg.h"

bool MkFileRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   { // result

      unsigned resultBufLen;

      if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos,
         &result, &resultBufLen) )
         return false;

      bufPos += resultBufLen;
   }

   if ( (FhgfsOpsErr)result == FhgfsOpsErr_SUCCESS)
   { // entryInfo, empty object if result != FhgfsOpsErr_SUCCESS, deserialization would fail then
      unsigned entryLen;

      if(!this->entryInfo.deserialize(&buf[bufPos], bufLen-bufPos, &entryLen) )
         return false;

      bufPos += entryLen;
   }

   return true;
}

void MkFileRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // result
   bufPos += Serialization::serializeInt(&buf[bufPos], result);

   bufPos += this->entryInfoPtr->serialize(&buf[bufPos]);
}




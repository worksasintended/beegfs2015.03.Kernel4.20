#include <common/storage/StorageErrors.h>
#include "FindOwnerRespMsg.h"

bool FindOwnerRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   { // result
   
      unsigned resultBufLen;

      if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos,
         &result, &resultBufLen) )
         return false;

      bufPos += resultBufLen;
   }

   { // entryDepth
   
      unsigned ownerDepthBufLen;

      if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos,
         &entryDepth, &ownerDepthBufLen) )
         return false;

      bufPos += ownerDepthBufLen;
   }

   if ( (FhgfsOpsErr)result == FhgfsOpsErr_SUCCESS)
   { // entryInfo, empty object if result != FhgfsOpsErr_SUCCESS, deserialization would fail then
      unsigned entryInfoBufLen;
   
      if(!this->entryInfo.deserialize(&buf[bufPos], bufLen-bufPos, &entryInfoBufLen) )
         return false;

      bufPos += entryInfoBufLen;
   }

   return true;
}

void FindOwnerRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // result
   bufPos += Serialization::serializeInt(&buf[bufPos], this->result);
   
   // entryDepth
   bufPos += Serialization::serializeUInt(&buf[bufPos], this->entryDepth);

   // entryInfo
   bufPos += this->entryInfo.serialize(&buf[bufPos]);
}




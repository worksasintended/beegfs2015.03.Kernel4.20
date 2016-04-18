#include "SetMirrorBuddyGroupRespMsg.h"

void SetMirrorBuddyGroupRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // result
   bufPos += Serialization::serializeInt(&buf[bufPos], (int)result);

   // groupID
   bufPos += Serialization::serializeUInt16(&buf[bufPos], groupID);

   // padding for 4-byte alignment
   bufPos += Serialization::serialLenUInt16();
}

bool SetMirrorBuddyGroupRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   {
      // result
      int intResult;
      unsigned resultBufLen;

      if ( !Serialization::deserializeInt(&buf[bufPos], bufLen - bufPos, &intResult,
         &resultBufLen) )
         return false;

      bufPos += resultBufLen;
      this->result = (FhgfsOpsErr) intResult;
   }

   {
      // groupID
      unsigned groupIDBufLen;

      if ( !Serialization::deserializeUInt16(&buf[bufPos], bufLen - bufPos, &groupID,
         &groupIDBufLen) )
         return false;

      bufPos += groupIDBufLen;
   }

   // padding for 4-byte alignment (incrementing bufPos not needed, but to avoid human errors later)
   bufPos += Serialization::serialLenUInt16();

   return true;
}




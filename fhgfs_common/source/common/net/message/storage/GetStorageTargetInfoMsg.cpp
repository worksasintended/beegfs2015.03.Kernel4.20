#include "GetStorageTargetInfoMsg.h"

bool GetStorageTargetInfoMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   // targetIDs
   if (!Serialization::deserializeUInt16ListPreprocess(&buf[bufPos], bufLen - bufPos,
      &targetIDsElemNum, &targetIDsStart, &targetIDsLen))
      return false;

   bufPos += targetIDsLen;

   return true;
}


void GetStorageTargetInfoMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // targetIDs
   bufPos += Serialization::serializeUInt16List(&buf[bufPos], targetIDs);
}

#include "GetStorageResyncStatsRespMsg.h"

bool GetStorageResyncStatsRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   { // jobStats
      unsigned jobStatsBufLen;

      if ( !jobStats.deserialize(&buf[bufPos], bufLen-bufPos, &jobStatsBufLen) )
         return false;

      bufPos += jobStatsBufLen;
   }

   return true;
}

void GetStorageResyncStatsRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // jobStatsPtr
   bufPos += jobStatsPtr->serialize(&buf[bufPos]);
}

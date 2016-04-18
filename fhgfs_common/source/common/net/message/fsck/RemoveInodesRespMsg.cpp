#include "RemoveInodesRespMsg.h"

bool RemoveInodesRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   // failedEntryIDList
   {
      Serialization::deserializeStringListPreprocess(&buf[bufPos], bufLen - bufPos,
         &entryIDListElemNum, &entryIDListStart, &entryIDListBufLen);

      bufPos += entryIDListBufLen;
   }

   // entryTypeList
   {
      Serialization::deserializeIntListPreprocess(&buf[bufPos], bufLen - bufPos,
         &entryTypeListElemNum, &entryTypeListStart, &entryTypeListBufLen);

      bufPos += entryTypeListBufLen;
   }

   return true;
}

void RemoveInodesRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // failedEntryIDList
   bufPos += Serialization::serializeStringList(&buf[bufPos], failedEntryIDList);

   // entryTypeList
   bufPos += Serialization::serializeIntList(&buf[bufPos], entryTypeList);
}


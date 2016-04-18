#include "RemoveInodesMsg.h"

bool RemoveInodesMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   // entryIDList
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

void RemoveInodesMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // entryIDList
   bufPos += Serialization::serializeStringList(&buf[bufPos], entryIDList);

   // entryTypeList
   bufPos += Serialization::serializeIntList(&buf[bufPos], entryTypeList);
}


#include <common/app/log/LogContext.h>
#include "ListChunkDirIncrementalRespMsg.h"

void ListChunkDirIncrementalRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // result
   bufPos += Serialization::serializeInt(&buf[bufPos], this->result);

   // newOffset
   bufPos += Serialization::serializeInt64(&buf[bufPos], this->newOffset);

   // names
   bufPos += Serialization::serializeStringList(&buf[bufPos], this->names);

   // entryTypes
   bufPos += Serialization::serializeIntList(&buf[bufPos], this->entryTypes);
}

bool ListChunkDirIncrementalRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   {  // result
      unsigned resultBufLen;
      if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos, &this->result, &resultBufLen) )
         return false;

      bufPos += resultBufLen;
   }

   {  // newOffset
      unsigned newOffsetBufLen;

      if(!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos,
         &this->newOffset, &newOffsetBufLen) )
         return false;

      bufPos += newOffsetBufLen;
   }

   {  // names
      if(!Serialization::deserializeStringListPreprocess(&buf[bufPos], bufLen-bufPos,
         &this->namesElemNum, &this->namesListStart, &this->namesBufLen) )
         return false;

      bufPos += this->namesBufLen;
   }

   {  // entryTypes
      if(!Serialization::deserializeIntListPreprocess(&buf[bufPos], bufLen-bufPos,
         &this->entryTypesElemNum, &this->entryTypesListStart, &this->entryTypesBufLen) )
         return false;

      bufPos += this->entryTypesBufLen;
   }

   return true;
}



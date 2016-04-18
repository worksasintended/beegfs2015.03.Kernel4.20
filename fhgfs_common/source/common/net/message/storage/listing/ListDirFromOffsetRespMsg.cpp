#include <common/app/log/LogContext.h>
#include "ListDirFromOffsetRespMsg.h"

void ListDirFromOffsetRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // newServerOffset
   bufPos += Serialization::serializeInt64(&buf[bufPos], this->newServerOffset);

   // serverOffsets
   bufPos += Serialization::serializeInt64List(&buf[bufPos], this->serverOffsets);

   // result
   bufPos += Serialization::serializeInt(&buf[bufPos], this->result);

   // entryTypes
   bufPos += Serialization::serializeUInt8List(&buf[bufPos], this->entryTypes);

   // entryIDs
   bufPos += Serialization::serializeStringList(&buf[bufPos], this->entryIDs);

   // names
   bufPos += Serialization::serializeStringList(&buf[bufPos], this->names);
}

bool ListDirFromOffsetRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   {  // newServerOffset
      unsigned newServerOffsetBufLen;

      if(!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos,
         &this->newServerOffset, &newServerOffsetBufLen) )
         return false;

      bufPos += newServerOffsetBufLen;
   }

   {  // serverOffsets
      if(!Serialization::deserializeInt64ListPreprocess(&buf[bufPos], bufLen-bufPos,
         &this->serverOffsetsElemNum, &this->serverOffsetsListStart, &this->serverOffsetsBufLen) )
         return false;

      bufPos += this->serverOffsetsBufLen;
   }

   {  // result
      unsigned fieldLen;
      if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos, &this->result, &fieldLen) )
         return false;

      bufPos += fieldLen;
   }

   {  // entryTypes
      if(!Serialization::deserializeUInt8ListPreprocess(&buf[bufPos], bufLen-bufPos,
         &this->entryTypesElemNum, &this->entryTypesListStart, &this->entryTypesBufLen) )
         return false;

      bufPos += this->entryTypesBufLen;
   }

   {  // entryIDs
      if (!Serialization::deserializeStringListPreprocess(&buf[bufPos], bufLen-bufPos,
         &this->entryIDsElemNum, &this->entryIDsListStart, &this->entryIDsBufLen) )
         return false;

      bufPos += this->entryIDsBufLen;
   }

   {  // names
      if(!Serialization::deserializeStringListPreprocess(&buf[bufPos], bufLen-bufPos,
         &this->namesElemNum, &this->namesListStart, &this->namesBufLen) )
         return false;

      bufPos += this->namesBufLen;
   }

   if(unlikely(
      (this->entryTypesElemNum != this->namesElemNum) ||
      (this->entryTypesElemNum != this->entryIDsElemNum) ||
      (this->entryTypesElemNum != this->serverOffsetsElemNum) ) ) // check for equal list lengths
   {
	   LogContext(__func__).log(Log_WARNING,
	      "Sanity check failed (number of list elements does not match!");
	   LogContext(__func__).logBacktrace();
      return false;
   }

   return true;
}



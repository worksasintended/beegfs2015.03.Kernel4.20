#include "ListDirFromOffsetRespMsg.h"


fhgfs_bool ListDirFromOffsetRespMsg_deserializePayload(NetMessage* this, const char* buf,
   size_t bufLen)
{
   const char* logContext = "ListDirFromOffsetRespMsg deserialization";
   ListDirFromOffsetRespMsg* thisCast = (ListDirFromOffsetRespMsg*)this;

   size_t bufPos = 0;
   
   unsigned resultFieldLen;
   unsigned newServerOffsetBufLen;

   {  // newServerOffset
      if(!Serialization_deserializeInt64(&buf[bufPos], bufLen-bufPos,
         &thisCast->newServerOffset, &newServerOffsetBufLen) )
         return fhgfs_false;

      bufPos += newServerOffsetBufLen;
   }

   {  // serverOffsets
      if(!Serialization_deserializeInt64CpyVecPreprocess(&buf[bufPos], bufLen-bufPos,
         &thisCast->serverOffsetsElemNum, &thisCast->serverOffsetsListStart,
         &thisCast->serverOffsetsBufLen) )
         return fhgfs_false;

      bufPos += thisCast->serverOffsetsBufLen;
   }

   {  // result
      if(!Serialization_deserializeInt(&buf[bufPos], bufLen-bufPos, &thisCast->result,
         &resultFieldLen) )
         return fhgfs_false;

      bufPos += resultFieldLen;
   }

   {  // entryTypes
      if(!Serialization_deserializeUInt8VecPreprocess(&buf[bufPos], bufLen-bufPos,
         &thisCast->entryTypesElemNum, &thisCast->entryTypesListStart, &thisCast->entryTypesBufLen) )
         return fhgfs_false;

      bufPos += thisCast->entryTypesBufLen;
   }

   {  // entryIDs
      if (!Serialization_deserializeStrCpyVecPreprocess(&buf[bufPos], bufLen-bufPos,
         &thisCast->entryIDsElemNum, &thisCast->entryIDsListStart, &thisCast->entryIDsBufLen) )
         return fhgfs_false;

      bufPos += thisCast->entryIDsBufLen;
   }

   {  // names
      if(!Serialization_deserializeStrCpyVecPreprocess(&buf[bufPos], bufLen-bufPos,
         &thisCast->namesElemNum, &thisCast->namesListStart, &thisCast->namesBufLen) )
         return fhgfs_false;

      bufPos += thisCast->namesBufLen;
   }

   // sanity check for equal list lengths
   if(unlikely(
      (thisCast->entryTypesElemNum != thisCast->namesElemNum) ||
      (thisCast->entryTypesElemNum != thisCast->entryIDsElemNum) ||
      (thisCast->entryTypesElemNum != thisCast->serverOffsetsElemNum) ) )
   {
      printk_fhgfs(KERN_INFO, "%s: Sanity check failed (number of list elements does not match)!\n",
         logContext);
      return fhgfs_false;
   }


   return fhgfs_true;
}

#include "Serialization.h"

#include <common/storage/StorageTargetInfo.h>

unsigned Serialization::serializeStorageTargetInfoList(char* buf,
   StorageTargetInfoList *storageTargetInfoList)
{
   unsigned storageTargetInfoListSize = storageTargetInfoList->size();

   size_t bufPos = 0;

   // elem count info field
   bufPos += serializeUInt(&buf[bufPos], storageTargetInfoListSize);

   // serialize each element of the list
   StorageTargetInfoListIter iter = storageTargetInfoList->begin();

   for (unsigned i = 0; i < storageTargetInfoListSize; i++, iter++)
   {
      StorageTargetInfo element = *iter;

      bufPos += element.serialize(&buf[bufPos]);
   }

   return bufPos;
}

unsigned Serialization::serialLenStorageTargetInfoList(StorageTargetInfoList* storageTargetInfoList)
{
   unsigned storageTargetInfoListSize = storageTargetInfoList->size();

   size_t bufPos = 0;

   bufPos += serialLenUInt();

   // serialize each element of the list
   StorageTargetInfoListIter iter = storageTargetInfoList->begin();

   for (unsigned i = 0; i < storageTargetInfoListSize; i++, iter++)
   {
      StorageTargetInfo element = *iter;

      bufPos += element.serialLen();
   }

   return bufPos;
}

bool Serialization::deserializeStorageTargetInfoListPreprocess(const char* buf, size_t bufLen,
   const char** outInfoStart, unsigned *outElemNum, unsigned* outLen)
{
   // Note: This is a fairly inefficient implementation (requiring redundant pre-processing),
   size_t bufPos = 0;

   unsigned elemNumFieldLen;

   if (unlikely(!deserializeUInt(&buf[bufPos], bufLen-bufPos, outElemNum,
         &elemNumFieldLen) ))
      return false;

   bufPos += elemNumFieldLen;

   *outInfoStart = &buf[bufPos];

   for (unsigned i = 0; i < *outElemNum; i++)
   {
      StorageTargetInfo element;
      unsigned elementBufLen;

      if (unlikely(!element.deserialize(&buf[bufPos], bufLen-bufPos, &elementBufLen)))
         return false;

      bufPos += elementBufLen;
   }

   *outLen = bufPos;

   return true;
}

bool Serialization::deserializeStorageTargetInfoList(unsigned storageTargetInfoListElemNum,
   const char* storageTargetInfoListStart, StorageTargetInfoList* outList)
{
   size_t bufPos = 0;
   size_t bufLen = ~0; // fake bufLen to max value (has already been verified during pre-processing)
   const char* buf = storageTargetInfoListStart;

   for (unsigned i = 0; i < storageTargetInfoListElemNum; i++)
   {
      StorageTargetInfo element;
      unsigned elementBufLen;

      if (unlikely(!element.deserialize(&buf[bufPos], bufLen-bufPos, &elementBufLen)))
         return false;

      bufPos += elementBufLen;

      outList->push_back(element);
   }

   return true;
}

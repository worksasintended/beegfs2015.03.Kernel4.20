/*
 * Serialization of QuotaDataList
 */

#include "Serialization.h"


unsigned Serialization::serialLenQuotaDataList(QuotaDataList* quotaDataList)
{
   size_t bufPos = 0;

   bufPos += serialLenUInt();
   bufPos += quotaDataList->size() * QuotaData::serialLen();

   return bufPos;
}

unsigned Serialization::serializeQuotaDataList(char* buf, QuotaDataList* quotaDataList)
{
   unsigned quotaDataListSize = quotaDataList->size();

   size_t bufPos = 0;

   // elem count info field

   bufPos += serializeUInt(&buf[bufPos], quotaDataListSize);

   // serialize each element of the entryInfoList

   for (QuotaDataListIter iter = quotaDataList->begin(); iter != quotaDataList->end(); iter++)
   {
      QuotaData& quotaData = *iter;

      bufPos += quotaData.serialize(&buf[bufPos]);
   }

   return bufPos;
}

bool Serialization::deserializeQuotaDataListPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outListBufLen)
{
   size_t bufPos = 0;

   // elem count info field
   unsigned elemNumFieldLen;

   if ( unlikely(!deserializeUInt(&buf[bufPos], bufLen-bufPos, outElemNum,
         &elemNumFieldLen) ) )
      return false;

   bufPos += elemNumFieldLen;

   *outListStart = &buf[bufPos];

   // calculate size of the list content and add the size to the buffer position pointer
   unsigned listElementsLen = *outElemNum * QuotaData::serialLen();
   bufPos += listElementsLen;

   // check buffer length
   if(unlikely(bufPos > bufLen) )
      return false;

   *outListBufLen = bufPos;

   return true;
}

void Serialization::deserializeQuotaDataList(unsigned listBufLen, unsigned elemNum,
   const char* listStart, QuotaDataList* outList)
{
   const char* buf = listStart;
   size_t bufPos = 0;
   size_t bufLen = ~0;

   for(unsigned i=0; i < elemNum; i++)
   {
      QuotaData quotaData;
      unsigned quotaDataLen;

      quotaData.deserialize(&buf[bufPos], bufLen-bufPos, &quotaDataLen);

      bufPos += quotaDataLen;

      outList->push_back(quotaData);
   }
}

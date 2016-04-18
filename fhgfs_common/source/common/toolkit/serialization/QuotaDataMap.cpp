/*
 * Serialization of QuotaDataMap
 */


#include "Serialization.h"


unsigned Serialization::serialLenQuotaDataMap(QuotaDataMap* quotaDataMap)
{
   size_t bufPos = 0;

   bufPos += serialLenUInt();
   bufPos += quotaDataMap->size() * QuotaData::serialLen();

   return bufPos;
}

unsigned Serialization::serializeQuotaDataMap(char* buf, QuotaDataMap* quotaDataMap)
{
   unsigned quotaDataMapSize = quotaDataMap->size();

   size_t bufPos = 0;

   // elem count info field

   bufPos += serializeUInt(&buf[bufPos], quotaDataMapSize);

   // serialize only each value of the QuotaDataMap, because the value contains the key

   for (QuotaDataMapIter iter = quotaDataMap->begin(); iter != quotaDataMap->end(); iter++)
   {
      QuotaData quotaData = iter->second;

      bufPos += quotaData.serialize(&buf[bufPos]);
   }

   return bufPos;
}

bool Serialization::deserializeQuotaDataMapPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outMapStart, unsigned* outMapBufLen)
{
   size_t bufPos = 0;

   // elem count info field
   {
      unsigned elemNumFieldLen;

      if ( unlikely(!deserializeUInt(&buf[bufPos], bufLen-bufPos, outElemNum,
            &elemNumFieldLen) ) )
         return false;

      bufPos += elemNumFieldLen;
   }

   *outMapStart = &buf[bufPos];

   // calculate size of the map content and add the size to the buffer position pointer
   unsigned mapElementsLen = *outElemNum * QuotaData::serialLen();
   bufPos += mapElementsLen;

   // check buffer length
   if(unlikely(bufPos > bufLen) )
      return false;

   *outMapBufLen = bufPos;

   return true;
}

bool Serialization::deserializeQuotaDataMap(unsigned mapBufLen, unsigned elemNum,
   const char* mapStart, QuotaDataMap* outQuotaDataMap)
{
   const char* buf = mapStart;
   size_t bufPos = 0;
   size_t bufLen = ~0;

   for(unsigned i=0; i < elemNum; i++)
   {
      QuotaData quotaData;
      unsigned quotaDataLen;

      if ( unlikely(!quotaData.deserialize(&buf[bufPos], bufLen-bufPos, &quotaDataLen) ) )
         return false;

      bufPos += quotaDataLen;

      outQuotaDataMap->insert(QuotaDataMapVal(quotaData.getID(), quotaData));
   }

   return true;
}

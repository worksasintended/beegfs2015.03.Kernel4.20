/*
 * Serialization of QuotaDataMapForTarget
 */


#include "Serialization.h"


unsigned Serialization::serialLenQuotaDataMapForTarget(QuotaDataMapForTarget* quotaDataMapForTarget)
{
   size_t bufPos = 0;

   bufPos += serialLenUInt();                                     // totalBufLen info field

   bufPos += serialLenUInt();                                     // number of target maps

   for(QuotaDataMapForTargetIter iter = quotaDataMapForTarget->begin();
      iter != quotaDataMapForTarget->end(); iter++)
   {
      bufPos += Serialization::serialLenUShort();                            // the targetNumID
      bufPos += Serialization::serialLenQuotaDataMap(&iter->second);         // the QuotaDataMap
   }

   return bufPos;
}

unsigned Serialization::serializeQuotaDataMapForTarget(char* buf,
   QuotaDataMapForTarget* quotaDataMapForTarget)
{
   size_t bufPos = 0;

   unsigned requiredLen = serialLenQuotaDataMapForTarget(quotaDataMapForTarget);
   unsigned quotaDataMapForTargetsSize = quotaDataMapForTarget->size();


   // totalBufLen info field

   bufPos += serializeUInt(&buf[bufPos], requiredLen);


   // number of target maps

   bufPos += serializeUInt(&buf[bufPos], quotaDataMapForTargetsSize);


   for(QuotaDataMapForTargetIter iter = quotaDataMapForTarget->begin();
      iter != quotaDataMapForTarget->end(); iter++)
   {
      // the targetNumID of the target QuotaDataMap

      bufPos += serializeUShort(&buf[bufPos], iter->first);


      // serialize the QuotaDataMap of the target

      bufPos += Serialization::serializeQuotaDataMap(&buf[bufPos], &iter->second);
   }

   return bufPos;
}

bool Serialization::deserializeQuotaDataMapForTargetPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outMapStart, unsigned* outMapBufLen)
{
   size_t bufPos = 0;

   // totalBufLen info field
   unsigned bufLenFieldLen;
   if(unlikely(!deserializeUInt(&buf[bufPos], bufLen-bufPos, outMapBufLen, &bufLenFieldLen) ) )
      return false;
   bufPos += bufLenFieldLen;

   // elem count field
   unsigned elemNumFieldLen;
   if(unlikely(!deserializeUInt(&buf[bufPos], bufLen-bufPos, outElemNum, &elemNumFieldLen) ) )
      return false;
   bufPos += elemNumFieldLen;

   *outMapStart = &buf[bufPos];

   if(unlikely(*outMapBufLen > bufLen) )
      return false;

   return true;
}

bool Serialization::deserializeQuotaDataMapForTarget(unsigned mapBufLen, unsigned elemNum,
   const char* mapStart, QuotaDataMapForTarget* outQuotaDataMapForTarget)
{
   const char* buf = mapStart;
   size_t bufLen = mapBufLen;
   size_t bufPos = 0;

   for(unsigned i=0; i < elemNum; i++)
   {
      QuotaDataMap quotaDataMap;
      const char* outTargetMapStart = 0;
      unsigned outLen;
      unsigned outElemNum;


      // the targetNumID of the target QuotaDataMap

      uint16_t outTargetNumID = 0;
      unsigned targetNumIDLen = 0; // just mute compiler warnings on sl5
      if(unlikely(!deserializeUShort(&buf[bufPos], bufLen-bufPos, &outTargetNumID,
         &targetNumIDLen) ) )
         return false;
      bufPos += targetNumIDLen;


      // deserialize the QuotaDataMap of the target

      if (unlikely(!Serialization::deserializeQuotaDataMapPreprocess(&buf[bufPos], bufLen-bufPos,
         &outElemNum, &outTargetMapStart, &outLen) ) )
         return false;

      if (unlikely(!Serialization::deserializeQuotaDataMap(bufLen-bufPos, outElemNum,
         outTargetMapStart, &quotaDataMap) ) )
         return false;

      bufPos += outLen;

      outQuotaDataMapForTarget->insert(QuotaDataMapForTargetMapVal(outTargetNumID, quotaDataMap) );
   }

   return true;
}

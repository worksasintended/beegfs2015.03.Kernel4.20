#include "Serialization.h"

#include <common/nodes/TargetStateInfo.h>

unsigned Serialization::serializeTargetStateInfoMap(char* buf,
      TargetStateInfoMap* targetStateInfoMap)
{
   size_t bufPos = 0;
   unsigned elemCount = targetStateInfoMap->size();
   bufPos += serializeUInt(&buf[bufPos], elemCount);

   for (TargetStateInfoMapIter it = targetStateInfoMap->begin();
        it != targetStateInfoMap->end(); ++it)
      bufPos += serializeTargetStateInfoMapElement(&buf[bufPos], *it);

   return bufPos;
}

unsigned Serialization::serialLenTargetStateInfoMap(TargetStateInfoMap* targetStateInfoMap)
{
   size_t bufPos = 0;
   bufPos += serialLenUInt(); // elemCount

   bufPos += targetStateInfoMap->size() * serialLenTargetStateInfoMapElem();

   return bufPos;
}

bool Serialization::deserializeTargetStateInfoMapPreprocess(const char* buf, size_t bufLen,
   const char** outInfoStart, unsigned *outElemNum, unsigned* outLen)
{
   // Note: This is a fairly inefficient implementation (requiring redundant pre-processing),
   size_t bufPos = 0;
   unsigned elemNumFieldLen;
   if (!deserializeUInt(&buf[bufPos], bufLen-bufPos, outElemNum, &elemNumFieldLen))
      return false;

   bufPos += elemNumFieldLen;

   *outInfoStart = &buf[bufPos];

   for (unsigned i = 0; i < *outElemNum; i++)
   {
      std::pair<uint16_t, TargetStateInfo> element;
      unsigned elementBufLen;

      if (!deserializeTargetStateInfoMapElement(&buf[bufPos], bufLen-bufPos, &element,
               &elementBufLen))
         return false;

      bufPos += elementBufLen;
   }

   *outLen = bufPos;
   return true;
}

bool Serialization::deserializeTargetStateInfoMap(unsigned targetStateInfoMapElemNum,
   const char* targetStateInfoMapStart, TargetStateInfoMap* outMap)
{
   size_t bufPos = 0;
   size_t bufLen = ~0; // fake bufLen to max value (has already been verified during pre-processing)
   const char* buf = targetStateInfoMapStart;

   for (unsigned i = 0; i < targetStateInfoMapElemNum; i++)
   {
      std::pair<uint16_t, TargetStateInfo> element;
      unsigned elementBufLen;

      if (!deserializeTargetStateInfoMapElement(&buf[bufPos], bufLen-bufPos, &element,
               &elementBufLen))
         return false;

      bufPos += elementBufLen;
      outMap->insert(element);
   }

   return true;
}

unsigned Serialization::serializeTargetStateInfoMapElement(char* buf,
      const std::pair<uint16_t, TargetStateInfo>& value)
{
   size_t bufPos = 0;

   // ID
   bufPos += Serialization::serializeUInt16(&buf[bufPos], value.first);
   // reachabilityState
   bufPos += Serialization::serializeInt(&buf[bufPos], value.second.reachabilityState);
   // consistencyState
   bufPos += Serialization::serializeInt(&buf[bufPos], value.second.consistencyState);
   // lastChangedTime
   bufPos += Serialization::serializeInt64(&buf[bufPos], value.second.lastChangedTime.now.tv_sec);
   bufPos += Serialization::serializeInt64(&buf[bufPos], value.second.lastChangedTime.now.tv_nsec);

   return bufPos;
}

bool Serialization::deserializeTargetStateInfoMapElement(const char* buf, size_t bufLen,
      std::pair<uint16_t, TargetStateInfo>* outValue, unsigned* outLen)
{
   unsigned bufPos = 0;

   // ID
   {
      unsigned idBufLen;
      if (!Serialization::deserializeUInt16(&buf[bufPos], bufLen-bufPos, &outValue->first,
            &idBufLen))
         return false;
      bufPos += idBufLen;
   }

   // reachabilityState
   {
      unsigned reachabilityStateBufLen;
      int reachabilityStateInt;
      if (!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos,
               &reachabilityStateInt, &reachabilityStateBufLen))
         return false;
      outValue->second.reachabilityState = (TargetReachabilityState)reachabilityStateInt;
      bufPos += reachabilityStateBufLen;
   }

   // consistencyState
   {
      unsigned consistencyStateBufLen;
      int consistencyStateInt;
      if (!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos,
               &consistencyStateInt, &consistencyStateBufLen))
         return false;
      outValue->second.consistencyState = (TargetConsistencyState)consistencyStateInt;
      bufPos += consistencyStateBufLen;
   }

   // lastChangedTime
   {
      unsigned secBufLen;
      unsigned nsBufLen;
      int64_t sec;
      int64_t nsec;

      if (!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos,
               &sec, &secBufLen))
         return false;

      outValue->second.lastChangedTime.now.tv_sec = sec;

      bufPos += secBufLen;

      if (!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos,
               &nsec, &nsBufLen))
         return false;

      outValue->second.lastChangedTime.now.tv_nsec = nsec;
      bufPos += nsBufLen;
   }

   *outLen = bufPos;
   return true;
}

unsigned Serialization::serialLenTargetStateInfoMapElem()
{
   size_t bufPos = 0;

   // ID
   bufPos += Serialization::serialLenUInt16();
   // reachabilityState
   bufPos += Serialization::serialLenInt();
   // consistencyState
   bufPos += Serialization::serialLenInt();
   // lastChangedTime
   bufPos += Serialization::serialLenInt64();
   bufPos += Serialization::serialLenInt64();

   return bufPos;
}

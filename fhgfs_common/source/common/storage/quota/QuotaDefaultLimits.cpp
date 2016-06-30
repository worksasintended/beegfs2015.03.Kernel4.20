#include <common/toolkit/serialization/Serialization.h>
#include "QuotaDefaultLimits.h"



unsigned QuotaDefaultLimits::serialLen()
{
   return Serialization::serialLenUInt64() +    // defaultUserQuotaInodes
          Serialization::serialLenUInt64() +    // defaultUserQuotaSize
          Serialization::serialLenUInt64() +    // defaultGroupQuotaInodes
          Serialization::serialLenUInt64();     // defaultGroupQuotaSize

}

size_t QuotaDefaultLimits::serialize(char* buf)
{
   size_t bufPos = 0;

   // defaultUserQuotaInodes
   bufPos += Serialization::serializeUInt64(&buf[bufPos], defaultUserQuotaInodes);

   // defaultUserQuotaSize
   bufPos += Serialization::serializeUInt64(&buf[bufPos], defaultUserQuotaSize);

   // defaultGroupQuotaInodes
   bufPos += Serialization::serializeUInt64(&buf[bufPos], defaultGroupQuotaInodes);

   // defaultGroupQuotaSize
   bufPos += Serialization::serializeUInt64(&buf[bufPos], defaultGroupQuotaSize);

   return bufPos;
}

bool QuotaDefaultLimits::deserialize(const char* buf, size_t bufLen, unsigned* outLen)
{
   size_t bufPos = 0;

   // defaultUserQuotaInodes

   unsigned defaultUserQuotaInodesBufLen;

   if(!Serialization::deserializeUInt64(&buf[bufPos], bufLen-bufPos, &defaultUserQuotaInodes,
      &defaultUserQuotaInodesBufLen) )
      return false;

   bufPos += defaultUserQuotaInodesBufLen;

   // defaultUserQuotaSize

   unsigned defaultUserQuotaSizeBufLen;

   if(!Serialization::deserializeUInt64(&buf[bufPos], bufLen-bufPos, &defaultUserQuotaSize,
      &defaultUserQuotaSizeBufLen) )
      return false;

   bufPos += defaultUserQuotaSizeBufLen;

   // defaultGroupQuotaInodes

   unsigned defaultGroupQuotaInodesBufLen;

   if(!Serialization::deserializeUInt64(&buf[bufPos], bufLen-bufPos, &defaultGroupQuotaInodes,
      &defaultGroupQuotaInodesBufLen) )
      return false;

   bufPos += defaultGroupQuotaInodesBufLen;

   // defaultGroupQuotaSize

   unsigned defaultGroupQuotaSizeBufLen;

   if(!Serialization::deserializeUInt64(&buf[bufPos], bufLen-bufPos, &defaultGroupQuotaSize,
      &defaultGroupQuotaSizeBufLen) )
      return false;

   bufPos += defaultGroupQuotaSizeBufLen;


   *outLen = bufPos;

   return true;
}

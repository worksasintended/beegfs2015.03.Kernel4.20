#include "FsckTargetID.h"

#include <common/toolkit/serialization/Serialization.h>

/**
 * Serialize into outBuf
 */
size_t FsckTargetID::serialize(char* outBuf)
{
   size_t bufPos = 0;

   // id
   bufPos += Serialization::serializeUInt16(&outBuf[bufPos], id);

   // targetIDType
   bufPos += Serialization::serializeInt(&outBuf[bufPos], (int)targetIDType);

   return bufPos;
}

/**
 * deserialize the given buffer
 */
bool FsckTargetID::deserialize(const char* buf, size_t bufLen, unsigned* outLen)
{
   unsigned bufPos = 0;

   uint16_t id;
   int targetIDType;

   {
      // id
      unsigned idBufLen;

      if(!Serialization::deserializeUInt16(&buf[bufPos], bufLen-bufPos,
         &id, &idBufLen) )
         return false;

      bufPos += idBufLen;
   }

   {
      // targetIDType
      unsigned targetIDTypeBufLen;

      if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos,
         &targetIDType, &targetIDTypeBufLen) )
         return false;

      bufPos += targetIDTypeBufLen;
   }

   this->update(id, (FsckTargetIDType)targetIDType);

   *outLen = bufPos;

   return true;
}

/**
 * Required size for serialization
 */
unsigned FsckTargetID::serialLen(void)
{
   unsigned length =
      Serialization::serialLenUInt16() + // id
      Serialization::serialLenInt();    // targetIDType

      return length;
}

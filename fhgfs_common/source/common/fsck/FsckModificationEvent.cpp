#include "FsckModificationEvent.h"

#include <common/toolkit/serialization/Serialization.h>

/**
 * Serialize into outBuf
 */
size_t FsckModificationEvent::serialize(char* outBuf)
{
   size_t bufPos = 0;

   // eventType
   bufPos += Serialization::serializeUInt8(&outBuf[bufPos], (uint8_t)this->getEventType());

   // entryID
   bufPos += Serialization::serializeStrAlign4(&outBuf[bufPos], this->getEntryID().length(),
      this->getEntryID().c_str());

   return bufPos;
}

/**
 * deserialize the given buffer
 */
bool FsckModificationEvent::deserialize(const char* buf, size_t bufLen, unsigned* outLen)
{
   unsigned bufPos = 0;

   ModificationEventType eventType;
   std::string entryID;

   {
      // eventType
      uint8_t intEventType;
      unsigned eventTypeBufLen;

      if(!Serialization::deserializeUInt8(&buf[bufPos], bufLen-bufPos, &intEventType,
         &eventTypeBufLen))
         return false;

      eventType = (ModificationEventType)intEventType;

      bufPos += eventTypeBufLen;
   }

   {
      // id
      unsigned entryIDBufLen;
      const char* entryIDChar;
      unsigned entryIDLen;

      if(!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &entryIDLen, &entryIDChar, &entryIDBufLen) )
         return false;

      entryID.assign(entryIDChar, entryIDLen);
      bufPos += entryIDBufLen;
   }

   this->update(eventType, entryID);
   *outLen = bufPos;

   return true;
}

/**
 * Required size for serialization
 */
unsigned FsckModificationEvent::serialLen()
{
   unsigned length =
      Serialization::serialLenUInt8() + // eventType
      Serialization::serialLenStrAlign4(this->getEntryID().length() ); // entryID

      return length;
}

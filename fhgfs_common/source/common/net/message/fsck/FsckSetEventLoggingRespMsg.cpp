#include "FsckSetEventLoggingRespMsg.h"

bool FsckSetEventLoggingRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   {
      // result
      unsigned resultBufLen;

      if ( !Serialization::deserializeBool(&buf[bufPos], bufLen - bufPos,
         &(this->result), &resultBufLen) )
         return false;

      bufPos += resultBufLen;
   }

   {
      // missedEvents
      unsigned missedEventsBufLen;

      if ( !Serialization::deserializeBool(&buf[bufPos], bufLen - bufPos,
         &(this->missedEvents), &missedEventsBufLen) )
         return false;

      bufPos += missedEventsBufLen;
   }

   return true;
}

void FsckSetEventLoggingRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // result
   bufPos += Serialization::serializeBool(&buf[bufPos], this->result);

   // missedEvents
   bufPos += Serialization::serializeBool(&buf[bufPos], this->missedEvents);
}

#include "SetLastBuddyCommOverrideMsg.h"

bool SetLastBuddyCommOverrideMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   {
      // targetID
      unsigned targetIDBufLen;

      if ( !Serialization::deserializeUInt16(&buf[bufPos], bufLen - bufPos, &targetID,
         &targetIDBufLen) )
         return false;

      bufPos += targetIDBufLen;
   }
   
   {
      // timestamp
      unsigned timestampBufLen;

      if ( !Serialization::deserializeInt64(&buf[bufPos], bufLen - bufPos, &timestamp,
         &timestampBufLen) )
         return false;

      bufPos += timestampBufLen;
   }
   {
      // abortResync
      unsigned abortResyncBufLen;

      if ( !Serialization::deserializeBool(&buf[bufPos], bufLen - bufPos, &abortResync,
         &abortResyncBufLen) )
         return false;

      bufPos += abortResyncBufLen;
   }



   return true;   
}

void SetLastBuddyCommOverrideMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // targetID
   bufPos += Serialization::serializeUInt16(&buf[bufPos], targetID);
   
   // timestamp
   bufPos += Serialization::serializeInt64(&buf[bufPos], timestamp);

   // abortResync
   bufPos += Serialization::serializeBool(&buf[bufPos], abortResync);
}


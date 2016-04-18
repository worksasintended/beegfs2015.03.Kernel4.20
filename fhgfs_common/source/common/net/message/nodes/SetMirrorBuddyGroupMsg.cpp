#include "SetMirrorBuddyGroupMsg.h"

void SetMirrorBuddyGroupMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // primaryTargetID
   bufPos += Serialization::serializeUInt16(&buf[bufPos], primaryTargetID);

   // secondaryTargetID
   bufPos += Serialization::serializeUInt16(&buf[bufPos], secondaryTargetID);

   // buddyGroupID
   bufPos += Serialization::serializeUInt16(&buf[bufPos], buddyGroupID);

   // allowUpdate
   bufPos += Serialization::serializeBool(&buf[bufPos], allowUpdate);

   // ackID
   bufPos += Serialization::serializeStr(&buf[bufPos], ackIDLen, ackID);
}

bool SetMirrorBuddyGroupMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   {
      // primaryTargetID
      unsigned idBufLen;

      if ( !Serialization::deserializeUInt16(&buf[bufPos], bufLen - bufPos, &primaryTargetID,
         &idBufLen) )
         return false;

      bufPos += idBufLen;
   }

   {
      // secondaryTargetID
      unsigned idBufLen;

      if ( !Serialization::deserializeUInt16(&buf[bufPos], bufLen - bufPos, &secondaryTargetID,
         &idBufLen) )
         return false;

      bufPos += idBufLen;
   }

   {
      // buddyGroupID
      unsigned idBufLen;

      if ( !Serialization::deserializeUInt16(&buf[bufPos], bufLen - bufPos, &buddyGroupID,
         &idBufLen) )
         return false;

      bufPos += idBufLen;
   }

   {
      // allowUpdate
      unsigned allowUpdateLen;

      if ( !Serialization::deserializeBool(&buf[bufPos], bufLen - bufPos, &allowUpdate,
         &allowUpdateLen) )
         return false;

      bufPos += allowUpdateLen;
   }

   {
      // ackID
      unsigned ackBufLen;

      if ( !Serialization::deserializeStr(&buf[bufPos], bufLen - bufPos, &ackIDLen, &ackID,
         &ackBufLen) )
         return false;

      bufPos += ackBufLen;
   }

   return true;
}




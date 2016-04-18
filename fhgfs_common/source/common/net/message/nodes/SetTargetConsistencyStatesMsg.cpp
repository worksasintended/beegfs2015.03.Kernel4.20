#include "SetTargetConsistencyStatesMsg.h"

void SetTargetConsistencyStatesMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // targetIDs
   bufPos += Serialization::serializeUInt16List(&buf[bufPos], targetIDs);

   // states
   bufPos += Serialization::serializeUInt8List(&buf[bufPos], states);

   // ackID
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], ackIDLen, ackID);

   // setOnline
   bufPos += Serialization::serializeBool(&buf[bufPos], setOnline);
}

bool SetTargetConsistencyStatesMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   {
      // targetIDs

      if ( !Serialization::deserializeUInt16ListPreprocess(&buf[bufPos], bufLen - bufPos,
           &targetIDsElemNum, &targetIDsListStart, &targetIDsBufLen) )
         return false;

      bufPos += targetIDsBufLen;
   }

   {
      // states
      if ( !Serialization::deserializeUInt8ListPreprocess(&buf[bufPos], bufLen - bufPos,
           &statesElemNum, &statesListStart, &statesBufLen) )
         return false;

      bufPos += statesBufLen;
   }

   { // ackID
      unsigned ackBufLen;

      if ( !Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
           &ackIDLen, &ackID, &ackBufLen) )
         return false;

      bufPos += ackBufLen;
   }

   { // setOnline
      unsigned setOnlineLen;

      if ( !Serialization::deserializeBool(&buf[bufPos], bufLen-bufPos, &this->setOnline,
           &setOnlineLen) )
         return false;

      bufPos += setOnlineLen;
   }

   return true;
}

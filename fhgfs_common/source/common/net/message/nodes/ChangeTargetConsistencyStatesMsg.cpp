#include "ChangeTargetConsistencyStatesMsg.h"

void ChangeTargetConsistencyStatesMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // nodeType
   bufPos += Serialization::serializeInt(&buf[bufPos], nodeType);

   // targetIDs
   bufPos += Serialization::serializeUInt16List(&buf[bufPos], targetIDs);

   // oldStates
   bufPos += Serialization::serializeUInt8List(&buf[bufPos], oldStates);

   // newStates
   bufPos += Serialization::serializeUInt8List(&buf[bufPos], newStates);

   // ackID
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], ackIDLen, ackID);
}

bool ChangeTargetConsistencyStatesMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   { // nodeType
      unsigned nodeTypeBufLen;

      if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos, &nodeType, &nodeTypeBufLen) )
         return false;

      bufPos += nodeTypeBufLen;
   }

   {
      // targetIDs
      if ( !Serialization::deserializeUInt16ListPreprocess(&buf[bufPos], bufLen - bufPos,
           &targetIDsElemNum, &targetIDsListStart, &targetIDsBufLen) )
         return false;

      bufPos += targetIDsBufLen;
   }

   {
      // oldStates
      if ( !Serialization::deserializeUInt8ListPreprocess(&buf[bufPos], bufLen - bufPos,
           &oldStatesElemNum, &oldStatesListStart, &oldStatesBufLen) )
         return false;

      bufPos += oldStatesBufLen;
   }

   {
      // newStates
      if ( !Serialization::deserializeUInt8ListPreprocess(&buf[bufPos], bufLen - bufPos,
           &newStatesElemNum, &newStatesListStart, &newStatesBufLen) )
         return false;

      bufPos += newStatesBufLen;
   }

   { // ackID
      unsigned ackBufLen;

      if ( !Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
           &ackIDLen, &ackID, &ackBufLen) )
         return false;

      bufPos += ackBufLen;
   }

   return true;
}

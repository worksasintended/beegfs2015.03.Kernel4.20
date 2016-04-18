#include "GetTargetStatesRespMsg.h"

void GetTargetStatesRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // targetIDs
   bufPos += Serialization::serializeUInt16List(&buf[bufPos], targetIDs);

   // reachabilityStates
   bufPos += Serialization::serializeUInt8List(&buf[bufPos], reachabilityStates);

   // states
   bufPos += Serialization::serializeUInt8List(&buf[bufPos], consistencyStates);
}

bool GetTargetStatesRespMsg::deserializePayload(const char* buf, size_t bufLen)
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
      // reachabilityStates
      if ( !Serialization::deserializeUInt8ListPreprocess(&buf[bufPos], bufLen - bufPos,
         &reachabilityStatesElemNum, &reachabilityStatesListStart, &reachabilityStatesBufLen) )
         return false;

      bufPos += reachabilityStatesBufLen;
   }

   {
      // consistencyStates
      if ( !Serialization::deserializeUInt8ListPreprocess(&buf[bufPos], bufLen - bufPos,
         &consistencyStatesElemNum, &consistencyStatesListStart, &consistencyStatesBufLen) )
         return false;

      bufPos += consistencyStatesBufLen;
   }

   return true;
}




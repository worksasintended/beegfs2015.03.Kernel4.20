#include "GetStatesAndBuddyGroupsRespMsg.h"

void GetStatesAndBuddyGroupsRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // buddyGroupIDs
   bufPos += Serialization::serializeUInt16List(&buf[bufPos], buddyGroupIDs);

   // primaryTargetIDs
   bufPos += Serialization::serializeUInt16List(&buf[bufPos], primaryTargetIDs);

   // secondaryTargetIDs
   bufPos += Serialization::serializeUInt16List(&buf[bufPos], secondaryTargetIDs);

   // targetIDs
   bufPos += Serialization::serializeUInt16List(&buf[bufPos], targetIDs);

   // targetReachabilityStates
   bufPos += Serialization::serializeUInt8List(&buf[bufPos], targetReachabilityStates);

   // targetConsistencyStates
   bufPos += Serialization::serializeUInt8List(&buf[bufPos], targetConsistencyStates);
}

bool GetStatesAndBuddyGroupsRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   {
      // buddyGroupIDs

      if ( !Serialization::deserializeUInt16ListPreprocess(&buf[bufPos], bufLen - bufPos,
         &buddyGroupIDsElemNum, &buddyGroupIDsListStart, &buddyGroupIDsBufLen) )
         return false;

      bufPos += buddyGroupIDsBufLen;
   }

   {
      // primaryTargetIDs

      if ( !Serialization::deserializeUInt16ListPreprocess(&buf[bufPos], bufLen - bufPos,
         &primaryTargetIDsElemNum, &primaryTargetIDsListStart, &primaryTargetIDsBufLen) )
         return false;

      bufPos += primaryTargetIDsBufLen;
   }

   {
      // secondaryTargetIDs

      if ( !Serialization::deserializeUInt16ListPreprocess(&buf[bufPos], bufLen - bufPos,
         &secondaryTargetIDsElemNum, &secondaryTargetIDsListStart, &secondaryTargetIDsBufLen) )
         return false;

      bufPos += secondaryTargetIDsBufLen;
   }

   {
      // targetIDs

      if ( !Serialization::deserializeUInt16ListPreprocess(&buf[bufPos], bufLen - bufPos,
         &targetIDsElemNum, &targetIDsListStart, &targetIDsBufLen) )
         return false;

      bufPos += targetIDsBufLen;
   }

   {
      // targetReachabilityStates

      if ( !Serialization::deserializeUInt8ListPreprocess(&buf[bufPos], bufLen- bufPos,
            &targetReachabilityStatesElemNum, &targetReachabilityStatesListStart,
            &targetReachabilityStatesBufLen) )
         return false;

      bufPos += targetReachabilityStatesBufLen;
   }

   {
      // targetConsistencyStates

      if ( !Serialization::deserializeUInt8ListPreprocess(&buf[bufPos], bufLen- bufPos,
            &targetConsistencyStatesElemNum, &targetConsistencyStatesListStart,
            &targetConsistencyStatesBufLen) )
         return false;

      bufPos += targetConsistencyStatesBufLen;
   }


   return true;
}

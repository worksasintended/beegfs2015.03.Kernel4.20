#include "GetMirrorBuddyGroupsRespMsg.h"

void GetMirrorBuddyGroupsRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // buddyGroupIDs
   bufPos += Serialization::serializeUInt16List(&buf[bufPos], buddyGroupIDs);

   // primaryTargetIDs
   bufPos += Serialization::serializeUInt16List(&buf[bufPos], primaryTargetIDs);

   // secondaryTargetIDs
   bufPos += Serialization::serializeUInt16List(&buf[bufPos], secondaryTargetIDs);
}

bool GetMirrorBuddyGroupsRespMsg::deserializePayload(const char* buf, size_t bufLen)
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


   return true;
}




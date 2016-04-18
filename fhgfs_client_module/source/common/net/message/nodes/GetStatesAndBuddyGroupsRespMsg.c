#include "GetStatesAndBuddyGroupsRespMsg.h"

fhgfs_bool GetStatesAndBuddyGroupsRespMsg_deserializePayload(NetMessage* this, const char* buf,
      size_t bufLen)
{
   GetStatesAndBuddyGroupsRespMsg* thisCast = (GetStatesAndBuddyGroupsRespMsg*)this;

   size_t bufPos = 0;

   // mirrorBuddyGroupIDs
   if(!Serialization_deserializeUInt16ListPreprocess(&buf[bufPos], bufLen-bufPos,
      &thisCast->buddyGroupIDsElemNum, &thisCast->buddyGroupIDsListStart,
      &thisCast->buddyGroupIDsBufLen) )
      return fhgfs_false;

   bufPos += thisCast->buddyGroupIDsBufLen;

   // primaryTargetIDs
   if(!Serialization_deserializeUInt16ListPreprocess(&buf[bufPos], bufLen-bufPos,
      &thisCast->primaryTargetIDsElemNum, &thisCast->primaryTargetIDsListStart,
      &thisCast->primaryTargetIDsBufLen) )
      return fhgfs_false;

   bufPos += thisCast->primaryTargetIDsBufLen;

   // secondaryTargetIDs
   if(!Serialization_deserializeUInt16ListPreprocess(&buf[bufPos], bufLen-bufPos,
      &thisCast->secondaryTargetIDsElemNum, &thisCast->secondaryTargetIDsListStart,
      &thisCast->secondaryTargetIDsBufLen) )
      return fhgfs_false;

   bufPos += thisCast->secondaryTargetIDsBufLen;

   // targetIDs
   if(!Serialization_deserializeUInt16ListPreprocess(&buf[bufPos], bufLen-bufPos,
      &thisCast->targetIDsElemNum, &thisCast->targetIDsListStart,
      &thisCast->targetIDsBufLen) )
      return fhgfs_false;

   bufPos += thisCast->targetIDsBufLen;

   // targetReachabilityStates
   if(!Serialization_deserializeUInt8ListPreprocess(&buf[bufPos], bufLen-bufPos,
      &thisCast->targetReachabilityStatesElemNum, &thisCast->targetReachabilityStatesListStart,
      &thisCast->targetReachabilityStatesBufLen) )
      return fhgfs_false;

   bufPos += thisCast->targetReachabilityStatesBufLen;

   // targetConsistencyStates
   if(!Serialization_deserializeUInt8ListPreprocess(&buf[bufPos], bufLen-bufPos,
      &thisCast->targetConsistencyStatesElemNum, &thisCast->targetConsistencyStatesListStart,
      &thisCast->targetConsistencyStatesBufLen) )
      return fhgfs_false;

   bufPos += thisCast->targetConsistencyStatesBufLen;

   return fhgfs_true;
}



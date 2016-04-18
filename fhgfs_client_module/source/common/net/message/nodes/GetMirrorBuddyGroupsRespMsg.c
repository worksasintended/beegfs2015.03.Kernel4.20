#include "GetMirrorBuddyGroupsRespMsg.h"

/**
 * Warning: not implementend
 */
void GetMirrorBuddyGroupsRespMsg_serializePayload(NetMessage* this, char* buf)
{
   // not implemented
}

fhgfs_bool GetMirrorBuddyGroupsRespMsg_deserializePayload(NetMessage* this, const char* buf,
   size_t bufLen)
{
   GetMirrorBuddyGroupsRespMsg* thisCast = (GetMirrorBuddyGroupsRespMsg*)this;

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

   return fhgfs_true;
}

unsigned GetMirrorBuddyGroupsRespMsg_calcMessageLength(NetMessage* this)
{
   GetMirrorBuddyGroupsRespMsg* thisCast = (GetMirrorBuddyGroupsRespMsg*)this;

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenUInt16List(thisCast->mirrorBuddyGroupIDs) +
      Serialization_serialLenUInt16List(thisCast->primaryTargetIDs)    +
      Serialization_serialLenUInt16List(thisCast->secondaryTargetIDs);
}


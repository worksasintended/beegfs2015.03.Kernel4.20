#include "GetTargetStatesRespMsg.h"

/**
 * Warning: not implementend
 */
void GetTargetStatesRespMsg_serializePayload(NetMessage* this, char* buf)
{
   // not implemented
}

fhgfs_bool GetTargetStatesRespMsg_deserializePayload(NetMessage* this, const char* buf,
   size_t bufLen)
{
   GetTargetStatesRespMsg* thisCast = (GetTargetStatesRespMsg*)this;

   size_t bufPos = 0;

   // targetIDs
   if(!Serialization_deserializeUInt16ListPreprocess(&buf[bufPos], bufLen-bufPos,
      &thisCast->targetIDsElemNum, &thisCast->targetIDsListStart,
      &thisCast->targetIDsBufLen) )
      return fhgfs_false;

   bufPos += thisCast->targetIDsBufLen;

   // reachabilityStates
   if(!Serialization_deserializeUInt8ListPreprocess(&buf[bufPos], bufLen-bufPos,
      &thisCast->reachabilityStatesElemNum, &thisCast->reachabilityStatesListStart,
      &thisCast->reachabilityStatesBufLen) )
      return fhgfs_false;

   bufPos += thisCast->reachabilityStatesBufLen;

   // consistencyStates
   if(!Serialization_deserializeUInt8ListPreprocess(&buf[bufPos], bufLen-bufPos,
      &thisCast->consistencyStatesElemNum, &thisCast->consistencyStatesListStart,
      &thisCast->consistencyStatesBufLen) )
      return fhgfs_false;

   bufPos += thisCast->consistencyStatesBufLen;

   return fhgfs_true;
}

unsigned GetTargetStatesRespMsg_calcMessageLength(NetMessage* this)
{
   GetTargetStatesRespMsg* thisCast = (GetTargetStatesRespMsg*)this;

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenUInt16List(thisCast->targetIDs)   +
      Serialization_serialLenUInt8List(thisCast->reachabilityStates) +
      Serialization_serialLenUInt8List(thisCast->consistencyStates);
}


#include "GetTargetMappingsRespMsg.h"


/**
 * Warning: not implementend
 */
void GetTargetMappingsRespMsg_serializePayload(NetMessage* this, char* buf)
{
   // not implemented
}

fhgfs_bool GetTargetMappingsRespMsg_deserializePayload(NetMessage* this, const char* buf,
   size_t bufLen)
{
   GetTargetMappingsRespMsg* thisCast = (GetTargetMappingsRespMsg*)this;

   size_t bufPos = 0;

   // targetIDs
   if(!Serialization_deserializeUInt16ListPreprocess(&buf[bufPos], bufLen-bufPos,
      &thisCast->targetIDsElemNum, &thisCast->targetIDsListStart, &thisCast->targetIDsBufLen) )
      return fhgfs_false;

   bufPos += thisCast->targetIDsBufLen;

   // nodeIDs
   if(!Serialization_deserializeUInt16ListPreprocess(&buf[bufPos], bufLen-bufPos,
      &thisCast->nodeIDsElemNum, &thisCast->nodeIDsListStart, &thisCast->nodeIDsBufLen) )
      return fhgfs_false;

   bufPos += thisCast->nodeIDsBufLen;

   return fhgfs_true;
}

unsigned GetTargetMappingsRespMsg_calcMessageLength(NetMessage* this)
{
   GetTargetMappingsRespMsg* thisCast = (GetTargetMappingsRespMsg*)this;

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenUInt16List(thisCast->targetIDs) +
      Serialization_serialLenUInt16List(thisCast->nodeIDs);
}


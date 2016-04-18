#include "StatStoragePathRespMsg.h"


/**
 * Serialzation not supported!
 */
void StatStoragePathRespMsg_serializePayload(NetMessage* this, char* buf)
{
   // not implemented
}

fhgfs_bool StatStoragePathRespMsg_deserializePayload(NetMessage* this, const char* buf,
   size_t bufLen)
{
   StatStoragePathRespMsg* thisCast = (StatStoragePathRespMsg*)this;

   size_t bufPos = 0;
   
   unsigned resultFieldLen;
   unsigned sizeTotalFieldLen;
   unsigned sizeFreeFieldLen;
   unsigned inodesTotalFieldLen;
   unsigned inodesFreeFieldLen;

   // result
   if(!Serialization_deserializeInt(&buf[bufPos], bufLen-bufPos, &thisCast->result,
      &resultFieldLen) )
      return fhgfs_false;

   bufPos += resultFieldLen;

   // sizeTotal
   if(!Serialization_deserializeInt64(&buf[bufPos], bufLen-bufPos, &thisCast->sizeTotal,
      &sizeTotalFieldLen) )
      return fhgfs_false;

   bufPos += sizeTotalFieldLen;
      
   // sizeFree
   if(!Serialization_deserializeInt64(&buf[bufPos], bufLen-bufPos, &thisCast->sizeFree,
      &sizeFreeFieldLen) )
      return fhgfs_false;

   bufPos += sizeFreeFieldLen;

   // inodesTotal
   if(!Serialization_deserializeInt64(&buf[bufPos], bufLen-bufPos, &thisCast->inodesTotal,
      &inodesTotalFieldLen) )
      return fhgfs_false;

   bufPos += inodesTotalFieldLen;

   // inodesFree
   if(!Serialization_deserializeInt64(&buf[bufPos], bufLen-bufPos, &thisCast->inodesFree,
      &inodesFreeFieldLen) )
      return fhgfs_false;

   bufPos += inodesFreeFieldLen;

   return fhgfs_true;
}

unsigned StatStoragePathRespMsg_calcMessageLength(NetMessage* this)
{
   //StatRespMsg* thisCast = (StatRespMsg*)this;

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenInt() + // result
      Serialization_serialLenInt64() + // sizeTotal
      Serialization_serialLenInt64() + // sizeFree
      Serialization_serialLenInt64() + // inodesTotal
      Serialization_serialLenInt64(); // inodesFree
}



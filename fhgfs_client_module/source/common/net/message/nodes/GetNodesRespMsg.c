#include "GetNodesRespMsg.h"

void GetNodesRespMsg_serializePayload(NetMessage* this, char* buf)
{
   GetNodesRespMsg* thisCast = (GetNodesRespMsg*)this;

   size_t bufPos = 0;

   // nodeList (8b aligned)
   bufPos += Serialization_serializeNodeList(&buf[bufPos], thisCast->nodeList);

   // rootNumID
   bufPos += Serialization_serializeUShort(&buf[bufPos], thisCast->rootNumID);
}

fhgfs_bool GetNodesRespMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   GetNodesRespMsg* thisCast = (GetNodesRespMsg*)this;

   size_t bufPos = 0;

   { // nodeList
      unsigned nodeListBufLen;

      if(!Serialization_deserializeNodeListPreprocess(&buf[bufPos], bufLen-bufPos,
         &thisCast->nodeListElemNum, &thisCast->nodeListStart, &nodeListBufLen) )
         return fhgfs_false;

      bufPos += nodeListBufLen;
   }

   { // rootNumID
      unsigned rootNumIDBufLen;

      if(!Serialization_deserializeUShort(&buf[bufPos], bufLen-bufPos,
         &thisCast->rootNumID, &rootNumIDBufLen) )
         return fhgfs_false;

      bufPos += rootNumIDBufLen;
   }

   return fhgfs_true;
}

unsigned GetNodesRespMsg_calcMessageLength(NetMessage* this)
{
   GetNodesRespMsg* thisCast = (GetNodesRespMsg*)this;

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenNodeList(thisCast->nodeList) + // (8b aligned)
      Serialization_serialLenUShort(); // rootNumID
}



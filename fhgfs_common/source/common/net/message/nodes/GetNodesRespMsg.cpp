#include "GetNodesRespMsg.h"

bool GetNodesRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   // nodeList

   unsigned nodeListBufLen;

   if(!Serialization::deserializeNodeListPreprocess(&buf[bufPos], bufLen-bufPos,
      &nodeListElemNum, &nodeListStart, &nodeListBufLen) )
      return false;

   bufPos += nodeListBufLen;

   // rootNumID
   
   unsigned rootNumIDBufLen;
   
   if(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos,
      &rootNumID, &rootNumIDBufLen) )
      return false;
   
   bufPos += rootNumIDBufLen;

   return true;   
}

void GetNodesRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // nodeList (8b aligned)
   bufPos += Serialization::serializeNodeList(&buf[bufPos], nodeList);

   // rootNumID
   bufPos += Serialization::serializeUShort(&buf[bufPos], rootNumID);
}




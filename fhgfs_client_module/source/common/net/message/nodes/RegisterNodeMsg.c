#include <app/App.h>
#include <common/nodes/Node.h>
#include <common/toolkit/SocketTk.h>
#include <common/net/msghelpers/MsgHelperAck.h>
#include <common/toolkit/ListTk.h>
#include <nodes/NodeStoreEx.h>
#include <app/config/Config.h>
#include "RegisterNodeMsg.h"

void RegisterNodeMsg_serializePayload(NetMessage* this, char* buf)
{
   RegisterNodeMsg* thisCast = (RegisterNodeMsg*)this;

   size_t bufPos = 0;

   // nodeFeatureFlags (8b aligned)
   bufPos += BitStore_serialize(thisCast->nodeFeatureFlags, &buf[bufPos] );

   // instanceVersion
   bufPos += Serialization_serializeUInt64(&buf[bufPos], thisCast->instanceVersion);

   // nicListVersion
   bufPos += Serialization_serializeUInt64(&buf[bufPos], thisCast->nicListVersion);

   // nodeID
   bufPos += Serialization_serializeStrAlign4(&buf[bufPos], thisCast->nodeIDLen, thisCast->nodeID);

   // nicList (4b aligned)
   bufPos += Serialization_serializeNicList(&buf[bufPos], thisCast->nicList);

   // nodeType
   bufPos += Serialization_serializeInt(&buf[bufPos], thisCast->nodeType);

   // nodeNumID
   bufPos += Serialization_serializeUShort(&buf[bufPos], thisCast->nodeNumID);

   // rootNumID
   bufPos += Serialization_serializeUShort(&buf[bufPos], thisCast->rootNumID);

   // portUDP
   bufPos += Serialization_serializeUShort(&buf[bufPos], thisCast->portUDP);

   // portTCP
   bufPos += Serialization_serializeUShort(&buf[bufPos], thisCast->portTCP);
}

/**
 * Not implemented!
 */
fhgfs_bool RegisterNodeMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   return fhgfs_false;
}

unsigned RegisterNodeMsg_calcMessageLength(NetMessage* this)
{
   RegisterNodeMsg* thisCast = (RegisterNodeMsg*)this;

   return NETMSG_HEADER_LENGTH +
      BitStore_serialLen(thisCast->nodeFeatureFlags) + // (8byte aligned)
      Serialization_serialLenUInt64() + // instanceVersion
      Serialization_serialLenUInt64() + // nicListVersion
      Serialization_serialLenStrAlign4(thisCast->nodeIDLen) +
      Serialization_serialLenNicList(thisCast->nicList) +
      Serialization_serialLenInt() + // nodeType
      Serialization_serialLenUShort() + // nodeNumID
      Serialization_serialLenUShort() + // rootNumID
      Serialization_serialLenUShort() + // portUDP
      Serialization_serialLenUShort(); // portTCP
}


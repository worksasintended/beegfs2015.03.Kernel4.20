#include <app/App.h>
#include <common/nodes/Node.h>
#include <common/toolkit/SocketTk.h>
#include <common/net/msghelpers/MsgHelperAck.h>
#include <common/toolkit/ListTk.h>
#include <nodes/NodeStoreEx.h>
#include <app/config/Config.h>
#include "HeartbeatMsgEx.h"

void HeartbeatMsgEx_serializePayload(NetMessage* this, char* buf)
{
   HeartbeatMsgEx* thisCast = (HeartbeatMsgEx*)this;

   size_t bufPos = 0;

   // nodeFeatureFlags
   bufPos += BitStore_serialize(thisCast->nodeFeatureFlags, &buf[bufPos] );

   // instanceVersion
   bufPos += Serialization_serializeUInt64(&buf[bufPos], thisCast->instanceVersion);

   // nicListVersion
   bufPos += Serialization_serializeUInt64(&buf[bufPos], thisCast->nicListVersion);

   // nodeType
   bufPos += Serialization_serializeInt(&buf[bufPos], thisCast->nodeType);

   // fhgfsVersion
   bufPos += Serialization_serializeUInt(&buf[bufPos], thisCast->fhgfsVersion);

   // nodeID
   bufPos += Serialization_serializeStrAlign4(&buf[bufPos], thisCast->nodeIDLen, thisCast->nodeID);

   // ackID
   bufPos += Serialization_serializeStrAlign4(&buf[bufPos], thisCast->ackIDLen, thisCast->ackID);

   // nodeNumID
   bufPos += Serialization_serializeUShort(&buf[bufPos], thisCast->nodeNumID);

   // rootNumID
   bufPos += Serialization_serializeUShort(&buf[bufPos], thisCast->rootNumID);

   // portUDP
   bufPos += Serialization_serializeUShort(&buf[bufPos], thisCast->portUDP);

   // portTCP
   bufPos += Serialization_serializeUShort(&buf[bufPos], thisCast->portTCP);

   // nicList
   bufPos += Serialization_serializeNicList(&buf[bufPos], thisCast->nicList);
}

fhgfs_bool HeartbeatMsgEx_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   HeartbeatMsgEx* thisCast = (HeartbeatMsgEx*)this;

   size_t bufPos = 0;


   {// nodeFeatureFlags
      unsigned featuresBufLen;

      if(!BitStore_deserializePreprocess(&buf[bufPos], bufLen-bufPos,
         &thisCast->nodeFeatureFlagsStart, &featuresBufLen) )
         return fhgfs_false;

      bufPos += featuresBufLen;
   }

   {// instanceVersion
      unsigned instanceBufLen;

      if(!Serialization_deserializeUInt64(&buf[bufPos], bufLen-bufPos,
         &thisCast->instanceVersion, &instanceBufLen) )
         return fhgfs_false;

      bufPos += instanceBufLen;
   }

   {// nicListVersion
      unsigned nicVersionBufLen;

      if(!Serialization_deserializeUInt64(&buf[bufPos], bufLen-bufPos,
         &thisCast->nicListVersion, &nicVersionBufLen) )
         return fhgfs_false;

      bufPos += nicVersionBufLen;
   }

   {// nodeType
      unsigned nodeTypeBufLen;

      if(!Serialization_deserializeInt(&buf[bufPos], bufLen-bufPos,
         &thisCast->nodeType, &nodeTypeBufLen) )
         return fhgfs_false;

      bufPos += nodeTypeBufLen;
   }

   {// fhgfsVersion
      unsigned versionBufLen;

      if(!Serialization_deserializeInt(&buf[bufPos], bufLen-bufPos,
         &thisCast->fhgfsVersion, &versionBufLen) )
         return fhgfs_false;

      bufPos += versionBufLen;
   }

   {// nodeID
      unsigned nodeBufLen;

      if(!Serialization_deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &thisCast->nodeIDLen, &thisCast->nodeID, &nodeBufLen) )
         return fhgfs_false;

      bufPos += nodeBufLen;
   }

   {// ackID
      unsigned ackBufLen;

      if(!Serialization_deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &thisCast->ackIDLen, &thisCast->ackID, &ackBufLen) )
         return fhgfs_false;

      bufPos += ackBufLen;
   }

   {// nodeNumID
      unsigned nodeNumIDBufLen;

      if(!Serialization_deserializeUShort(&buf[bufPos], bufLen-bufPos,
         &thisCast->nodeNumID, &nodeNumIDBufLen) )
         return fhgfs_false;

      bufPos += nodeNumIDBufLen;
   }

   {// rootNumID
      unsigned rootNumIDBufLen;

      if(!Serialization_deserializeUShort(&buf[bufPos], bufLen-bufPos,
         &thisCast->rootNumID, &rootNumIDBufLen) )
         return fhgfs_false;

      bufPos += rootNumIDBufLen;
   }

   {// portUDP
      unsigned portUDPBufLen;

      if(!Serialization_deserializeUShort(&buf[bufPos], bufLen-bufPos,
         &thisCast->portUDP, &portUDPBufLen) )
         return fhgfs_false;

      bufPos += portUDPBufLen;
   }

   {// portTCP
      unsigned portTCPBufLen;

      if(!Serialization_deserializeUShort(&buf[bufPos], bufLen-bufPos,
         &thisCast->portTCP, &portTCPBufLen) )
         return fhgfs_false;

      bufPos += portTCPBufLen;
   }

   {// nicList
      unsigned nicListBufLen;

      if(!Serialization_deserializeNicListPreprocess(&buf[bufPos], bufLen-bufPos,
         &thisCast->nicListElemNum, &thisCast->nicListStart, &nicListBufLen) )
         return fhgfs_false;

      bufPos += nicListBufLen;
   }
   
   return fhgfs_true;
}

unsigned HeartbeatMsgEx_calcMessageLength(NetMessage* this)
{
   HeartbeatMsgEx* thisCast = (HeartbeatMsgEx*)this;

   return NETMSG_HEADER_LENGTH +
      BitStore_serialLen(thisCast->nodeFeatureFlags) + // (8byte aligned)
      Serialization_serialLenUInt64() + // instanceVersion
      Serialization_serialLenUInt64() + // nicListVersion
      Serialization_serialLenInt() + // nodeType
      Serialization_serialLenUInt() + // fhgfsVersion
      Serialization_serialLenStrAlign4(thisCast->nodeIDLen) +
      Serialization_serialLenStrAlign4(thisCast->ackIDLen) +
      Serialization_serialLenUShort() + // nodeNumID
      Serialization_serialLenUShort() + // rootNumID
      Serialization_serialLenUShort() + // portUDP
      Serialization_serialLenUShort() + // portTCP
      Serialization_serialLenNicList(thisCast->nicList); // (4byte aligned)
}

fhgfs_bool __HeartbeatMsgEx_processIncoming(NetMessage* this, struct App* app,
   fhgfs_sockaddr_in* fromAddr, struct Socket* sock, char* respBuf, size_t bufLen)
{
   Logger* log = App_getLogger(app);
   const char* logContext = "Heartbeat incoming";

   HeartbeatMsgEx* thisCast = (HeartbeatMsgEx*)this;

   NicAddressList nicList;
   Node* node;
   NodeConnPool* connPool;
   Node* localNode;
   NicAddressList* localNicList;
   NicListCapabilities localNicCaps;
   NodeStoreEx* nodes = NULL;
   fhgfs_bool isNodeNew;


   // check if nodeNumID is set

   if(!HeartbeatMsgEx_getNodeNumID(thisCast) )
   { // shouldn't happen: this node would need to register first to get a nodeNumID assigned
      Logger_logFormatted(log, Log_WARNING, logContext,
         "Rejecting heartbeat of node without numeric ID: %s (Type: %s)",
         HeartbeatMsgEx_getNodeID(thisCast),
         Node_nodeTypeToStr(HeartbeatMsgEx_getNodeType(thisCast) ) );

      goto ack_resp;
   }

   // find the corresponding node store for this node type

   switch(HeartbeatMsgEx_getNodeType(thisCast) )
   {
      case NODETYPE_Meta:
         nodes = App_getMetaNodes(app); break;

      case NODETYPE_Storage:
         nodes = App_getStorageNodes(app); break;

      case NODETYPE_Mgmt:
         nodes = App_getMgmtNodes(app); break;

      default:
      {
         int nodeType = HeartbeatMsgEx_getNodeType(thisCast);
         const char* nodeID = HeartbeatMsgEx_getNodeID(thisCast);

         Logger_logErrFormatted(log, logContext, "Invalid node type: %d (%s); ID: %s",
            nodeType, Node_nodeTypeToStr(nodeType), nodeID);
         
         goto ack_resp;
      } break;
   }

   // construct node

   NicAddressList_init(&nicList);

   HeartbeatMsgEx_parseNicList(thisCast, &nicList);

   node = Node_construct(app,
      HeartbeatMsgEx_getNodeID(thisCast), HeartbeatMsgEx_getNodeNumID(thisCast),
      HeartbeatMsgEx_getPortUDP(thisCast), HeartbeatMsgEx_getPortTCP(thisCast), &nicList);
      // (will belong to the NodeStore => no destruct() required)

   Node_setNodeType(node, HeartbeatMsgEx_getNodeType(thisCast) );
   Node_setFhgfsVersion(node, HeartbeatMsgEx_getFhgfsVersion(thisCast) );

   {// set nodeFeatureFlags
      BitStore nodeFeatureFlags;

      BitStore_init(&nodeFeatureFlags, fhgfs_false);

      HeartbeatMsgEx_parseNodeFeatureFlags(thisCast, &nodeFeatureFlags);
      Node_setFeatureFlags(node, &nodeFeatureFlags);

      BitStore_uninit(&nodeFeatureFlags);
   }

   // set local nic capabilities

   localNode = App_getLocalNode(app);
   localNicList = Node_getNicList(localNode);
   connPool = Node_getConnPool(node);

   NIC_supportedCapabilities(localNicList, &localNicCaps);
   NodeConnPool_setLocalNicCaps(connPool, &localNicCaps);

   // add node to store (or update it)

   isNodeNew = NodeStoreEx_addOrUpdateNode(nodes, &node);
   if(isNodeNew)
   {
      fhgfs_bool supportsSDP = NIC_supportsSDP(&nicList);
      fhgfs_bool supportsRDMA = NIC_supportsRDMA(&nicList);

      Logger_logFormatted(log, Log_WARNING, logContext,
         "New node: %s %s [ID: %hu]; %s%s",
         Node_nodeTypeToStr(HeartbeatMsgEx_getNodeType(thisCast) ),
         HeartbeatMsgEx_getNodeID(thisCast),
         HeartbeatMsgEx_getNodeNumID(thisCast),
         (supportsSDP ? "SDP; " : ""),
         (supportsRDMA ? "RDMA; " : "") );
   }

   __HeartbeatMsgEx_processIncomingRoot(thisCast, app);

ack_resp:   
   // send ack
   MsgHelperAck_respondToAckRequest(app, HeartbeatMsgEx_getAckID(thisCast), fromAddr, sock,
      respBuf, bufLen);

   // clean-up
   ListTk_kfreeNicAddressListElems(&nicList);
   NicAddressList_uninit(&nicList);

   return fhgfs_true;
}

/**
 * Handles the contained root information.
 */
void __HeartbeatMsgEx_processIncomingRoot(HeartbeatMsgEx* this, App* app)
{
   Logger* log = App_getLogger(app);
   const char* logContext = "Heartbeat incoming (root)";

   NodeStoreEx* metaNodes;
   fhgfs_bool setRootRes;

   // check whether root info is defined
   if( (HeartbeatMsgEx_getNodeType(this) != NODETYPE_Meta) ||
       !HeartbeatMsgEx_getRootNumID(this) )
      return;

   // try to apply the contained root info

   metaNodes = App_getMetaNodes(app);

   setRootRes = NodeStoreEx_setRootNodeNumID(
      metaNodes, HeartbeatMsgEx_getRootNumID(this), fhgfs_false);

   if(setRootRes)
   { // found the very first root
      Logger_logFormatted(log, Log_CRITICAL, logContext, "Root (by Heartbeat): %hu",
         HeartbeatMsgEx_getRootNumID(this) );
   }

}


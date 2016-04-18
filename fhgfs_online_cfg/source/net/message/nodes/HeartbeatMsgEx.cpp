#include <common/net/msghelpers/MsgHelperAck.h>
#include <common/net/sock/NetworkInterfaceCard.h>
#include <program/Program.h>
#include "HeartbeatMsgEx.h"

bool HeartbeatMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("Heartbeat incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   //LOG_DEBUG_CONTEXT(log, 4, std::string("Received a HeartbeatMsg from: ") + peer);

   App* app = Program::getApp();
   
   bool isNodeNew;

   // construct node

   NicAddressList nicList;
   parseNicList(&nicList);

   Node* node = new Node(getNodeID(), getNodeNumID(), getPortUDP(), getPortTCP(),
      nicList); // (will belong to the NodeStore => no delete() required)

   node->setNodeType(getNodeType() );
   node->setFhgfsVersion(getFhgfsVersion() );

   BitStore nodeFeatureFlags;
   parseNodeFeatureFlags(&nodeFeatureFlags);

   node->setFeatureFlags(&nodeFeatureFlags);

   // set local nic capabilities

   Node* localNode = app->getLocalNode();
   NicAddressList localNicList(localNode->getNicList() );
   NicListCapabilities localNicCaps;

   NetworkInterfaceCard::supportedCapabilities(&localNicList, &localNicCaps);
   node->getConnPool()->setLocalNicCaps(&localNicCaps);

   std::string nodeIDWithTypeStr = node->getNodeIDWithTypeStr();

   // get corresponding node store
   
   AbstractNodeStore* nodes = app->getAbstractNodeStoreFromType(getNodeType() );
   if(unlikely(!nodes) )
   {
      log.logErr(std::string("Invalid node type: ") + StringTk::intToStr(getNodeType() ) );
      
      goto ack_resp;
   }

   // add/update node in store

   isNodeNew = nodes->addOrUpdateNode(&node);
   if(isNodeNew)
   {
      NicAddressList nicList;
      parseNicList(&nicList);

      processNewNode(nodeIDWithTypeStr, getNodeType(), &nicList, peer);
   }


   processIncomingRoot();

ack_resp:
   MsgHelperAck::respondToAckRequest(this, fromAddr, sock,
      respBuf, bufLen, app->getDatagramListener() );

   return true;
}

/**
 * Handles the contained root information.
 */
void HeartbeatMsgEx::processIncomingRoot()
{
   // check whether root info is defined
   if( (getNodeType() != NODETYPE_Meta) || !getRootNumID() )
      return;

   // try to apply the contained root info
   Program::getApp()->getMetaNodes()->setRootNodeNumID(getRootNumID(), false);
}

/**
 * @param nodeIDWithTypeStr for log msg (the string that is returned by
 * node->getNodeIDWithTypeStr() ).
 */
void HeartbeatMsgEx::processNewNode(std::string nodeIDWithTypeStr, NodeType nodeType,
   NicAddressList* nicList, std::string sourcePeer)
{
   LogContext log("Node registration");

   App* app = Program::getApp();
   AbstractNodeStore* nodes = app->getAbstractNodeStoreFromType(getNodeType() );

   bool supportsSDP = NetworkInterfaceCard::supportsSDP(nicList);
   bool supportsRDMA = NetworkInterfaceCard::supportsRDMA(nicList);
   std::string nodeTypeStr = Node::nodeTypeToStr(nodeType);

   log.log(Log_WARNING, std::string("New node: ") +
      nodeIDWithTypeStr + "; " +
      std::string(supportsSDP ? "SDP; " : "") +
      std::string(supportsRDMA ? "RDMA; " : "") +
      std::string("Source: ") + sourcePeer);

   log.log(Log_DEBUG, std::string("Number of nodes in the system: ") +
      StringTk::uintToStr(nodes->getSize() ) +
      std::string(" (Type: ") + nodeTypeStr + ")" );
}

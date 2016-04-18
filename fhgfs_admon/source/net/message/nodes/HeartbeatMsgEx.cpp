#include <common/net/msghelpers/MsgHelperAck.h>
#include <common/net/sock/NetworkInterfaceCard.h>
#include <program/Program.h>
#include "HeartbeatMsgEx.h"

bool HeartbeatMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("Heartbeat incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   //LOG_DEBUG_CONTEXT(log, Log_DEBUG, std::string("Received a HeartbeatMsg from: ") + peer);

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


   // add/update node in store

   NodeStoreServers* nodes;

   switch(getNodeType() )
   {
      case NODETYPE_Meta:
         nodes = app->getMetaNodes(); break;

      case NODETYPE_Storage:
         nodes = app->getStorageNodes(); break;

      case NODETYPE_Mgmt:
         nodes = app->getMgmtNodes(); break;

      default:
      {
         log.logErr(std::string("Invalid node type: ") + StringTk::intToStr(getNodeType() ) );

         goto ack_resp;
      } break;
   }

   isNodeNew = nodes->addOrUpdateNodeEx(&node, NULL);

   if(isNodeNew)
   { // log info about new server
      bool supportsSDP = NetworkInterfaceCard::supportsSDP(&nicList);
      bool supportsRDMA = NetworkInterfaceCard::supportsRDMA(&nicList);

      log.log(Log_WARNING, std::string("New node: ") +
         nodeIDWithTypeStr + "; " +
         std::string(supportsSDP ? "SDP; " : "") +
         std::string(supportsRDMA ? "RDMA; " : "") +
         std::string("Source: ") + peer);

      log.log(Log_WARNING,
         "Number of nodes: " + StringTk::intToStr(nodes->getSize() ) + " "
         "(Type: " + Node::nodeTypeToStr((NodeType)getNodeType() ) + ")");
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


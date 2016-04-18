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

   NicAddressList localNicList(app->getLocalNicList() );
   NicListCapabilities localNicCaps;

   NetworkInterfaceCard::supportedCapabilities(&localNicList, &localNicCaps);
   node->getConnPool()->setLocalNicCaps(&localNicCaps);
   
   std::string nodeIDWithTypeStr = node->getNodeIDWithTypeStr();


   // add/update node in store

   AbstractNodeStore* nodes;
   
   switch(getNodeType() )
   {
      case NODETYPE_Meta:
         nodes = app->getMetaNodes(); break;
      
      case NODETYPE_Mgmt:
         nodes = app->getMgmtNodes(); break;

      case NODETYPE_Storage:
         nodes = app->getStorageNodes(); break;

      case NODETYPE_Hsm:
         nodes = app->getHsmNodes(); break;

      default:
      {
         log.logErr("Invalid/unexpected node type: " + Node::nodeTypeToStr(getNodeType() ) );
         
         goto ack_resp;
      } break;
   }

   isNodeNew = nodes->addOrUpdateNode(&node);
   if(isNodeNew)
   { // log info about new server
      bool supportsSDP = NetworkInterfaceCard::supportsSDP(&nicList);
      bool supportsRDMA = NetworkInterfaceCard::supportsRDMA(&nicList);

      log.log(Log_WARNING, std::string("New node: ") +
         nodeIDWithTypeStr + "; " +
         std::string(supportsSDP ? "SDP; " : "") +
         std::string(supportsRDMA ? "RDMA; " : "") );
   }


ack_resp:
   MsgHelperAck::respondToAckRequest(this, fromAddr, sock,
      respBuf, bufLen, app->getDatagramListener() );

   app->getNodeOpStats()->updateNodeOp(
      sock->getPeerIP(), StorageOpCounter_HEARTBEAT, getMsgHeaderUserID() );

   return true;
}


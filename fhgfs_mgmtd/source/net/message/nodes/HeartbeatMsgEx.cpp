#include <common/net/msghelpers/MsgHelperAck.h>
#include <common/net/sock/NetworkInterfaceCard.h>
#include <common/nodes/NodeStoreClients.h>
#include <common/nodes/NodeStoreServers.h>
#include <net/message/nodes/RegisterNodeMsgEx.h>
#include <program/Program.h>
#include "HeartbeatMsgEx.h"


bool HeartbeatMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("Heartbeat incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   //LOG_DEBUG_CONTEXT(log, Log_DEBUG, std::string("Received a HeartbeatMsg from: ") + peer);

   App* app = Program::getApp();
   NodeCapacityPools* metaCapacityPools = app->getMetaCapacityPools();
   HeartbeatManager* heartbeatMgr = app->getHeartbeatMgr();

   bool isNodeNew;

   NodeType nodeType = getNodeType();
   std::string nodeID(getNodeID() );

   NicAddressList nicList;
   parseNicList(&nicList);

   BitStore nodeFeatureFlags;
   parseNodeFeatureFlags(&nodeFeatureFlags);


   // check for empty nodeID; (sanity check, should never fail)

   if(unlikely(nodeID.empty() ) )
   {
      log.log(Log_WARNING, "Rejecting heartbeat of node with empty long ID "
         "from: " + peer + "; "
         "type: " + Node::nodeTypeToStr(nodeType) );

      return false;
   }


   if(nodeType == NODETYPE_Client)
   { // this is a client heartbeat
      NodeStoreClients* clients = app->getClientNodes();

      // construct node

      Node* node = RegisterNodeMsgEx::constructNode(
         nodeID, getNodeNumID(), getPortUDP(), getPortTCP(), nicList);

      node->setNodeType(getNodeType() );
      node->setFhgfsVersion(getFhgfsVersion() );
      node->setFeatureFlags(&nodeFeatureFlags);

      // add node to store (or update it)

      isNodeNew = clients->addOrUpdateNode(&node);
   }
   else
   { // this is a server heartbeat

      /* only accept new servers if nodeNumID is set
         (otherwise RegisterNodeMsg would need to be called first) */

      if(!getNodeNumID() )
      { /* shouldn't happen: this server would need to register first to get a nodeNumID assigned */

         log.log(Log_WARNING,
            "Rejecting heartbeat of node without numeric ID: " + nodeID + "; "
            "type: " + Node::nodeTypeToStr(nodeType) );

         return false;
      }

      // get the corresponding node store for this node type

      NodeStoreServers* servers = app->getServerStoreFromType(nodeType);
      if(unlikely(!servers) )
      {
         log.logErr(std::string("Invalid node type: ") + StringTk::intToStr(nodeType) );

         return false;
      }

      // check if adding a new server is allowed (in case this is a server)

      if(!RegisterNodeMsgEx::checkNewServerAllowed(servers, getNodeNumID(), nodeType) )
      { // this is a new server and adding was disabled
         log.log(Log_WARNING, std::string("Registration of new servers disabled. Rejecting: ") +
            nodeID + " (Type: " + Node::nodeTypeToStr(nodeType) + ")");

         return true;
      }

      // construct node

      Node* node = RegisterNodeMsgEx::constructNode(
         nodeID, getNodeNumID(), getPortUDP(), getPortTCP(), nicList);

      node->setNodeType(nodeType);
      node->setFhgfsVersion(getFhgfsVersion() );
      node->setFeatureFlags(&nodeFeatureFlags);

      std::string typedNodeID = node->getTypedNodeID();

      // add node to store (or update it)

      uint16_t confirmationNodeNumID;

      isNodeNew = servers->addOrUpdateNodeEx(&node, &confirmationNodeNumID);

      if(confirmationNodeNumID != getNodeNumID() )
      { // unable to add node to store
         log.log(Log_WARNING, "Node rejected because of ID conflict. "
            "Given numeric ID: " + StringTk::uintToStr(getNodeNumID() ) + "; "
            "string ID: " + getNodeID() + "; "
            "type: " + Node::nodeTypeToStr(nodeType) );

         return true;
      }

      // add to capacity pools

      if(nodeType == NODETYPE_Meta)
      {
         app->getMetaStateStore()->addIfNotExists(getNodeNumID(), CombinedTargetState(
            TargetReachabilityState_POFFLINE, TargetConsistencyState_GOOD) );

         bool isNewMetaTarget = metaCapacityPools->addIfNotExists(
            confirmationNodeNumID, CapacityPool_LOW);

         if(isNewMetaTarget)
            heartbeatMgr->notifyAsyncAddedNode(nodeID, getNodeNumID(), nodeType);

         // (note: storage targets get published through MapTargetMsg)
      }

      // handle root node information (if any is given)

      RegisterNodeMsgEx::processIncomingRoot(getRootNumID(), nodeType);

   } // end of server heartbeat specific handling

   
   if(isNodeNew)
   { // this node is new
      RegisterNodeMsgEx::processNewNode(nodeID, getNodeNumID(), nodeType, getFhgfsVersion(),
         &nicList, peer);
   }

   // send response

   MsgHelperAck::respondToAckRequest(this, fromAddr, sock,
      respBuf, bufLen, app->getDatagramListener() );

   if (nodeType == NODETYPE_Storage)
      app->getTargetStateStore()->saveStates();

   return true;
}



#include <common/net/message/nodes/HeartbeatMsg.h>
#include <common/net/message/nodes/HeartbeatRequestMsg.h>
#include <common/net/message/nodes/MapTargetsMsg.h>
#include <common/net/message/nodes/MapTargetsRespMsg.h>
#include <common/net/message/nodes/RefreshCapacityPoolsMsg.h>
#include <common/net/message/nodes/RefreshTargetStatesMsg.h>
#include <common/net/message/nodes/RemoveNodeMsg.h>
#include <common/net/message/nodes/RemoveNodeRespMsg.h>
#include <common/net/message/nodes/SetMirrorBuddyGroupMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/SocketTk.h>
#include <program/Program.h>
#include "HbMgrNotification.h"


void HbMgrNotificationNodeAdded::processNotification()
{
   App* app = Program::getApp();

   Node* node;

   if(nodeType == NODETYPE_Client)
      node = app->getClientNodes()->referenceNode(nodeID);
   else
      node = app->getServerStoreFromType(nodeType)->referenceNode(nodeNumID);

   if(!node)
      return;

   LOG_DEBUG(__func__, Log_SPAM, std::string("Propagating new node information: ") +
      node->getNodeIDWithTypeStr() );


   propagateAddedNode(node);


   if(nodeType == NODETYPE_Client)
      app->getClientNodes()->releaseNode(&node);
   else
      app->getServerStoreFromType(nodeType)->releaseNode(&node);
}

void HbMgrNotificationNodeAdded::propagateAddedNode(Node* node)
{
   App* app = Program::getApp();
   DatagramListener* dgramLis = app->getDatagramListener();
   NodeStoreClients* clients = app->getClientNodes();
   NodeStoreServers* metaNodes = app->getMetaNodes();
   NodeStoreServers* storageNodes = app->getStorageNodes();

   std::string nodeID = node->getID();
   uint16_t nodeNumID = node->getNumID();
   NicAddressList nicList(node->getNicList() );
   const BitStore* nodeFeatureFlags = node->getNodeFeatures();


   if(nodeType == NODETYPE_Meta)
   {
      uint16_t rootNodeID = app->getMetaNodes()->getRootNodeNumID();

      HeartbeatMsg hbMsg(nodeID.c_str(), nodeNumID, nodeType, &nicList, nodeFeatureFlags);
      hbMsg.setRootNumID(rootNodeID);
      hbMsg.setPorts(node->getPortUDP(), node->getPortTCP() );
      hbMsg.setFhgfsVersion(node->getFhgfsVersion() );

      dgramLis->sendToNodesUDPwithAck(clients, &hbMsg);
      dgramLis->sendToNodesUDPwithAck(metaNodes, &hbMsg);
   }
   else
   if(nodeType == NODETYPE_Storage)
   {
      HeartbeatMsg hbMsg(nodeID.c_str(), nodeNumID, nodeType, &nicList, nodeFeatureFlags);
      hbMsg.setPorts(node->getPortUDP(), node->getPortTCP() );
      hbMsg.setFhgfsVersion(node->getFhgfsVersion() );

      dgramLis->sendToNodesUDPwithAck(storageNodes, &hbMsg);
      dgramLis->sendToNodesUDPwithAck(clients, &hbMsg);
      dgramLis->sendToNodesUDPwithAck(metaNodes, &hbMsg);
   }
   else
   if(nodeType == NODETYPE_Client)
   {
      HeartbeatMsg hbMsg(nodeID.c_str(), nodeNumID, nodeType, &nicList, nodeFeatureFlags);
      hbMsg.setPorts(node->getPortUDP(), node->getPortTCP() );
      hbMsg.setFhgfsVersion(node->getFhgfsVersion() );

      dgramLis->sendToNodesUDPwithAck(metaNodes, &hbMsg);
   }
   else
   if(nodeType == NODETYPE_Hsm)
   {
      HeartbeatMsg hbMsg(nodeID.c_str(), nodeNumID, nodeType, &nicList, nodeFeatureFlags);
      hbMsg.setPorts(node->getPortUDP(), node->getPortTCP() );
      hbMsg.setFhgfsVersion(node->getFhgfsVersion() );

      dgramLis->sendToNodesUDPwithAck(storageNodes, &hbMsg);
   }
}

void HbMgrNotificationNodeRemoved::processNotification()
{
   LOG_DEBUG(__func__, Log_SPAM,
      std::string("Propagating removed node information: ") + nodeID +
      " (Type: " + Node::nodeTypeToStr(nodeType) + ")");

   propagateRemovedNode();
}

void HbMgrNotificationNodeRemoved::propagateRemovedNode()
{
   App* app = Program::getApp();
   DatagramListener* dgramLis = app->getDatagramListener();
   NodeStoreClients* clients = app->getClientNodes();
   NodeStoreServers* metaNodes = app->getMetaNodes();
   NodeStoreServers* storageNodes = app->getStorageNodes();

   RemoveNodeMsg removeMsg(nodeID.c_str(), nodeNumID, nodeType);

   if(nodeType == NODETYPE_Meta)
   {
      dgramLis->sendToNodesUDPwithAck(metaNodes, &removeMsg);
      dgramLis->sendToNodesUDPwithAck(clients, &removeMsg);
   }
   else
   if(nodeType == NODETYPE_Storage)
   {
      dgramLis->sendToNodesUDPwithAck(metaNodes, &removeMsg);
      dgramLis->sendToNodesUDPwithAck(clients, &removeMsg);
      dgramLis->sendToNodesUDPwithAck(storageNodes, &removeMsg);
   }
   else
   if(nodeType == NODETYPE_Client)
   {
      // Note: Meta and storage servers need to be informed about removed clients via their
      //    own downloadAndSyncNodes() method to handle the information in an appropriate thread
   }
}

void HbMgrNotificationTargetAdded::processNotification()
{
   LOG_DEBUG(__func__, Log_SPAM,
      "Propagating added target information: " + StringTk::uintToStr(targetID) + "; "
      "node: " + StringTk::uintToStr(nodeID) );

   propagateAddedTarget();
}

void HbMgrNotificationTargetAdded::propagateAddedTarget()
{
   // note: we only submit a single target here to keep the UDP messages small

   App* app = Program::getApp();
   DatagramListener* dgramLis = app->getDatagramListener();
   NodeStoreClients* clients = app->getClientNodes();
   NodeStoreServers* metaNodes = app->getMetaNodes();
   NodeStoreServers* storageNodes = app->getStorageNodes();

   UInt16List targetIDs;
   targetIDs.push_back(targetID);

   MapTargetsMsg msg(&targetIDs, nodeID);

   dgramLis->sendToNodesUDPwithAck(storageNodes, &msg);
   dgramLis->sendToNodesUDPwithAck(clients, &msg);
   dgramLis->sendToNodesUDPwithAck(metaNodes, &msg);
}

void HbMgrNotificationRefreshCapacityPools::processNotification()
{
   LOG_DEBUG(__func__, Log_SPAM, std::string("Propagating capacity pools refresh order") );

   propagateRefreshCapacityPools();
}

void HbMgrNotificationRefreshCapacityPools::propagateRefreshCapacityPools()
{
   App* app = Program::getApp();
   DatagramListener* dgramLis = app->getDatagramListener();
   NodeStoreServers* metaNodes = app->getMetaNodes();

   RefreshCapacityPoolsMsg msg;

   dgramLis->sendToNodesUDPwithAck(metaNodes, &msg);
}

void HbMgrNotificationRefreshTargetStates::processNotification()
{
   LOG_DEBUG(__func__, Log_SPAM, std::string("Propagating target states refresh order") );

   propagateRefreshTargetStates();
}

void HbMgrNotificationRefreshTargetStates::propagateRefreshTargetStates()
{
   App* app = Program::getApp();
   DatagramListener* dgramLis = app->getDatagramListener();
   RefreshTargetStatesMsg msg;

   // send to clients
   NodeStoreClients* clientNodes = app->getClientNodes();
   dgramLis->sendToNodesUDPwithAck(clientNodes, &msg);

   // send to MDSs
   NodeStoreServers* metaNodes = app->getMetaNodes();
   dgramLis->sendToNodesUDPwithAck(metaNodes, &msg);

   // send to storage
   NodeStoreServers* storageNodes = app->getStorageNodes();
   dgramLis->sendToNodesUDPwithAck(storageNodes, &msg);
}

void HbMgrNotificationMirrorBuddyGroupAdded::processNotification()
{
   LOG_DEBUG(__func__, Log_SPAM,
      "Propagating added mirror buddy group information: " + StringTk::uintToStr(buddyGroupID)
      + "; targets: " + StringTk::uintToStr(primaryTargetID) + ","
      + StringTk::uintToStr(secondaryTargetID));

   propagateAddedMirrorBuddyGroup();
}

void HbMgrNotificationMirrorBuddyGroupAdded::propagateAddedMirrorBuddyGroup()
{
   // note: we only submit a single buddy group here to keep the UDP messages small
   App* app = Program::getApp();
   DatagramListener* dgramLis = app->getDatagramListener();
   NodeStoreServers* metaNodes = app->getMetaNodes();
   NodeStoreClients* clientNodes = app->getClientNodes();
   NodeStoreServers* storageNodes = app->getStorageNodes();

   SetMirrorBuddyGroupMsg msg(primaryTargetID, secondaryTargetID, buddyGroupID, true);

   dgramLis->sendToNodesUDPwithAck(storageNodes, &msg);
   dgramLis->sendToNodesUDPwithAck(clientNodes, &msg);
   dgramLis->sendToNodesUDPwithAck(metaNodes, &msg);
}

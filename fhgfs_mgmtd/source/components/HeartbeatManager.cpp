#include <common/net/message/nodes/HeartbeatMsg.h>
#include <common/net/message/nodes/HeartbeatRequestMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/Random.h>
#include <common/toolkit/SocketTk.h>
#include <common/toolkit/TimeAbs.h>
#include <program/Program.h>
#include "HeartbeatManager.h"

HeartbeatManager::HeartbeatManager(DatagramListener* dgramLis) throw(ComponentInitException) :
   PThread("HBeatMgr")
{
   log.setContext("HBeatMgr");

   this->cfg = Program::getApp()->getConfig();
   this->dgramLis = dgramLis;

   this->clients = Program::getApp()->getClientNodes();
   this->metaNodes = Program::getApp()->getMetaNodes();
   this->storageNodes = Program::getApp()->getStorageNodes();
}

HeartbeatManager::~HeartbeatManager()
{
   while(!notificationList.empty() )
   {
      delete(notificationList.front() );
      notificationList.pop_front();
   }
}


void HeartbeatManager::run()
{
   try
   {
      registerSignalHandler();

      mgmtInit();

      requestLoop();

      log.log(Log_DEBUG, "Component stopped.");
   }
   catch(std::exception& e)
   {
      Program::getApp()->handleComponentException(e);
   }

   saveDirtyNodeStores();
}

void HeartbeatManager::requestLoop()
{
   const int sleepTimeMS = 5000;
   bool skipSleepThisTime = false;

   const unsigned storeSaveIntervalMS = 900000;
   const unsigned clientsIntervalMS = 300000;

   Time lastStoreSaveT;
   Time lastClientsT;


   while(!waitForSelfTerminateOrder(skipSleepThisTime ? 0 : sleepTimeMS) )
   {
      skipSleepThisTime = false;

      if(processNotificationList() )
         skipSleepThisTime = true;

      // save stores
      if(lastStoreSaveT.elapsedMS() > storeSaveIntervalMS)
      {
         saveDirtyNodeStores();

         lastStoreSaveT.setToNow();
      }

      // check clients
      if(lastClientsT.elapsedMS() > clientsIntervalMS)
      {
         performClientsCheck();

         lastClientsT.setToNow();
      }

   } // end of while loop
}

void HeartbeatManager::mgmtInit()
{
   log.log(Log_NOTICE, "Notifying stored nodes...");

   broadcastHeartbeat();

   log.log(Log_NOTICE, "Init complete.");
}

/**
 * Send heartbeat to registered nodes.
 */
void HeartbeatManager::broadcastHeartbeat()
{
   const int numRetries = 1;
   const int timeoutMS = 200;

   App* app = Program::getApp();
   Node* localNode = app->getLocalNode();
   std::string localNodeID = localNode->getID();
   uint16_t localNodeNumID = localNode->getNumID();
   uint16_t rootNodeID = metaNodes->getRootNodeNumID();
   NicAddressList nicList(localNode->getNicList() );
   const BitStore* nodeFeatureFlags = localNode->getNodeFeatures();

   HeartbeatMsg hbMsg(localNodeID.c_str(), localNodeNumID, NODETYPE_Mgmt, &nicList,
      nodeFeatureFlags);
   hbMsg.setRootNumID(rootNodeID);
   hbMsg.setPorts(cfg->getConnMgmtdPortUDP(), cfg->getConnMgmtdPortTCP() );
   hbMsg.setFhgfsVersion(BEEGFS_VERSION_CODE);

   dgramLis->sendToNodesUDP(storageNodes, &hbMsg, numRetries, timeoutMS);
   dgramLis->sendToNodesUDP(metaNodes, &hbMsg, numRetries, timeoutMS);
   dgramLis->sendToNodesUDP(clients, &hbMsg, numRetries, timeoutMS);
}


/**
 * @param rootIDHint empty string to auto-define root or a nodeID that is assumed to be the root
 * @return true if a new root node has been defined
 */
bool HeartbeatManager::initRootNode(uint16_t rootIDHint)
{
   // be careful: this method is also called from other threads
   // note: after this method, the root node might still be undefined (this is normal)

   bool setRootRes = false;

   if(rootIDHint || metaNodes->getSize() )
   { // check whether root has already been set

      if(!rootIDHint)
         rootIDHint = metaNodes->getLowestNodeID();

      // set root to lowest ID (if no other root was set yet)
      setRootRes = metaNodes->setRootNodeNumID(rootIDHint, false);

      if(setRootRes)
      { // new root set
         log.log(Log_CRITICAL, "New root directory metadata node: " +
            Program::getApp()->getMetaNodes()->getNodeIDWithTypeStr(rootIDHint) );

         notifyAsyncAddedNode("", rootIDHint, NODETYPE_Meta); /* (real string ID will
            be retrieved by notifier before sending the heartbeat) */
      }
   }

   return setRootRes;
}


bool HeartbeatManager::checkNodeComm(Node* node, NodeType nodeType, Time* lastHbT)
{
   unsigned numRetries = 3;
   unsigned tryTimeoutMS = 2000;

   bool checkRes = false;

   for(unsigned i=0; (i <= numRetries) && !getSelfTerminate(); i++)
   {
      requestHeartbeat(node);

      if(node->waitForNewHeartbeatT(lastHbT, tryTimeoutMS) )
      {
         checkRes = true;
         break;
      }
   }

   return checkRes;
}


void HeartbeatManager::requestHeartbeat(Node* node)
{
   // send request msg to all available node interfaces

   HeartbeatRequestMsg msg;

   dgramLis->sendMsgToNode(node, &msg);
}

/**
 * Check reachability of clients and remove unreachable clients based on
 * cfg->getTuneClientAutoRemoveMins
 */
void HeartbeatManager::performClientsCheck()
{
   unsigned autoRemoveMins = cfg->getTuneClientAutoRemoveMins(); // 0 means disabled

   if(!autoRemoveMins)
      return;


   Node* node = clients->referenceFirstNode();

   while(node && !getSelfTerminate() )
   {
      Time lastHbT = node->getLastHeartbeatT();
      std::string nodeID = node->getID();
      uint16_t nodeNumID = node->getNumID();

      bool checkRes = checkNodeComm(node, NODETYPE_Client, &lastHbT);

      if(checkRes)
      { // node is alive
         //log.log(Log_DEBUG, std::string("Received heartbeat from node: ") + nextNode->getID() +
         //   " (Type: " + Node::nodeTypeToStr(nodeType) + ")");
      }
      else
      { // node is not responding
         unsigned hbElapsedMins = lastHbT.elapsedMS() / (1000*60);

         if(autoRemoveMins && (hbElapsedMins >= autoRemoveMins) )
         { // auto-remove node
            log.log(Log_WARNING, std::string("Node is not responding and will be removed: ") +
               node->getNodeIDWithTypeStr() );

            clients->deleteNode(nodeID);

            log.log(Log_WARNING, std::string("Number of remaining nodes: ") +
                     StringTk::intToStr(clients->getSize() ) +
                     " (Type: " + node->getNodeTypeStr() + ")");

            // add removed node to notification list
            notifyAsyncRemovedNode(nodeID, nodeNumID, NODETYPE_Client);
         }
      }

      // prepare next round
      node = clients->referenceNextNodeAndReleaseOld(node);
   }


   if(node) // note: "node!=NULL" here might happen if selfTerminate is set
      clients->releaseNode(&node);
}

void HeartbeatManager::notifyAsyncAddedNode(std::string nodeID, uint16_t nodeNumID,
   NodeType nodeType)
{
   HbMgrNotification* notification = new HbMgrNotificationNodeAdded(nodeID, nodeNumID, nodeType);

   SafeMutexLock notificationLock(&notificationListMutex);

   notificationList.push_back(notification);

   notificationLock.unlock();
}

void HeartbeatManager::notifyAsyncRemovedNode(std::string nodeID, uint16_t nodeNumID,
   NodeType nodeType)
{
   HbMgrNotification* notification = new HbMgrNotificationNodeRemoved(nodeID, nodeNumID, nodeType);

   SafeMutexLock notificationLock(&notificationListMutex);

   notificationList.push_back(notification);

   notificationLock.unlock();
}

void HeartbeatManager::notifyAsyncAddedTarget(uint16_t targetID, uint16_t nodeID)
{
   HbMgrNotification* notification = new HbMgrNotificationTargetAdded(targetID, nodeID);

   SafeMutexLock notificationLock(&notificationListMutex);

   notificationList.push_back(notification);

   notificationLock.unlock();
}

void HeartbeatManager::notifyAsyncAddedMirrorBuddyGroup(uint16_t buddyGroupID,
   uint16_t primaryTargetID, uint16_t secondaryTargetID)
{
   HbMgrNotification* notification = new HbMgrNotificationMirrorBuddyGroupAdded(buddyGroupID,
      primaryTargetID, secondaryTargetID);

   SafeMutexLock notificationLock(&notificationListMutex);

   notificationList.push_back(notification);

   notificationLock.unlock();
}


void HeartbeatManager::notifyAsyncRefreshCapacityPools()
{
   HbMgrNotification* notification = new HbMgrNotificationRefreshCapacityPools();

   SafeMutexLock notificationLock(&notificationListMutex);

   notificationList.push_back(notification);

   notificationLock.unlock();
}

void HeartbeatManager::notifyAsyncRefreshTargetStates()
{
   HbMgrNotification* notification = new HbMgrNotificationRefreshTargetStates();

   SafeMutexLock notificationLock(&notificationListMutex);

   notificationList.push_back(notification);

   notificationLock.unlock();
}

/**
 * Note: Processes only a single (!) entry each time it is called (to avoid long locking).
 *
 * @return true if more events are waiting in the queue
 */
bool HeartbeatManager::processNotificationList()
{
   bool retVal;

   SafeMutexLock listLock(&this->notificationListMutex); // L O C K

   if(!notificationList.empty() )
   {
      HbMgrNotification* currentNotification = *notificationList.begin();

      notificationList.pop_front();

      /* note: we unlock to avoid blocking other (worker) threads that add new notifications on our
         communication. it's safe because the front of the list won't change. */

      listLock.unlock(); // U N L O C K

      currentNotification->processNotification();

      listLock.relock(); // R E L O C K

      delete(currentNotification);
   }

   retVal = !notificationList.empty();

   listLock.unlock();

   return retVal;
}

void HeartbeatManager::saveDirtyNodeStores()
{
   if(metaNodes->isStoreDirty() )
      metaNodes->saveToFile();

   if(storageNodes->isStoreDirty() )
      storageNodes->saveToFile();

   if(clients->isStoreDirty() )
      clients->saveToFile();
}

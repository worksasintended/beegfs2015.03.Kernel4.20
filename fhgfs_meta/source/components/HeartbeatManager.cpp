#include <common/net/message/nodes/HeartbeatMsg.h>
#include <common/net/message/nodes/HeartbeatRequestMsg.h>
#include <common/net/message/nodes/RefreshCapacityPoolsMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/NodesTk.h>
#include <common/toolkit/Random.h>
#include <common/toolkit/SocketTk.h>
#include <components/ClientSyncer.h>
#include <program/Program.h>
#include <session/SessionStore.h>

#include "HeartbeatManager.h"


HeartbeatManager::HeartbeatManager(DatagramListener* dgramLis) throw(ComponentInitException) :
   PThread("HBeatMgr")
{
   log.setContext("HBeatMgr");

   this->cfg = Program::getApp()->getConfig();
   this->dgramLis = dgramLis;

   this->mgmtNodes = Program::getApp()->getMgmtNodes();
   this->metaNodes = Program::getApp()->getMetaNodes();
   this->storageNodes = Program::getApp()->getStorageNodes();

   this->nodeRegistered = false;
   this->mgmtInitDone = false;

   this->forceTargetStatesUpdate = false;
}

HeartbeatManager::~HeartbeatManager()
{
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

   signalMgmtInitDone();
}

void HeartbeatManager::requestLoop()
{
   ClientSyncer* clientSyncer = Program::getApp()->getClientSyncer();
   InternodeSyncer* internodeSyncer = Program::getApp()->getInternodeSyncer();

   const int sleepTimeMS = 5000;
   const unsigned registrationIntervalMS = 60000;
   const unsigned downloadNodesIntervalMS = 300000;
   const unsigned updateTargetStatesMS = cfg->getSysUpdateTargetStatesSecs() * 1000;

   Time lastRegistrationT;
   Time lastDownloadNodesT;
   Time lastTargetStatesUpdateT;


   while(!waitForSelfTerminateOrder(sleepTimeMS) )
   {
      // registration
      if(!nodeRegistered)
      {
         if((lastRegistrationT.elapsedMS() > registrationIntervalMS) )
         {
            if(registerNode() )
            {
               forceMgmtdPoolsRefresh();
               downloadAndSyncNodes();
               clientSyncer->forceDownload();
               downloadAndSyncTargetMappings();
               downloadAndSyncTargetStatesAndBuddyGroups();
               internodeSyncer->setForcePoolsUpdate();
            }

            lastRegistrationT.setToNow();
         }

         continue;
      }


      // all things below only happen after successful node registration...


      bool doTargetStatesUpdate = getAndResetForceTargetStatesUpdate() ||
         (lastTargetStatesUpdateT.elapsedMS() > updateTargetStatesMS);


      // download & sync nodes
      if(lastDownloadNodesT.elapsedMS() > downloadNodesIntervalMS)
      {
         downloadAndSyncNodes();
         downloadAndSyncTargetMappings();

         lastDownloadNodesT.setToNow();
      }

      // download & sync storage target states and mirror buddy groups
      if(doTargetStatesUpdate)
      {
         downloadAndSyncTargetStatesAndBuddyGroups();

         lastTargetStatesUpdateT.setToNow();
      }


   } // end of while loop
}


void HeartbeatManager::mgmtInit()
{
   ClientSyncer* clientSyncer = Program::getApp()->getClientSyncer();

   log.log(Log_WARNING, "Waiting for management node...");

   if(!NodesTk::waitForMgmtHeartbeat(this, dgramLis, mgmtNodes,
      cfg->getSysMgmtdHost(), cfg->getConnMgmtdPortUDP() ) )
      return;

   log.log(Log_NOTICE, "Management node found. Downloading node groups...");

   downloadAndSyncNodes();
   clientSyncer->downloadAndSyncClients();
   downloadAndSyncTargetMappings();
   downloadAndSyncTargetStatesAndBuddyGroups();

   log.log(Log_NOTICE, "Node registration...");

   if(registerNode() )
   { // download nodes again now that we will receive notifications about add/remove (avoids race)
      forceMgmtdPoolsRefresh();
      downloadAndSyncNodes();
      clientSyncer->downloadAndSyncClients();
      downloadAndSyncTargetMappings();
      downloadAndSyncTargetStatesAndBuddyGroups();
      Program::getApp()->getInternodeSyncer()->setForcePoolsUpdate();
   }

   signalMgmtInitDone();

   log.log(Log_NOTICE, "Init complete.");
}


void HeartbeatManager::downloadAndSyncNodes()
{
   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   if(!mgmtNode)
      return;

   App* app = Program::getApp();
   Node* localNode = app->getLocalNode();

   // metadata nodes

   NodeList metaNodesList;
   UInt16List addedMetaNodes;
   UInt16List removedMetaNodes;
   uint16_t rootNodeID;

   if(NodesTk::downloadNodes(mgmtNode, NODETYPE_Meta, &metaNodesList, true, &rootNodeID) )
   {
      metaNodes->syncNodes(&metaNodesList, &addedMetaNodes, &removedMetaNodes, true,
         localNode);
      if(metaNodes->setRootNodeNumID(rootNodeID, false) )
      {
         log.log(Log_CRITICAL, std::string("Root node ID (from sync results): ") +
            StringTk::uintToStr(rootNodeID) );
         app->getRootDir()->setOwnerNodeID(rootNodeID);
      }

      printSyncResults(NODETYPE_Meta, &addedMetaNodes, &removedMetaNodes);
   }

   // storage nodes

   NodeList storageNodesList;
   UInt16List addedStorageNodes;
   UInt16List removedStorageNodes;

   if(NodesTk::downloadNodes(mgmtNode, NODETYPE_Storage, &storageNodesList, true) )
   {
      storageNodes->syncNodes(&storageNodesList, &addedStorageNodes, &removedStorageNodes, true,
         localNode);
      printSyncResults(NODETYPE_Storage, &addedStorageNodes, &removedStorageNodes);
   }

   mgmtNodes->releaseNode(&mgmtNode);
}

void HeartbeatManager::printSyncResults(NodeType nodeType, UInt16List* addedNodes,
   UInt16List* removedNodes)
{
   if(addedNodes->size() )
      log.log(Log_WARNING, std::string("Nodes added (sync results): ") +
         StringTk::uintToStr(addedNodes->size() ) +
         " (Type: " + Node::nodeTypeToStr(nodeType) + ")");

   if(removedNodes->size() )
      log.log(Log_WARNING, std::string("Nodes removed (sync results): ") +
         StringTk::uintToStr(removedNodes->size() ) +
         " (Type: " + Node::nodeTypeToStr(nodeType) + ")");
}

void HeartbeatManager::downloadAndSyncTargetMappings()
{
   App* app = Program::getApp();
   TargetMapper* targetMapper = app->getTargetMapper();

   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   if(!mgmtNode)
      return;

   UInt16List targetIDs;
   UInt16List nodeIDs;

   bool downloadRes = NodesTk::downloadTargetMappings(mgmtNode, &targetIDs, &nodeIDs, true);
   if(downloadRes)
      targetMapper->syncTargetsFromLists(targetIDs, nodeIDs);

   mgmtNodes->releaseNode(&mgmtNode);
}

/**
 * Download and sync storage target states and mirror buddy groups.
 */
void HeartbeatManager::downloadAndSyncTargetStatesAndBuddyGroups()
{
   App* app = Program::getApp();
   MirrorBuddyGroupMapper* buddyGroupMapper = app->getMirrorBuddyGroupMapper();
   TargetStateStore* targetStates = app->getTargetStateStore();

   static bool downloadFailedLogged = false;

   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   if(!mgmtNode)
      return;

   UInt16List targetIDs;
   UInt8List reachabilityStates;
   UInt8List consistencyStates;

   UInt16List buddyGroupIDs;
   UInt16List primaryTargetIDs;
   UInt16List secondaryTargetIDs;

   bool downloadRes = NodesTk::downloadStatesAndBuddyGroups(mgmtNode,
      &buddyGroupIDs, &primaryTargetIDs, &secondaryTargetIDs,
      &targetIDs, &reachabilityStates, &consistencyStates, true);

   if(downloadRes)
   {
      targetStates->syncStatesAndGroupsFromLists(buddyGroupMapper,
         targetIDs, reachabilityStates, consistencyStates,
         buddyGroupIDs, primaryTargetIDs, secondaryTargetIDs);

      downloadFailedLogged = false;
   }
   else
   { // download failed, so we don't know actual status => carefully set all to poffline

      if(!downloadFailedLogged)
      {
         log.log(Log_WARNING, "Download from mgmtd failed. "
            "Setting all targets to probably-offline.");
         downloadFailedLogged = true;
      }

      targetStates->setAllStates(TargetReachabilityState_POFFLINE);
   }

   mgmtNodes->releaseNode(&mgmtNode);
}

/**
 * @return true if an ack was received for the heartbeat, false otherwise
 */
bool HeartbeatManager::registerNode()
{
   static bool registrationFailureLogged = false; // to avoid log spamming

   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   if(!mgmtNode)
      return false;

   Node* localNode = Program::getApp()->getLocalNode();
   std::string localNodeID = localNode->getID();
   uint16_t localNodeNumID = localNode->getNumID();
   uint16_t rootNodeID = metaNodes->getRootNodeNumID();
   NicAddressList nicList(localNode->getNicList() );
   const BitStore* nodeFeatureFlags = localNode->getNodeFeatures();

   HeartbeatMsg msg(localNodeID.c_str(), localNodeNumID, NODETYPE_Meta, &nicList, nodeFeatureFlags);
   msg.setRootNumID(rootNodeID);
   msg.setPorts(cfg->getConnMetaPortUDP(), cfg->getConnMetaPortTCP() );
   msg.setFhgfsVersion(BEEGFS_VERSION_CODE);

   this->nodeRegistered = dgramLis->sendToNodeUDPwithAck(mgmtNode, &msg);

   mgmtNodes->releaseNode(&mgmtNode);

   if(nodeRegistered)
      log.log(Log_WARNING, "Node registration successful.");
   else
   if(!registrationFailureLogged)
   {
      log.log(Log_CRITICAL, "Node registration not successful. Management node offline? "
         "Will keep on trying...");
      registrationFailureLogged = true;
   }

   return nodeRegistered;
}

/**
 * Tell mgmtd to update its capacity pools (after we have registered ourselves).
 *
 * @return true if an ack was received for our message, false otherwise
 */
bool HeartbeatManager::forceMgmtdPoolsRefresh()
{
   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   if(!mgmtNode)
      return false;

   RefreshCapacityPoolsMsg msg;

   bool ackReceived = dgramLis->sendToNodeUDPwithAck(mgmtNode, &msg);

   mgmtNodes->releaseNode(&mgmtNode);

   return ackReceived;
}

void HeartbeatManager::signalMgmtInitDone()
{
   SafeMutexLock mutexLock(&mgmtInitDoneMutex);

   mgmtInitDone = true;
   mgmtInitDoneCond.broadcast();

   mutexLock.unlock();
}

void HeartbeatManager::waitForMmgtInit()
{
   SafeMutexLock mutexLock(&mgmtInitDoneMutex);

   while(!mgmtInitDone)
      mgmtInitDoneCond.wait(&mgmtInitDoneMutex);

   mutexLock.unlock();
}

bool HeartbeatManager::isMgmtInitDone()
{
   bool retVal;

   SafeMutexLock mutexLock(&mgmtInitDoneMutex);

   retVal = mgmtInitDone;

   mutexLock.unlock();

   return retVal;
}


#include <app/config/Config.h>
#include <app/App.h>
#include <common/threading/SafeMutexLock.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/NodesTk.h>
#include <common/toolkit/Time.h>
#include <common/nodes/NodeStore.h>
#include <common/nodes/TargetCapacityPools.h>
#include <program/Program.h>
#include "InternodeSyncer.h"

InternodeSyncer::InternodeSyncer() throw(ComponentInitException) : PThread("XNodeSync"),
   log("XNodeSync"), serversDownloaded(false)
{
}

InternodeSyncer::~InternodeSyncer()
{
}


void InternodeSyncer::run()
{
   try
   {
      registerSignalHandler();

      // download all nodes,mappings,states and buddy groups
      UInt16List addedStorageNodes;
      UInt16List removedStorageNodes;
      UInt16List addedMetaNodes;
      UInt16List removedMetaNodes;
      bool syncRes = downloadAndSyncNodes(addedStorageNodes, removedStorageNodes, addedMetaNodes,
         removedMetaNodes);
      if (!syncRes)
      {
         log.logErr("Error downloading nodes from mgmtd.");
         Program::getApp()->abort();
         return;
      }

      syncRes = downloadAndSyncTargetMappings();
      if (!syncRes)
      {
         log.logErr("Error downloading target mappings from mgmtd.");
         Program::getApp()->abort();
         return;
      }

      Program::getApp()->getTargetMapper()->getMapping(originalTargetMap);

      syncRes = downloadAndSyncTargetStates();
      if (!syncRes)
      {
         log.logErr("Error downloading target states from mgmtd.");
         Program::getApp()->abort();
         return;
      }

      syncRes = downloadAndSyncMirrorBuddyGroups();
      if ( !syncRes )
      {
         log.logErr("Error downloading mirror buddy groups from mgmtd.");
         Program::getApp()->abort();
         return;
      }

      Program::getApp()->getMirrorBuddyGroupMapper()->getMirrorBuddyGroups(
         originalMirrorBuddyGroupMap);

      {
         SafeMutexLock lock(&serversDownloadedMutex);
         serversDownloaded = true;
         serversDownloadedCondition.signal();
         lock.unlock();
      }

      syncLoop();

      log.log(Log_DEBUG, "Component stopped.");
   }
   catch(std::exception& e)
   {
      PThread::getCurrentThreadApp()->handleComponentException(e);
   }
}

void InternodeSyncer::syncLoop()
{
   const int sleepIntervalMS = 3*1000; // 3sec
   const unsigned downloadNodesAndStatesIntervalMS = 30000; // 30 sec // 5min

   Time lastDownloadNodesAndStatesT;

   while(!waitForSelfTerminateOrder(sleepIntervalMS) )
   {
      // download & sync nodes
      if (lastDownloadNodesAndStatesT.elapsedMS() > downloadNodesAndStatesIntervalMS)
      {
         UInt16List addedStorageNodes;
         UInt16List removedStorageNodes;
         UInt16List addedMetaNodes;
         UInt16List removedMetaNodes;
         bool syncRes = downloadAndSyncNodes(addedStorageNodes, removedStorageNodes, addedMetaNodes,
            removedMetaNodes);

         if (!syncRes)
         {
            log.logErr("Error downloading nodes from mgmtd.");
            Program::getApp()->abort();
            break;
         }

         handleNodeChanges(NODETYPE_Meta, addedMetaNodes, removedMetaNodes);

         handleNodeChanges(NODETYPE_Storage, addedStorageNodes, removedStorageNodes);

         syncRes = downloadAndSyncTargetMappings();
         if (!syncRes)
         {
            log.logErr("Error downloading target mappings from mgmtd.");
            Program::getApp()->abort();
            break;
         }

         handleTargetMappingChanges();

         syncRes = downloadAndSyncTargetStates();
         if (!syncRes)
         {
            log.logErr("Error downloading target states from mgmtd.");
            Program::getApp()->abort();
            break;
         }

         syncRes = downloadAndSyncMirrorBuddyGroups();
         if ( !syncRes )
         {
            log.logErr("Error downloading mirror buddy groups from mgmtd.");
            Program::getApp()->abort();
            break;
         }

         handleBuddyGroupChanges();

         lastDownloadNodesAndStatesT.setToNow();
      }
   }
}

/**
 * @return false on error.
 */
bool InternodeSyncer::downloadAndSyncTargetStates()
{
   App* app = Program::getApp();
   NodeStore* mgmtNodes = app->getMgmtNodes();
   TargetStateStore* targetStateStore = app->getTargetStateStore();

   Node* node = mgmtNodes->referenceFirstNode();
   if(!node)
      return false;

   UInt16List targetIDs;
   UInt8List reachabilityStates;
   UInt8List consistencyStates;

   bool downloadRes = NodesTk::downloadTargetStates(node, NODETYPE_Storage,
      &targetIDs, &reachabilityStates, &consistencyStates, false);

   if(downloadRes)
      targetStateStore->syncStatesFromLists(targetIDs, reachabilityStates,
         consistencyStates);

   mgmtNodes->releaseNode(&node);

   return downloadRes;
}

/**
 * @return false on error
 */
bool InternodeSyncer::downloadAndSyncNodes(UInt16List& addedStorageNodes,
   UInt16List& removedStorageNodes, UInt16List& addedMetaNodes, UInt16List& removedMetaNodes)
{
   const char* logContext = "Nodes sync";

   App* app = Program::getApp();
   NodeStoreServers* mgmtNodes = app->getMgmtNodes();
   NodeStoreServers* metaNodes = app->getMetaNodes();
   NodeStoreServers* storageNodes = app->getStorageNodes();
   Node* localNode = app->getLocalNode();

   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   if(!mgmtNode)
   {
      return false;
   }

   { // storage nodes
      NodeList storageNodesList;

      bool storageRes =
         NodesTk::downloadNodes(mgmtNode, NODETYPE_Storage, &storageNodesList, false);
      if(!storageRes)
      {
         goto err_release_mgmt;
      }

      storageNodes->syncNodes(&storageNodesList, &addedStorageNodes, &removedStorageNodes, true,
         localNode);
      printSyncNodesResults(NODETYPE_Storage, &addedStorageNodes, &removedStorageNodes);
   }

   { // metadata nodes
      NodeList metaNodesList;
      uint16_t rootNodeID;

      bool metaRes =
         NodesTk::downloadNodes(mgmtNode, NODETYPE_Meta, &metaNodesList, false, &rootNodeID);
      if(!metaRes)
         goto err_release_mgmt;

      metaNodes->syncNodes(&metaNodesList, &addedMetaNodes, &removedMetaNodes, true, localNode);

      if(metaNodes->setRootNodeNumID(rootNodeID, false) )
      {
         LogContext(logContext).log(Log_CRITICAL,
            "Root NodeID (from sync results): " + StringTk::uintToStr(rootNodeID) );
      }

      printSyncNodesResults(NODETYPE_Meta, &addedMetaNodes, &removedMetaNodes);
   }

   mgmtNodes->releaseNode(&mgmtNode);

   return true;


err_release_mgmt:
   mgmtNodes->releaseNode(&mgmtNode);

   return false;
}

void InternodeSyncer::printSyncNodesResults(NodeType nodeType, UInt16List* addedNodes,
   UInt16List* removedNodes)
{
   const char* logContext = "Sync results";

   if(addedNodes->size() )
      LogContext(logContext).log(Log_WARNING, std::string("Nodes added: ") +
         StringTk::uintToStr(addedNodes->size() ) +
         " (Type: " + Node::nodeTypeToStr(nodeType) + ")");

   if(removedNodes->size() )
      LogContext(logContext).log(Log_WARNING, std::string("Nodes removed: ") +
         StringTk::uintToStr(removedNodes->size() ) +
         " (Type: " + Node::nodeTypeToStr(nodeType) + ")");
}

/**
 * @return false on error
 */
bool InternodeSyncer::downloadAndSyncTargetMappings()
{
   App* app = Program::getApp();
   NodeStoreServers* mgmtNodes = app->getMgmtNodes();
   TargetMapper* targetMapper = app->getTargetMapper();

   bool retVal = true;

   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   if(!mgmtNode)
      return false;

   UInt16List targetIDs;
   UInt16List nodeIDs;

   bool downloadRes = NodesTk::downloadTargetMappings(mgmtNode, &targetIDs, &nodeIDs, false);
   if(downloadRes)
      targetMapper->syncTargetsFromLists(targetIDs, nodeIDs);
   else
      retVal = false;

   mgmtNodes->releaseNode(&mgmtNode);

   return retVal;
}

/**
 * @return false on error
 */
bool InternodeSyncer::downloadAndSyncMirrorBuddyGroups()
{
   App* app = Program::getApp();
   NodeStoreServers* mgmtNodes = app->getMgmtNodes();
   MirrorBuddyGroupMapper* buddyGroupMapper = app->getMirrorBuddyGroupMapper();

   bool retVal = true;

   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   if(!mgmtNode)
      return false;

   UInt16List buddyGroupIDs;
   UInt16List primaryTargetIDs;
   UInt16List secondaryTargetIDs;

   bool downloadRes = NodesTk::downloadMirrorBuddyGroups(mgmtNode, NODETYPE_Storage, &buddyGroupIDs,
      &primaryTargetIDs, &secondaryTargetIDs, false);

   if(downloadRes)
      buddyGroupMapper->syncGroupsFromLists(buddyGroupIDs, primaryTargetIDs, secondaryTargetIDs);
   else
      retVal = false;

   mgmtNodes->releaseNode(&mgmtNode);

   return retVal;
}

void InternodeSyncer::handleNodeChanges(NodeType nodeType, UInt16List& addedNodes,
   UInt16List& removedNodes)
{
   const char* logContext = "handleNodeChanges";

   if ( addedNodes.size() )
      LogContext(logContext).log(Log_WARNING,
         std::string("Nodes added while beegfs-fsck was running: ")
            + StringTk::uintToStr(addedNodes.size()) + " (Type: " + Node::nodeTypeToStr(nodeType)
            + ")");

   if ( removedNodes.size() )
   {
      // removed nodes must lead to fsck stoppage
      LogContext(logContext).log(Log_WARNING,
         std::string("Nodes removed while beegfs-fsck was running: ")
            + StringTk::uintToStr(removedNodes.size()) + " (Type: " + Node::nodeTypeToStr(nodeType)
            + "); beegfs-fsck cannot continue.");

      Program::getApp()->abort();
   }
}

void InternodeSyncer::handleTargetMappingChanges()
{
   const char* logContext = "handleTargetMappingChanges";

   TargetMap newTargetMap;
   Program::getApp()->getTargetMapper()->getMapping(newTargetMap);

   for ( TargetMapIter originalMapIter = originalTargetMap.begin();
      originalMapIter != originalTargetMap.end(); originalMapIter++ )
   {
      uint16_t targetID = originalMapIter->first;
      uint16_t oldNodeID = originalMapIter->second;

      TargetMapIter newMapIter = newTargetMap.find(targetID);

      if ( newMapIter == newTargetMap.end() )
      {
         LogContext(logContext).log(Log_WARNING,
            "Target removed while beegfs-fsck was running; beegfs-fsck can't continue; targetID: "
               + StringTk::uintToStr(targetID));

         Program::getApp()->abort();
      }
      else
      {
         uint16_t newNodeID = newMapIter->second;
         if ( oldNodeID != newNodeID )
         {
            LogContext(logContext).log(Log_WARNING,
               "Target re-mapped while beegfs-fsck was running; beegfs-fsck can't continue; "
                  "targetID: " + StringTk::uintToStr(targetID));

            Program::getApp()->abort();
         }
      }
   }
}

void InternodeSyncer::handleBuddyGroupChanges()
{
   const char* logContext = "handleBuddyGroupChanges";

   MirrorBuddyGroupMap newMirrorBuddyGroupMap;
   Program::getApp()->getMirrorBuddyGroupMapper()->getMirrorBuddyGroups(newMirrorBuddyGroupMap);

   for ( MirrorBuddyGroupMapIter originalMapIter = originalMirrorBuddyGroupMap.begin();
      originalMapIter != originalMirrorBuddyGroupMap.end(); originalMapIter++ )
   {
      uint16_t buddyGroupID = originalMapIter->first;
      MirrorBuddyGroup oldBuddyGroup = originalMapIter->second;

      MirrorBuddyGroupMapIter newMapIter = newMirrorBuddyGroupMap.find(buddyGroupID);

      if ( newMapIter == newMirrorBuddyGroupMap.end() )
      {
         LogContext(logContext).log(Log_WARNING,
            "Mirror buddy group removed while beegfs-fsck was running; beegfs-fsck can't continue; "
            "groupID: " + StringTk::uintToStr(buddyGroupID));

         Program::getApp()->abort();
      }
      else
      {
         MirrorBuddyGroup newBuddyGroup = newMapIter->second;
         if ( oldBuddyGroup.firstTargetID != newBuddyGroup.firstTargetID )
         {
            LogContext(logContext).log(Log_WARNING,
               "Primary of mirror buddy group changed while beegfs-fsck was running; beegfs-fsck "
               "can't continue; groupID: " + StringTk::uintToStr(buddyGroupID));

            Program::getApp()->abort();
         }
      }
   }
}

void InternodeSyncer::waitForServers()
{
   SafeMutexLock lock(&serversDownloadedMutex);
   while (!serversDownloaded)
      serversDownloadedCondition.wait(&serversDownloadedMutex);
   lock.unlock();
}

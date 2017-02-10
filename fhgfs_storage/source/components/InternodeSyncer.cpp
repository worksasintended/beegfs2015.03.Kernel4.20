#include <app/config/Config.h>
#include <app/App.h>
#include <common/net/message/nodes/ChangeTargetConsistencyStatesMsg.h>
#include <common/net/message/nodes/ChangeTargetConsistencyStatesRespMsg.h>
#include <common/net/message/nodes/GetNodeCapacityPoolsMsg.h>
#include <common/net/message/nodes/GetNodeCapacityPoolsRespMsg.h>
#include <common/net/message/nodes/MapTargetsMsg.h>
#include <common/net/message/nodes/MapTargetsRespMsg.h>
#include <common/net/message/nodes/RefreshCapacityPoolsMsg.h>
#include <common/net/message/nodes/SetTargetConsistencyStatesMsg.h>
#include <common/net/message/nodes/SetTargetConsistencyStatesRespMsg.h>
#include <common/net/message/storage/SetStorageTargetInfoMsg.h>
#include <common/net/message/storage/SetStorageTargetInfoRespMsg.h>
#include <common/net/message/storage/quota/RequestExceededQuotaMsg.h>
#include <common/net/message/storage/quota/RequestExceededQuotaRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/NodesTk.h>
#include <common/toolkit/Time.h>
#include <common/nodes/NodeStore.h>
#include <common/nodes/TargetCapacityPools.h>
#include <net/msghelpers/MsgHelperIO.h>
#include <program/Program.h>
#include "InternodeSyncer.h"

InternodeSyncer::InternodeSyncer() throw(ComponentInitException) : PThread("XNodeSync"),
   log("XNodeSync"), forceTargetStatesUpdate(true)
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

      forceMgmtdPoolsRefresh(); // we're ready & able now, so tell mgmt to update capacity pools

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
   App* app = Program::getApp();
   Config* cfg = app->getConfig();
   LogContext log("SyncLoop");

   const int sleepIntervalMS = 3*1000; // 3sec

   const unsigned sweepNormalMS = 5*1000; // 5sec
   const unsigned sweepStressedMS = 2*1000; // 2sec
   const unsigned idleDisconnectIntervalMS = 70*60*1000; /* 70 minutes (must be less than half the
      streamlis idle disconnect interval to avoid cases where streamlis disconnects first) */
   const unsigned downloadNodesIntervalMS = 600000; // 10min

   const unsigned updateCapacitiesMS = cfg->getSysUpdateTargetStatesSecs() * 2000;
   const unsigned updateTargetStatesMS = cfg->getSysUpdateTargetStatesSecs() * 1000;

   Time lastCacheSweepT;
   Time lastIdleDisconnectT;
   Time lastDownloadNodesT;
   Time lastCapacityUpdateT;
   Time lastTargetStatesUpdateT;

   unsigned currentCacheSweepMS = sweepNormalMS; // (adapted inside the loop below)


   while(!waitForSelfTerminateOrder(sleepIntervalMS) )
   {
      bool targetStatesUpdateForced = getAndResetForceTargetStatesUpdate();

      if(lastCacheSweepT.elapsedMS() > currentCacheSweepMS)
      {
         bool flushTriggered = app->getChunkDirStore()->cacheSweepAsync();

         currentCacheSweepMS = (flushTriggered ? sweepStressedMS : sweepNormalMS);

         log.log(LogTopic_ADVANCED, Log_DEBUG,
               "Last cache sweep elapsed ms: " + StringTk::uintToStr(lastCacheSweepT.elapsedMS()));
         lastCacheSweepT.setToNow();
      }

      if(lastIdleDisconnectT.elapsedMS() > idleDisconnectIntervalMS)
      {
         dropIdleConns();

         log.log(LogTopic_ADVANCED, Log_DEBUG,
               "Drop idle conns elapsed ms: "
               + StringTk::uintToStr(lastIdleDisconnectT.elapsedMS()));
         lastIdleDisconnectT.setToNow();
      }

      // download & sync nodes
      if(lastDownloadNodesT.elapsedMS() > downloadNodesIntervalMS)
      {
         downloadAndSyncNodes();
         downloadAndSyncTargetMappings();
         downloadAndSyncMirrorBuddyGroups();

         log.log(LogTopic_ADVANCED, Log_DEBUG,
               "Download nodes elapsed ms: " + StringTk::uintToStr(lastDownloadNodesT.elapsedMS()));
         lastDownloadNodesT.setToNow();
      }

      if( targetStatesUpdateForced ||
          (lastTargetStatesUpdateT.elapsedMS() > updateTargetStatesMS) )
      {
         updateTargetStatesAndBuddyGroups();

         log.log(LogTopic_ADVANCED, Log_DEBUG,
               "Target states update elapsed ms: "
               + StringTk::uintToStr(lastTargetStatesUpdateT.elapsedMS()));
         lastTargetStatesUpdateT.setToNow();
      }

      if( targetStatesUpdateForced ||
          (lastCapacityUpdateT.elapsedMS() > updateCapacitiesMS) )
      {
         publishTargetCapacities();

         log.log(LogTopic_ADVANCED, Log_DEBUG,
               "Capacity update elapsed ms: "
               + StringTk::uintToStr(lastCapacityUpdateT.elapsedMS()));
         lastCapacityUpdateT.setToNow();
      }
   }
}

/**
 * Drop/reset idle conns from all server stores.
 */
void InternodeSyncer::dropIdleConns()
{
   App* app = Program::getApp();

   unsigned numDroppedConns = 0;

   numDroppedConns += dropIdleConnsByStore(app->getMgmtNodes() );
   numDroppedConns += dropIdleConnsByStore(app->getMetaNodes() );
   numDroppedConns += dropIdleConnsByStore(app->getStorageNodes() );

   if(numDroppedConns)
   {
      log.log(Log_DEBUG, "Dropped idle connections: " + StringTk::uintToStr(numDroppedConns) );
   }
}

/**
 * Walk over all nodes in the given store and drop/reset idle connections.
 *
 * @return number of dropped connections
 */
unsigned InternodeSyncer::dropIdleConnsByStore(NodeStoreServersEx* nodes)
{
   App* app = Program::getApp();

   unsigned numDroppedConns = 0;

   Node* node = nodes->referenceFirstNode();
   while(node)
   {
      /* don't do any idle disconnect stuff with local node
         (the LocalNodeConnPool doesn't support and doesn't need this kind of treatment) */

      if(node != app->getLocalNode() )
      {
         NodeConnPool* connPool = node->getConnPool();

         numDroppedConns += connPool->disconnectAndResetIdleStreams();
      }

      node = nodes->referenceNextNodeAndReleaseOld(node); // iterate to next node
   }

   return numDroppedConns;
}

void InternodeSyncer::updateTargetStatesAndBuddyGroups()
{
   const char* logContext = "Update states and mirror groups";
   LogContext(logContext).log(LogTopic_STATESYNC, Log_DEBUG,
         "Starting target state update.");

   App* app = Program::getApp();
   NodeStore* mgmtNodes = app->getMgmtNodes();
   TargetStateStore* targetStateStore = app->getTargetStateStore();
   StorageTargets* storageTargets = app->getStorageTargets();
   MirrorBuddyGroupMapper* mirrorBuddyGroupMapper = app->getMirrorBuddyGroupMapper();

   static bool downloadFailedLogged = false; // to avoid log spamming
   static bool publishFailedLogged = false; // to avoid log spamming

   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   if (unlikely(!mgmtNode))
   { // should never happen here, because mgmt is downloaded before InternodeSyncer startup
      LogContext(logContext).log(LogTopic_STATESYNC, Log_ERR, "Management node not defined.");
      return;
   }

   UInt16List buddyGroupIDs;
   UInt16List primaryTargetIDs;
   UInt16List secondaryTargetIDs;

   UInt16List targetIDs;
   UInt8List targetReachabilityStates;
   UInt8List targetConsistencyStates;

   unsigned numRetries = 10; // If publishing states fails 10 times, give up (-> POFFLINE).

   // Note: Publishing states fails if between downloadStatesAndBuddyGroups and
   // publishLocalTargetStateChanges, a state on the mgmtd is changed (e.g. because the primary
   // sets NEEDS_RESYNC for the secondary). In that case, we will retry.

   LogContext(logContext).log(LogTopic_STATESYNC, Log_DEBUG,
         "Beginning target state update...");
   bool publishSuccess = false;

   while (!publishSuccess && (numRetries--) )
   {
      // Clear all the lists in case we are already retrying and the lists contain leftover data
      // from the previous attempt.
      buddyGroupIDs.clear();
      primaryTargetIDs.clear();
      secondaryTargetIDs.clear();
      targetIDs.clear();
      targetReachabilityStates.clear();
      targetConsistencyStates.clear();

      bool downloadRes = NodesTk::downloadStatesAndBuddyGroups(mgmtNode, &buddyGroupIDs,
         &primaryTargetIDs, &secondaryTargetIDs, &targetIDs, &targetReachabilityStates,
         &targetConsistencyStates, true);

      if (!downloadRes)
      {
         if(!downloadFailedLogged)
         {
            LogContext(logContext).log(LogTopic_STATESYNC, Log_WARNING,
               "Downloading target states from management node failed. "
               "Setting all targets to probably-offline.");
            downloadFailedLogged = true;
         }

         targetStateStore->setAllStates(TargetReachabilityState_POFFLINE);

         break;
      }

      downloadFailedLogged = false;

      // Store old states for ChangeTargetConsistencyStatesMsg.
      UInt8List oldConsistencyStates = targetConsistencyStates;

      // Sync buddy groups here, because decideResync depends on it.
      // This is not a problem because if pushing target states fails all targets will be
      // (p)offline anyway.
      targetStateStore->syncStatesAndGroupsFromLists(mirrorBuddyGroupMapper,
         targetIDs, targetReachabilityStates, targetConsistencyStates,
         buddyGroupIDs, primaryTargetIDs, secondaryTargetIDs);

      storageTargets->syncBuddyGroupMap(buddyGroupIDs, primaryTargetIDs, secondaryTargetIDs);

      TargetStateMap statesFromMgmtd;
      StorageTargets::fillTargetStateMap(targetIDs, targetReachabilityStates,
         targetConsistencyStates, statesFromMgmtd);

      TargetStateMap localTargetChangedStates;
      storageTargets->decideResync(statesFromMgmtd, localTargetChangedStates);

      StorageTargets::updateTargetStateLists(localTargetChangedStates, targetIDs,
         targetReachabilityStates, targetConsistencyStates);

      publishSuccess = publishLocalTargetStateChanges(
         targetIDs, oldConsistencyStates, targetConsistencyStates);

      if(publishSuccess)
         storageTargets->checkBuddyNeedsResync();
   }

   if(!publishSuccess)
   {
      if(!publishFailedLogged)
      {
         log.log(LogTopic_STATESYNC, Log_WARNING,
               "Pushing local target states to mgmt failed.");
         publishFailedLogged = true;
      }
   }
   else
      publishFailedLogged = false;

   mgmtNodes->releaseNode(&mgmtNode);
}

void InternodeSyncer::publishTargetCapacities()
{
   App* app = Program::getApp();
   NodeStore* mgmtNodes = app->getMgmtNodes();
   StorageTargets* storageTargets = app->getStorageTargets();

   log.log(LogTopic_STATESYNC, Log_DEBUG, "Publishing target capacity infos.");

   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   if (!mgmtNode)
   {
      log.log(LogTopic_STATESYNC, Log_ERR, "Management node not defined.");
      return;
   }

   UInt16List targetIDs;
   StorageTargetInfoList targetInfoList;

   storageTargets->getAllTargetIDs(&targetIDs);
   storageTargets->generateTargetInfoList(targetIDs, targetInfoList);

   SetStorageTargetInfoMsg msg(NODETYPE_Storage, &targetInfoList);
   RequestResponseArgs rrArgs(mgmtNode, &msg, NETMSGTYPE_SetStorageTargetInfoResp);

#ifndef BEEGFS_DEBUG
   rrArgs.logFlags |= REQUESTRESPONSEARGS_LOGFLAG_CONNESTABLISHFAILED
                    | REQUESTRESPONSEARGS_LOGFLAG_RETRY;
#endif

   bool sendRes = MessagingTk::requestResponse(&rrArgs);

   static bool failureLogged = false;

   if (!sendRes)
   {
      if (!failureLogged)
         log.log(LogTopic_STATESYNC, Log_CRITICAL,
               "Pushing target free space info to management node failed.");

      failureLogged = true;
   }
   else
   {
      SetStorageTargetInfoRespMsg* respMsgCast =
         static_cast<SetStorageTargetInfoRespMsg*>(rrArgs.outRespMsg);

      if ( (FhgfsOpsErr)respMsgCast->getValue() != FhgfsOpsErr_SUCCESS)
         log.log(LogTopic_STATESYNC, Log_CRITICAL,
               "Management did not accept target free space info message.");

      failureLogged = false;
   }
}

void InternodeSyncer::publishTargetState(uint16_t targetID, TargetConsistencyState targetState)
{
   App* app = Program::getApp();
   NodeStore* mgmtNodes = app->getMgmtNodes();

   log.log(Log_DEBUG, "Publishing state for target: " + StringTk::uintToStr(targetID) );

   UInt16List targetIDs(1, targetID);
   UInt8List states(1, targetState);

   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   if (!mgmtNode)
   {
      log.logErr("Management node not defined.");
      return;
   }

   SetTargetConsistencyStatesMsg msg(&targetIDs, &states, true);

   char* respBuf = NULL;
   NetMessage* respMsg = NULL;
   bool sendRes = MessagingTk::requestResponse(mgmtNode, &msg,
      NETMSGTYPE_SetTargetConsistencyStatesResp, &respBuf, &respMsg);

   if (!sendRes)
      log.log(Log_CRITICAL, "Pushing target state to management node failed.");
   else
   {
      SetTargetConsistencyStatesRespMsg* respMsgCast = (SetTargetConsistencyStatesRespMsg*)respMsg;
      if ( (FhgfsOpsErr)respMsgCast->getValue() != FhgfsOpsErr_SUCCESS)
         log.log(Log_CRITICAL, "Management node did not accept target state.");

      SAFE_DELETE(respMsg);
      SAFE_FREE(respBuf);
   }
}

/**
 * Gets a list of target states changes (old/new), and reports the local ones (targets which are
 * present in this storage server's storageTargetDataMap) to the mgmtd.
 */
bool InternodeSyncer::publishLocalTargetStateChanges(UInt16List& targetIDs, UInt8List& oldStates,
      UInt8List& newStates)
{
   App* app = Program::getApp();
   StorageTargets* storageTargets = app->getStorageTargets();
   TargetOfflineWait& targetOfflineWait = storageTargets->getTargetOfflineWait();
   bool someTargetHasOfflineTimeout = targetOfflineWait.anyTargetHasTimeout();

   UInt16List localTargetIDs;
   UInt8List localOldStates;
   UInt8List localNewStates;

   UInt16ListIter targetIDsIter = targetIDs.begin();
   UInt8ListIter oldStatesIter = oldStates.begin();
   UInt8ListIter newStatesIter = newStates.begin();

   for ( ; targetIDsIter != targetIDs.end(); ++targetIDsIter, ++oldStatesIter, ++newStatesIter)
   {
      uint16_t targetID = *targetIDsIter;

      if ( !storageTargets->isLocalTarget(targetID) )
         continue;

      // Don't report targets which have an offline timeout at the moment.
      if (someTargetHasOfflineTimeout && targetOfflineWait.targetHasTimeout(targetID) )
         continue;

      localTargetIDs.push_back(targetID);
      localOldStates.push_back(*oldStatesIter);
      localNewStates.push_back(*newStatesIter);
   }

   return publishTargetStateChanges(localTargetIDs, localOldStates, localNewStates);
}

bool InternodeSyncer::publishTargetStateChanges(UInt16List& targetIDs, UInt8List& oldStates,
   UInt8List& newStates)
{
   App* app = Program::getApp();
   NodeStore* mgmtNodes = app->getMgmtNodes();
   bool res;

   log.log(LogTopic_STATESYNC, Log_DEBUG, "Publishing target state change");

   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   if (!mgmtNode)
   {
      log.log(LogTopic_STATESYNC, Log_ERR, "Management node not defined.");
      return true; // Don't stall indefinitely if we don't have a management node.
   }

   ChangeTargetConsistencyStatesMsg msg(NODETYPE_Storage, &targetIDs, &oldStates, &newStates);

   char* respBuf = NULL;
   NetMessage* respMsg = NULL;
   bool sendRes = MessagingTk::requestResponse(mgmtNode, &msg,
      NETMSGTYPE_ChangeTargetConsistencyStatesResp, &respBuf, &respMsg);

   if (!sendRes)
   {
      log.log(LogTopic_STATESYNC, Log_CRITICAL,
            "Pushing target state changes to management node failed.");
      res = false; // Retry.
   }
   else
   {
      ChangeTargetConsistencyStatesRespMsg* respMsgCast =
         (ChangeTargetConsistencyStatesRespMsg*)respMsg;

      if ( (FhgfsOpsErr)respMsgCast->getValue() != FhgfsOpsErr_SUCCESS)
      {
         log.log(LogTopic_STATESYNC, Log_CRITICAL,
               "Management node did not accept target state changes.");
         res = false; // States were changed while we evaluated the state changed. Try again.
      }
      else
         res = true;

      SAFE_DELETE(respMsg);
      SAFE_FREE(respBuf);
   }

   mgmtNodes->releaseNode(&mgmtNode);

   return res;
}

/**
 * @param outTargetIDs
 * @param outReachabilityStates
 * @param outConsistencyStates
 * @return false on error.
 */
bool InternodeSyncer::downloadAndSyncTargetStates(UInt16List& outTargetIDs,
   UInt8List& outReachabilityStates, UInt8List& outConsistencyStates)
{
   App* app = Program::getApp();
   NodeStore* mgmtNodes = app->getMgmtNodes();
   TargetStateStore* targetStateStore = app->getTargetStateStore();

   Node* node = mgmtNodes->referenceFirstNode();
   if(!node)
      return false;

   bool downloadRes = NodesTk::downloadTargetStates(node, NODETYPE_Storage,
      &outTargetIDs, &outReachabilityStates, &outConsistencyStates, false);

   if(downloadRes)
      targetStateStore->syncStatesFromLists(outTargetIDs, outReachabilityStates,
         outConsistencyStates);

   mgmtNodes->releaseNode(&node);

   return downloadRes;
}

/**
 * @return false on error
 */
bool InternodeSyncer::downloadAndSyncNodes()
{
   const char* logContext = "Nodes sync";
   LogContext(logContext).log(LogTopic_STATESYNC, Log_DEBUG, "Called.");

   App* app = Program::getApp();
   NodeStoreServers* mgmtNodes = app->getMgmtNodes();
   NodeStoreServers* metaNodes = app->getMetaNodes();
   NodeStoreServers* storageNodes = app->getStorageNodes();
   Node* localNode = app->getLocalNode();

   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   if(!mgmtNode)
      return false;


   { // storage nodes
      NodeList storageNodesList;
      UInt16List addedStorageNodes;
      UInt16List removedStorageNodes;

      bool storageRes =
         NodesTk::downloadNodes(mgmtNode, NODETYPE_Storage, &storageNodesList, true);
      if(!storageRes)
         goto err_release_mgmt;

      storageNodes->syncNodes(&storageNodesList, &addedStorageNodes, &removedStorageNodes, true,
         localNode);
      printSyncNodesResults(NODETYPE_Storage, &addedStorageNodes, &removedStorageNodes);
   }


   { // clients
      NodeList clientsList;
      StringList addedClients;
      StringList removedClients;

      bool clientsRes = NodesTk::downloadNodes(mgmtNode, NODETYPE_Client, &clientsList, true);
      if(!clientsRes)
         goto err_release_mgmt;

      // note: storage App doesn't have a client node store, thus no clients->syncNodes() here
      syncClientSessions(&clientsList);
   }


   { // metadata nodes
      NodeList metaNodesList;
      UShortList addedMetaNodes;
      UShortList removedMetaNodes;
      uint16_t rootNodeID;

      bool metaRes =
         NodesTk::downloadNodes(mgmtNode, NODETYPE_Meta, &metaNodesList, true, &rootNodeID);
      if(!metaRes)
         goto err_release_mgmt;

      metaNodes->syncNodes(&metaNodesList, &addedMetaNodes, &removedMetaNodes, true);

      if(metaNodes->setRootNodeNumID(rootNodeID, false) )
      {
         LogContext(logContext).log(LogTopic_STATESYNC, Log_CRITICAL,
            "Root NodeID (from sync results): " + StringTk::uintToStr(rootNodeID) );
      }

      printSyncNodesResults(NODETYPE_Meta, &addedMetaNodes, &removedMetaNodes);
   }


   #ifdef BEEGFS_HSM_DEPRECATED
   { // HSM nodes
      NodeStoreServers* hsmNodes = app->getHsmNodes();

      NodeList hsmNodesList;
      UShortList addedHsmNodes;
      UShortList removedHsmNodes;

      bool hsmRes = NodesTk::downloadNodes(mgmtNode, NODETYPE_Hsm, &hsmNodesList, false);
      if(!hsmRes)
         goto err_release_mgmt;

      hsmNodes->syncNodes(&hsmNodesList, &addedHsmNodes, &removedHsmNodes, true);

      printSyncNodesResults(NODETYPE_Hsm, &addedHsmNodes, &removedHsmNodes);
   }
   #endif

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
      LogContext(logContext).log(LogTopic_STATESYNC, Log_WARNING,
         std::string("Nodes added: ") +
         StringTk::uintToStr(addedNodes->size() ) +
         " (Type: " + Node::nodeTypeToStr(nodeType) + ")");

   if(removedNodes->size() )
      LogContext(logContext).log(LogTopic_STATESYNC, Log_WARNING,
         std::string("Nodes removed: ") +
         StringTk::uintToStr(removedNodes->size() ) +
         " (Type: " + Node::nodeTypeToStr(nodeType) + ")");
}

/**
 * @return false on error
 */
bool InternodeSyncer::downloadAndSyncTargetMappings()
{
   LogContext("Download target mappings").log(LogTopic_STATESYNC, Log_DEBUG,
         "Syncing target mappings.");

   App* app = Program::getApp();
   NodeStoreServers* mgmtNodes = app->getMgmtNodes();
   TargetMapper* targetMapper = app->getTargetMapper();

   bool retVal = true;

   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   if(!mgmtNode)
      return false;

   UInt16List targetIDs;
   UInt16List nodeIDs;

   bool downloadRes = NodesTk::downloadTargetMappings(mgmtNode, &targetIDs, &nodeIDs, true);
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
   LogContext("Downlod mirror groups").log(LogTopic_STATESYNC, Log_DEBUG,
         "Syncing mirror groups.");
   App* app = Program::getApp();
   NodeStoreServers* mgmtNodes = app->getMgmtNodes();
   MirrorBuddyGroupMapper* buddyGroupMapper = app->getMirrorBuddyGroupMapper();
   StorageTargets* storageTargets = app->getStorageTargets();

   bool retVal = true;

   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   if(!mgmtNode)
      return false;

   UInt16List buddyGroupIDs;
   UInt16List primaryTargetIDs;
   UInt16List secondaryTargetIDs;

   bool downloadRes = NodesTk::downloadMirrorBuddyGroups(mgmtNode, NODETYPE_Storage, &buddyGroupIDs,
      &primaryTargetIDs, &secondaryTargetIDs, true);

   if(downloadRes)
   {
      buddyGroupMapper->syncGroupsFromLists(buddyGroupIDs, primaryTargetIDs, secondaryTargetIDs);
      storageTargets->syncBuddyGroupMap(buddyGroupIDs, primaryTargetIDs, secondaryTargetIDs);
   }
   else
      retVal = false;

   mgmtNodes->releaseNode(&mgmtNode);

   return retVal;
}

/**
 * Synchronize local client sessions with registered clients from mgmt to release orphaned sessions.
 *
 * @param clientsList must be ordered; contained nodes will be removed and may no longer be
 * accessed after calling this method.
 */
void InternodeSyncer::syncClientSessions(NodeList* clientsList)
{
   const char* logContext = "Client sessions sync";
   LogContext(logContext).log(LogTopic_STATESYNC, Log_DEBUG, "Client session sync started.");

   App* app = Program::getApp();
   NodeStoreServers* storageNodes = app->getStorageNodes();
   SessionStore* sessions = app->getSessions();

   SessionList removedSessions;
   StringList unremovableSessions;

   sessions->syncSessions(clientsList, &removedSessions, &unremovableSessions);

   // print sessions removal results (upfront)
   if(!removedSessions.empty() || !unremovableSessions.empty() )
   {
      std::ostringstream logMsgStream;
      logMsgStream << "Removing " << removedSessions.size() << " client sessions. ";

      if(unremovableSessions.empty()) // no unremovable sessions
         LogContext(logContext).log(LogTopic_STATESYNC, Log_DEBUG, logMsgStream.str() );
      else
      { // unremovable sessions found => log warning
         logMsgStream << "(" << unremovableSessions.size() << " are unremovable)";
         LogContext(logContext).log(LogTopic_STATESYNC, Log_WARNING, logMsgStream.str() );
      }
   }


   // remove each file of each session
   SessionListIter sessionIter = removedSessions.begin();
   for( ; sessionIter != removedSessions.end(); sessionIter++) // CLIENT SESSIONS LOOP
   { // walk over all client sessions: cleanup each session
      Session* session = *sessionIter;
      std::string sessionID = session->getSessionID();
      SessionLocalFileStore* sessionFiles = session->getLocalFiles();

      SessionLocalFileList removedSessionFiles;
      StringList referencedSessionFiles;

      sessionFiles->removeAllSessions(&removedSessionFiles, &referencedSessionFiles);

      // print sessionFiles results (upfront)
      if(removedSessionFiles.size() || referencedSessionFiles.size() )
      {
         std::ostringstream logMsgStream;
         logMsgStream << sessionID << ": Removing " << removedSessionFiles.size() <<
            " file sessions. " << "(" << referencedSessionFiles.size() << " are referenced)";
         LogContext(logContext).log(LogTopic_STATESYNC, Log_NOTICE, logMsgStream.str() );
      }

      SessionLocalFileListIter fileIter = removedSessionFiles.begin();

      for( ; fileIter != removedSessionFiles.end(); fileIter++) // SESSION FILES LOOP
      { // walk over all files: close
         SessionLocalFile* sessionFile = *fileIter;
         Node* mirrorNode = sessionFile->getMirrorNode();
         int fd = sessionFile->getFD();
         std::string fileHandleID = sessionFile->getFileHandleID();

         // release mirror node reference
         if(mirrorNode)
            storageNodes->releaseNode(&mirrorNode);

         // close file descriptor
         if(fd != -1)
         {
            int closeRes = MsgHelperIO::close(fd);
            if(closeRes)
            { // close error
               LogContext(logContext).logErr("Unable to close local file. "
                  "FD: " + StringTk::intToStr(fd) + ". " +
                  "SysErr: " + System::getErrString() );
            }
            else
            { // success
               LogContext(logContext).log(Log_DEBUG, std::string("Local file closed.") +
                  " FD: " + StringTk::intToStr(fd) + ";" +
                  " HandleID: " + fileHandleID);
            }
         }


         delete(sessionFile);

      } // end of session files loop


      delete(session);

   } // end of client sessions loop
}

/**
 * @return false on error
 */
bool InternodeSyncer::downloadExceededQuotaList(QuotaDataType idType, QuotaLimitType exType,
   UIntList* outIDList, FhgfsOpsErr& error)
{
   App* app = Program::getApp();
   NodeStoreServers* mgmtNodes = app->getMgmtNodes();

   bool retVal = false;

   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   if(!mgmtNode)
      return false;

   RequestExceededQuotaMsg msg(idType, exType);

   bool commRes;
   char* respBuf = NULL;
   NetMessage* respMsg = NULL;
   RequestExceededQuotaRespMsg* respMsgCast = NULL;

   // connect & communicate
   commRes = MessagingTk::requestResponse(mgmtNode, &msg, NETMSGTYPE_RequestExceededQuotaResp,
      &respBuf, &respMsg);
   if(!commRes)
      goto err_exit;

   // handle result
   respMsgCast = (RequestExceededQuotaRespMsg*)respMsg;

   respMsgCast->parseIDList(outIDList);
   error = respMsgCast->getError();

   retVal = true;

   // cleanup
   SAFE_DELETE(respMsg);
   SAFE_FREE(respBuf);

err_exit:
   mgmtNodes->releaseNode(&mgmtNode);

   return retVal;
}

/**
 * @return false on error
 */
bool InternodeSyncer::downloadAllExceededQuotaLists()
{
   const char* logContext = "Exceeded quota sync";

   App* app = Program::getApp();
   Config* cfg = app->getConfig();
   ExceededQuotaStore* exceededQuotaStore = app->getExceededQuotaStore();

   bool retVal = true;

   UIntList tmpExceededUIDsSize;
   UIntList tmpExceededGIDsSize;
   UIntList tmpExceededUIDsInode;
   UIntList tmpExceededGIDsInode;

   FhgfsOpsErr error;

   if(downloadExceededQuotaList(QuotaDataType_USER, QuotaLimitType_SIZE, &tmpExceededUIDsSize,
      error) )
   {
      exceededQuotaStore->updateExceededQuota(&tmpExceededUIDsSize, QuotaDataType_USER,
         QuotaLimitType_SIZE);
   }
   else
   { // error
      LogContext(logContext).logErr("Unable to download exceeded file size quota for users.");
      retVal = false;
   }

   // check if mgmtd supports the feature of global configuration for quota enforcement
   if(error != FhgfsOpsErr_NOTSUPP)
   {
      // enable or disable quota enforcement
      if(error == FhgfsOpsErr_SUCCESS)
      {
         if(!cfg->getQuotaEnableEnforcement() )
         {
            LogContext(logContext).log(Log_DEBUG,
               "Quota enforcement is enabled on the management daemon, "
               "but not in the configuration of this storage server. "
               "The configuration from the management daemon overrides the local setting.");
         }
         else
         {
            LogContext(logContext).log(Log_DEBUG,
               "Quota enforcement enabled by management daemon.");
         }

         cfg->setQuotaEnableEnforcement(true);
      }
      else
      {
         if(cfg->getQuotaEnableEnforcement() )
         {
            LogContext(logContext).log(Log_DEBUG,
               "Quota enforcement is enabled in the configuration of this storage server, "
               "but not on the management daemon. "
               "The configuration from the management daemon overrides the local setting.");
         }
         else
         {
            LogContext(logContext).log(Log_DEBUG,
               "Quota enforcement disabled by management daemon.");
         }

         cfg->setQuotaEnableEnforcement(false);
         return true;
      }
   }

   if(downloadExceededQuotaList(QuotaDataType_GROUP, QuotaLimitType_SIZE, &tmpExceededGIDsSize,
      error) )
   {
      exceededQuotaStore->updateExceededQuota(&tmpExceededGIDsSize, QuotaDataType_GROUP,
         QuotaLimitType_SIZE);
   }
   else
   { // error
      LogContext(logContext).logErr("Unable to download exceeded file size quota for groups.");
      retVal = false;
   }

   if(downloadExceededQuotaList(QuotaDataType_USER, QuotaLimitType_INODE, &tmpExceededUIDsInode,
      error) )
   {
      exceededQuotaStore->updateExceededQuota(&tmpExceededUIDsInode, QuotaDataType_USER,
         QuotaLimitType_INODE);
   }
   else
   { // error
      LogContext(logContext).logErr("Unable to download exceeded file number quota for users.");
      retVal = false;
   }

   if(downloadExceededQuotaList(QuotaDataType_GROUP, QuotaLimitType_INODE, &tmpExceededGIDsInode,
      error) )
   {
      exceededQuotaStore->updateExceededQuota(&tmpExceededGIDsInode, QuotaDataType_GROUP,
         QuotaLimitType_INODE);
   }
   else
   { // error
      LogContext(logContext).logErr("Unable to download exceeded file number quota for groups.");
      retVal = false;
   }

   return retVal;
}

/**
 * Tell mgmtd to update its capacity pools.
 *
 * @return true if an ack was received for our message, false otherwise.
 */
bool InternodeSyncer::forceMgmtdPoolsRefresh()
{
   App* app = Program::getApp();
   DatagramListener* dgramLis = app->getDatagramListener();
   NodeStoreServers* mgmtNodes = app->getMgmtNodes();

   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   if(!mgmtNode)
      return false;

   RefreshCapacityPoolsMsg msg;

   bool ackReceived = dgramLis->sendToNodeUDPwithAck(mgmtNode, &msg);

   mgmtNodes->releaseNode(&mgmtNode);

   return ackReceived;
}

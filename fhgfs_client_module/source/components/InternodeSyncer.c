#include <app/App.h>
#include <net/filesystem/FhgfsOpsRemoting.h>
#include <common/net/message/control/AckMsgEx.h>
#include <common/net/message/nodes/HeartbeatMsgEx.h>
#include <common/net/message/nodes/HeartbeatRequestMsgEx.h>
#include <common/net/message/nodes/RemoveNodeMsgEx.h>
#include <common/net/message/nodes/RemoveNodeRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/NodesTk.h>
#include <common/toolkit/Random.h>
#include <common/toolkit/Time.h>
#include "InternodeSyncer.h"


void InternodeSyncer_init(InternodeSyncer* this, App* app)
{
   // call super constructor
   Thread_init( (Thread*)this, BEEGFS_THREAD_NAME_PREFIX_STR "XNodeSync", __InternodeSyncer_run);

   this->componentValid = fhgfs_true;
   this->forceTargetStatesUpdate = fhgfs_false;

   this->app = app;
   this->cfg = App_getConfig(app);

   this->dgramLis = App_getDatagramListener(app);
   this->helperd = App_getHelperd(app);

   this->mgmtNodes = App_getMgmtNodes(app);
   this->metaNodes = App_getMetaNodes(app);
   this->storageNodes = App_getStorageNodes(app);

   this->nodeRegistered = fhgfs_false;
   this->mgmtInitDone = fhgfs_false;

   this->mgmtInitDoneMutex = Mutex_construct();
   this->mgmtInitDoneCond = Condition_construct();

   this->delayedCloseMutex = Mutex_construct();
   this->delayedCloseQueue = PointerList_construct();

   this->delayedEntryUnlockMutex = Mutex_construct();
   this->delayedEntryUnlockQueue = PointerList_construct();

   this->delayedRangeUnlockMutex = Mutex_construct();
   this->delayedRangeUnlockQueue = PointerList_construct();

   this->forceTargetStatesUpdateMutex = Mutex_construct();
   this->lastSuccessfulTargetStatesUpdateT = Time_construct();
   this->targetOfflineTimeoutMS = Config_getSysTargetOfflineTimeoutSecs(this->cfg) * 1000;
}

struct InternodeSyncer* InternodeSyncer_construct(App* app)
{
   struct InternodeSyncer* this = (InternodeSyncer*)os_kmalloc(sizeof(*this) );

   if(likely(this) )
      InternodeSyncer_init(this, app);

   return this;
}

void InternodeSyncer_uninit(InternodeSyncer* this)
{
   PointerListIter iter;

   // free delayed close Q elements

   PointerListIter_init(&iter, this->delayedCloseQueue);

   while(!PointerListIter_end(&iter) )
   {
      __InternodeSyncer_delayedCloseFreeEntry(this,
         (DelayedCloseEntry*)PointerListIter_value(&iter) );
      PointerListIter_next(&iter);
   }

   PointerList_destruct(this->delayedCloseQueue);
   Mutex_destruct(this->delayedCloseMutex);

   // free delayed entry unlock Q elements

   PointerListIter_init(&iter, this->delayedEntryUnlockQueue);

   while(!PointerListIter_end(&iter) )
   {
      __InternodeSyncer_delayedEntryUnlockFreeEntry(this,
         (DelayedEntryUnlockEntry*)PointerListIter_value(&iter) );
      PointerListIter_next(&iter);
   }

   PointerList_destruct(this->delayedEntryUnlockQueue);
   Mutex_destruct(this->delayedEntryUnlockMutex);

   // free delayed range unlock Q elements

   PointerListIter_init(&iter, this->delayedRangeUnlockQueue);

   while(!PointerListIter_end(&iter) )
   {
      __InternodeSyncer_delayedRangeUnlockFreeEntry(this,
         (DelayedRangeUnlockEntry*)PointerListIter_value(&iter) );
      PointerListIter_next(&iter);
   }

   PointerList_destruct(this->delayedRangeUnlockQueue);
   Mutex_destruct(this->delayedRangeUnlockMutex);

   Mutex_destruct(this->forceTargetStatesUpdateMutex);
   Time_destruct(this->lastSuccessfulTargetStatesUpdateT);

   Condition_destruct(this->mgmtInitDoneCond);
   Mutex_destruct(this->mgmtInitDoneMutex);

   Thread_uninit( (Thread*)this);
}

void InternodeSyncer_destruct(InternodeSyncer* this)
{
   InternodeSyncer_uninit(this);

   os_kfree(this);
}

void __InternodeSyncer_run(Thread* this)
{
   InternodeSyncer* thisCast = (InternodeSyncer*)this;

   const char* logContext = "InternodeSyncer (run)";
   Logger* log = App_getLogger(thisCast->app);

   Logger_logFormatted(log, Log_DEBUG, logContext, "Searching for nodes...");

   __InternodeSyncer_mgmtInit(thisCast);

   _InternodeSyncer_requestLoop(thisCast);

   __InternodeSyncer_signalMgmtInitDone(thisCast);

   if(thisCast->nodeRegistered)
      __InternodeSyncer_unregisterNode(thisCast);

   Logger_log(log, Log_DEBUG, logContext, "Component stopped.");
}

void _InternodeSyncer_requestLoop(InternodeSyncer* this)
{
   const unsigned sleepTimeMS = 5*1000;

   const unsigned mgmtInitIntervalMS = 5000;
   const unsigned reregisterIntervalMS = 300000;
   const unsigned downloadNodesIntervalMS = 180000;
   const unsigned updateTargetStatesMS = Config_getSysUpdateTargetStatesSecs(this->cfg) * 1000;
   const unsigned delayedOpsIntervalMS = 60*1000;
   const unsigned idleDisconnectIntervalMS = 70*60*1000; /* 70 minutes (must be less than half the
      server-side streamlis idle disconnect interval to avoid server disconnecting first) */

   Time* lastMgmtInitT;
   Time* lastReregisterT;
   Time* lastDownloadNodesT;

   Time* lastDelayedOpsT;
   Time* lastIdleDisconnectT;
   Time* lastTargetStatesUpdateT;

   Thread* thisThread = (Thread*)this;

   lastMgmtInitT = Time_construct();
   lastReregisterT = Time_construct();
   lastDownloadNodesT = Time_construct();

   lastDelayedOpsT = Time_construct();
   lastIdleDisconnectT = Time_construct();
   lastTargetStatesUpdateT = Time_construct();

   while(!_Thread_waitForSelfTerminateOrder(thisThread, sleepTimeMS) )
   {
      fhgfs_bool targetStatesUpdateForced =
         InternodeSyncer_getAndResetForceTargetStatesUpdate(this);

      // mgmt init
      if(!this->mgmtInitDone)
      {
         if(Time_elapsedMS(lastMgmtInitT) > mgmtInitIntervalMS)
         {
            __InternodeSyncer_mgmtInit(this);

            Time_setToNow(lastMgmtInitT);
         }

         continue;
      }

      // everything below only happens after successful management init...

      // re-register
      if(Time_elapsedMS(lastReregisterT) > reregisterIntervalMS)
      {
         __InternodeSyncer_reregisterNode(this);

         Time_setToNow(lastReregisterT);
      }

      // download & sync nodes
      if(Time_elapsedMS(lastDownloadNodesT) > downloadNodesIntervalMS)
      {
         __InternodeSyncer_downloadAndSyncNodes(this);
         __InternodeSyncer_downloadAndSyncTargetMappings(this);

         Time_setToNow(lastDownloadNodesT);
      }

      // download target states
      if( targetStatesUpdateForced ||
          (Time_elapsedMS(lastTargetStatesUpdateT) > updateTargetStatesMS) )
      {
         __InternodeSyncer_updateTargetStates(this,
            NODETYPE_Meta, App_getMetaStateStore(this->app) );
         __InternodeSyncer_updateTargetStatesAndBuddyGroups(this);

         Time_setToNow(lastTargetStatesUpdateT);
      }

      // delayed operations (that were left due to interruption by signal or network errors)
      if(Time_elapsedMS(lastDelayedOpsT) > delayedOpsIntervalMS)
      {
         __InternodeSyncer_delayedEntryUnlockComm(this);
         __InternodeSyncer_delayedRangeUnlockComm(this);
         __InternodeSyncer_delayedCloseComm(this);

         Time_setToNow(lastDelayedOpsT);
      }

      // drop idle connections
      if(Time_elapsedMS(lastIdleDisconnectT) > idleDisconnectIntervalMS)
      {
         __InternodeSyncer_dropIdleConns(this);

         Time_setToNow(lastIdleDisconnectT);
      }
   }

   Time_destruct(lastMgmtInitT);
   Time_destruct(lastReregisterT);
   Time_destruct(lastDownloadNodesT);

   Time_destruct(lastDelayedOpsT);
   Time_destruct(lastIdleDisconnectT);
   Time_destruct(lastTargetStatesUpdateT);
}

void __InternodeSyncer_signalMgmtInitDone(InternodeSyncer* this)
{
   Mutex_lock(this->mgmtInitDoneMutex);

   this->mgmtInitDone = fhgfs_true;

   Condition_broadcast(this->mgmtInitDoneCond);

   Mutex_unlock(this->mgmtInitDoneMutex);
}

void __InternodeSyncer_mgmtInit(InternodeSyncer* this)
{
   static fhgfs_bool waitForMgmtLogged = fhgfs_false; // to avoid log spamming

   const char* logContext = "Init";
   Logger* log = App_getLogger(this->app);

   if(!waitForMgmtLogged)
   {
      const char* hostname = Config_getSysMgmtdHost(this->cfg);
      unsigned short port = Config_getConnMgmtdPortUDP(this->cfg);

      Logger_logFormatted(log, Log_WARNING, logContext,
         "Waiting for beegfs-mgmtd@%s:%hu...", hostname, port);
      waitForMgmtLogged = fhgfs_true;
   }

   if(!__InternodeSyncer_waitForMgmtHeartbeat(this) )
      return;

   Logger_log(log, Log_NOTICE, logContext, "Management node found. Downloading node groups...");

   __InternodeSyncer_downloadAndSyncNodes(this);
   __InternodeSyncer_downloadAndSyncTargetMappings(this);
   __InternodeSyncer_updateTargetStates(this,
      NODETYPE_Meta, App_getMetaStateStore(this->app) );
   __InternodeSyncer_updateTargetStatesAndBuddyGroups(this);

   Logger_log(log, Log_NOTICE, logContext, "Node registration...");

   if(__InternodeSyncer_registerNode(this) )
   { // download nodes again now that we will receive notifications about add/remove (avoids race)
      __InternodeSyncer_downloadAndSyncNodes(this);
      __InternodeSyncer_downloadAndSyncTargetMappings(this);
      __InternodeSyncer_updateTargetStates(this,
         NODETYPE_Meta, App_getMetaStateStore(this->app) );
      __InternodeSyncer_updateTargetStatesAndBuddyGroups(this);
   }

   __InternodeSyncer_signalMgmtInitDone(this);

   Logger_log(log, Log_NOTICE, logContext, "Init complete.");
}


/**
 * @param timeoutMS 0 to wait infinitely (which is probably never what you want)
 */
fhgfs_bool InternodeSyncer_waitForMgmtInit(InternodeSyncer* this, int timeoutMS)
{
   fhgfs_bool retVal;

   Mutex_lock(this->mgmtInitDoneMutex);

   if(!timeoutMS)
   {
      while(!this->mgmtInitDone)
            Condition_wait(this->mgmtInitDoneCond, this->mgmtInitDoneMutex);
   }
   else
   if(!this->mgmtInitDone)
      Condition_timedwait(this->mgmtInitDoneCond, this->mgmtInitDoneMutex, timeoutMS);

   retVal = this->mgmtInitDone;

   Mutex_unlock(this->mgmtInitDoneMutex);

   return retVal;
}

fhgfs_bool __InternodeSyncer_waitForMgmtHeartbeat(InternodeSyncer* this)
{
   const int waitTimeoutMS = 400;
   const int numRetries = 1;

   char heartbeatReqBuf[NETMSG_MIN_LENGTH];
   Thread* thisThread = (Thread*)this;

   const char* hostname = Config_getSysMgmtdHost(this->cfg);
   unsigned short port = Config_getConnMgmtdPortUDP(this->cfg);
   fhgfs_in_addr ipAddr;
   int i;

   HeartbeatRequestMsgEx msg;

   if(NodeStoreEx_getSize(this->mgmtNodes) )
      return fhgfs_true;


   // prepare request message

   HeartbeatRequestMsgEx_init(&msg);
   NetMessage_serialize( (NetMessage*)&msg, heartbeatReqBuf, NETMSG_MIN_LENGTH);
   HeartbeatRequestMsgEx_uninit( (NetMessage*)&msg);


   // resolve name, send message, wait for incoming heartbeat

   for(i=0; (i <= numRetries) && !Thread_getSelfTerminate(thisThread); i++)
   {
      fhgfs_bool resolveRes = SocketTk_getHostByName(this->helperd, hostname, &ipAddr);
      if(resolveRes)
      { // we got an ip => send request and wait
         int tryTimeoutWarpMS = Random_getNextInRange(-(waitTimeoutMS/4), waitTimeoutMS/4);

         DatagramListener_sendtoIP(this->dgramLis, heartbeatReqBuf, NETMSG_MIN_LENGTH, 0,
            ipAddr, port);

         if(NodeStoreEx_waitForFirstNode(this->mgmtNodes, waitTimeoutMS + tryTimeoutWarpMS) )
         {
            return fhgfs_true; // heartbeat received => we're done
         }
      }
   }

   return fhgfs_false;
}

/**
 * @return fhgfs_true if an ack was received for the heartbeat, fhgfs_false otherwise
 */
fhgfs_bool __InternodeSyncer_registerNode(InternodeSyncer* this)
{
   static fhgfs_bool registrationFailureLogged = fhgfs_false; // to avoid log spamming

   const char* logContext = "Registration";
   Logger* log = App_getLogger(this->app);

   Node* mgmtNode;

   NoAllocBufferStore* bufStore = App_getMsgBufStore(this->app);
   Node* localNode = App_getLocalNode(this->app);
   const char* localNodeID = Node_getID(localNode);
   NicAddressList* nicList = Node_getNicList(localNode);
   const BitStore* nodeFeatureFlags = Node_getNodeFeatures(localNode);
   const char* ackID = "registration"; // just a dummy for direct reply, not used in ackStore

   HeartbeatMsgEx msg;
   char* respBuf;
   NetMessage* respMsg;
   FhgfsOpsErr requestRes;
   //AckMsgEx* hbResp; // response value not needed currently

   mgmtNode = NodeStoreEx_referenceFirstNode(this->mgmtNodes);
   if(!mgmtNode)
      return fhgfs_false;

   HeartbeatMsgEx_initFromNodeData(&msg, localNodeID, 0, NODETYPE_Client, nicList,
      nodeFeatureFlags);
   HeartbeatMsgEx_setPorts(&msg, Config_getConnClientPortUDP(this->cfg), 0);
   HeartbeatMsgEx_setFhgfsVersion(&msg, BEEGFS_VERSION_CODE);
   HeartbeatMsgEx_setAckID(&msg, ackID);

   // connect & communicate
   requestRes = MessagingTk_requestResponse(this->app, mgmtNode,
      (NetMessage*)&msg, NETMSGTYPE_Ack, &respBuf, &respMsg);

   if(unlikely(requestRes != FhgfsOpsErr_SUCCESS) )
   { // request/response failed
      goto cleanup_request;
   }

   // handle result
   // hbResp = (AckMsgEx*)respMsg; // response value not needed currently

   this->nodeRegistered = fhgfs_true;


   // clean-up
   NetMessage_virtualDestruct(respMsg);
   NoAllocBufferStore_addBuf(bufStore, respBuf);


cleanup_request:
   HeartbeatMsgEx_uninit( (NetMessage*)&msg);

   NodeStoreEx_releaseNode(this->mgmtNodes, &mgmtNode);


   // log registration result
   if(this->nodeRegistered)
      Logger_log(log, Log_WARNING, logContext, "Node registration successful.");
   else
   if(!registrationFailureLogged)
   {
      Logger_log(log, Log_CRITICAL, logContext,
         "Node registration failed. Management node offline? Will keep on trying...");
      registrationFailureLogged = fhgfs_true;
   }

   return this->nodeRegistered;
}

/**
 * Note: This just sends a heartbeat to the mgmt node to re-new our existence information
 * (in case the mgmt node assumed our death for some reason like network errors etc.)
 */
void __InternodeSyncer_reregisterNode(InternodeSyncer* this)
{
   Node* mgmtNode;

   Node* localNode = App_getLocalNode(this->app);
   const char* localNodeID = Node_getID(localNode);
   NicAddressList* nicList = Node_getNicList(localNode);
   const BitStore* nodeFeatureFlags = Node_getNodeFeatures(localNode);

   HeartbeatMsgEx msg;

   mgmtNode = NodeStoreEx_referenceFirstNode(this->mgmtNodes);
   if(!mgmtNode)
      return;

   HeartbeatMsgEx_initFromNodeData(&msg, localNodeID, 0, NODETYPE_Client, nicList,
      nodeFeatureFlags);
   HeartbeatMsgEx_setPorts(&msg, Config_getConnClientPortUDP(this->cfg), 0);
   HeartbeatMsgEx_setFhgfsVersion(&msg, BEEGFS_VERSION_CODE);

   DatagramListener_sendMsgToNode(this->dgramLis, mgmtNode, (NetMessage*)&msg);


   HeartbeatMsgEx_uninit( (NetMessage*)&msg);

   NodeStoreEx_releaseNode(this->mgmtNodes, &mgmtNode);
}

/**
 * @return fhgfs_true if an ack was received for the heartbeat, fhgfs_false otherwise
 */
fhgfs_bool __InternodeSyncer_unregisterNode(InternodeSyncer* this)
{
   const char* logContext = "Deregistration";
   Logger* log = App_getLogger(this->app);

   /* note: be careful not to use datagrams here, because this is called during App stop and hence
         the DGramLis is probably not even listening for responses anymore */

   Node* mgmtNode;

   NoAllocBufferStore* bufStore = App_getMsgBufStore(this->app);
   Node* localNode = App_getLocalNode(this->app);
   const char* localNodeID = Node_getID(localNode);
   uint16_t localNodeNumID = Node_getNumID(localNode);

   RemoveNodeMsgEx msg;
   char* respBuf;
   NetMessage* respMsg;
   FhgfsOpsErr requestRes;
   //RemoveNodeRespMsg* rmResp; // response value not needed currently
   fhgfs_bool nodeUnregistered = fhgfs_false;

   mgmtNode = NodeStoreEx_referenceFirstNode(this->mgmtNodes);
   if(!mgmtNode)
      return fhgfs_false;

   RemoveNodeMsgEx_initFromNodeData(&msg, localNodeID, localNodeNumID, NODETYPE_Client);

   // connect & communicate
   requestRes = MessagingTk_requestResponse(this->app, mgmtNode,
      (NetMessage*)&msg, NETMSGTYPE_RemoveNodeResp, &respBuf, &respMsg);

   if(unlikely(requestRes != FhgfsOpsErr_SUCCESS) )
   { // request/response failed
      goto cleanup_request;
   }

   // handle result
   // rmResp = (RemoveNodeRespMsg*)respMsg; // response value not needed currently

   nodeUnregistered = fhgfs_true;


   // cleanup
   NetMessage_virtualDestruct(respMsg);
   NoAllocBufferStore_addBuf(bufStore, respBuf);


cleanup_request:
   HeartbeatMsgEx_uninit( (NetMessage*)&msg);

   NodeStoreEx_releaseNode(this->mgmtNodes, &mgmtNode);


   // log deregistration result
   if(nodeUnregistered)
      Logger_log(log, Log_WARNING, logContext, "Node deregistration successful.");
   else
   {
      Logger_log(log, Log_CRITICAL, logContext,
         "Node deregistration failed. Management node offline?");
      Logger_logFormatted(log, Log_CRITICAL, logContext,
         "In case you didn't enable automatic removal, you will have to remove this ClientID "
         "manually from the system: %s", localNodeID);
   }

   return nodeUnregistered;
}


void __InternodeSyncer_downloadAndSyncNodes(InternodeSyncer* this)
{
   Node* mgmtNode;
   Node* localNode;

   NodeList metaNodesList;
   UInt16List addedMetaNodes;
   UInt16List removedMetaNodes;
   uint16_t rootNodeID;

   NodeList storageNodesList;
   UInt16List addedStorageNodes;
   UInt16List removedStorageNodes;


   mgmtNode = NodeStoreEx_referenceFirstNode(this->mgmtNodes);
   if(!mgmtNode)
      return;

   localNode = App_getLocalNode(this->app);

   // metadata nodes

   NodeList_init(&metaNodesList);
   UInt16List_init(&addedMetaNodes);
   UInt16List_init(&removedMetaNodes);

   if(NodesTk_downloadNodes(this->app, mgmtNode, NODETYPE_Meta, &metaNodesList, &rootNodeID) )
   {
      NodeStoreEx_syncNodes(this->metaNodes,
         &metaNodesList, &addedMetaNodes, &removedMetaNodes, localNode);
      NodeStoreEx_setRootNodeNumID(this->metaNodes, rootNodeID, fhgfs_false);
      __InternodeSyncer_printSyncResults(this, NODETYPE_Meta, &addedMetaNodes, &removedMetaNodes);
   }

   NodeList_uninit(&metaNodesList);
   UInt16List_uninit(&addedMetaNodes);
   UInt16List_uninit(&removedMetaNodes);

   // storage nodes

   NodeList_init(&storageNodesList);
   UInt16List_init(&addedStorageNodes);
   UInt16List_init(&removedStorageNodes);

   if(NodesTk_downloadNodes(this->app, mgmtNode, NODETYPE_Storage, &storageNodesList, NULL) )
   {
      NodeStoreEx_syncNodes(this->storageNodes,
         &storageNodesList, &addedStorageNodes, &removedStorageNodes, localNode);
      __InternodeSyncer_printSyncResults(this,
         NODETYPE_Storage, &addedStorageNodes, &removedStorageNodes);
   }

   // cleanup

   NodeList_uninit(&storageNodesList);
   UInt16List_uninit(&addedStorageNodes);
   UInt16List_uninit(&removedStorageNodes);

   NodeStoreEx_releaseNode(this->mgmtNodes, &mgmtNode);
}

void __InternodeSyncer_printSyncResults(InternodeSyncer* this, NodeType nodeType,
   UInt16List* addedNodes, UInt16List* removedNodes)
{
   const char* logContext = "Sync";
   Logger* log = App_getLogger(this->app);

   if(UInt16List_length(addedNodes) )
      Logger_logFormatted(log, Log_WARNING, logContext,
         "Nodes added (sync results): %d (Type: %s)",
         (int)UInt16List_length(addedNodes), Node_nodeTypeToStr(nodeType) );

   if(UInt16List_length(removedNodes) )
      Logger_logFormatted(log, Log_WARNING, logContext,
         "Nodes removed (sync results): %d (Type: %s)",
         (int)UInt16List_length(removedNodes), Node_nodeTypeToStr(nodeType) );
}

void __InternodeSyncer_downloadAndSyncTargetMappings(InternodeSyncer* this)
{
   TargetMapper* targetMapper = App_getTargetMapper(this->app);
   Node* mgmtNode;
   fhgfs_bool downloadRes;

   UInt16List targetIDs;
   UInt16List nodeIDs;

   mgmtNode = NodeStoreEx_referenceFirstNode(this->mgmtNodes);
   if(!mgmtNode)
      return;

   UInt16List_init(&targetIDs);
   UInt16List_init(&nodeIDs);

   downloadRes = NodesTk_downloadTargetMappings(this->app, mgmtNode, &targetIDs, &nodeIDs);
   if(downloadRes)
      TargetMapper_syncTargetsFromLists(targetMapper, &targetIDs, &nodeIDs);

   // cleanup

   UInt16List_uninit(&targetIDs);
   UInt16List_uninit(&nodeIDs);

   NodeStoreEx_releaseNode(this->mgmtNodes, &mgmtNode);
}


/**
 * @param nodeType type of states to download (meta, storage).
 * @param targetStateStore the state store with which the downloaded states should be sync'ed (meta,
 *        storage).
 */
extern void __InternodeSyncer_updateTargetStates(InternodeSyncer* this, NodeType nodeType,
   TargetStateStore* targetStateStore)
{
   const char* logContext = "InternodeSyncer";
   Logger* log = App_getLogger(this->app);

   NodeStoreEx* mgmtdNodes = App_getMgmtNodes(this->app);

   Node* mgmtdNode;
   fhgfs_bool downloadRes;

   UInt16List targetIDs;
   UInt8List targetReachabilityStates;
   UInt8List targetConsistencyStates;

   mgmtdNode = NodeStoreEx_referenceFirstNode(mgmtdNodes);
   if(!mgmtdNode)
      return;

   UInt16List_init(&targetIDs);
   UInt8List_init(&targetReachabilityStates);
   UInt8List_init(&targetConsistencyStates);

   downloadRes = NodesTk_downloadTargetStates(this->app, mgmtdNode, nodeType, &targetIDs,
      &targetReachabilityStates, &targetConsistencyStates);

   if(downloadRes)
   {
      TargetStateStore_syncStatesFromLists(targetStateStore, &targetIDs, &targetReachabilityStates,
         &targetConsistencyStates);
      Time_setToNow(this->lastSuccessfulTargetStatesUpdateT);
      if (nodeType == NODETYPE_Meta)
      {
         Logger_log(log, Log_DEBUG, logContext, "Metadata node states synced.");
      }
      else
      {
         Logger_log(log, Log_DEBUG, logContext, "Target states synced.");
      }
   }
   else
   if( (this->targetOfflineTimeoutMS != 0)
      && (Time_elapsedMS(this->lastSuccessfulTargetStatesUpdateT) > this->targetOfflineTimeoutMS) )
   {
      fhgfs_bool setStateRes =
         TargetStateStore_setAllStates(targetStateStore, TargetReachabilityState_OFFLINE);

      if(setStateRes)
         Logger_log(log, Log_WARNING, logContext,
            "Target state sync failed. All targets set to offline.");
   }
   else
   {
      fhgfs_bool setStatesRes =
         TargetStateStore_setAllStates(targetStateStore, TargetReachabilityState_POFFLINE);

      if(setStatesRes)
      {
         Logger_log(log, Log_WARNING, logContext,
            "Target state sync failed. All targets set to probably-offline.");
      }
   }


   UInt16List_uninit(&targetIDs);
   UInt8List_uninit(&targetReachabilityStates);
   UInt8List_uninit(&targetConsistencyStates);
   NodeStoreEx_releaseNode(mgmtdNodes, &mgmtdNode);
}

/**
 * note: currently only downloads storage targets, not meta.
 *
 * @param targetStateStore the state store with which the downloaded states should be sync'ed
 */
void __InternodeSyncer_updateTargetStatesAndBuddyGroups(InternodeSyncer* this)
{
   const char* logContext = "Update states and mirror groups";
   Logger* log = App_getLogger(this->app);

   NodeStoreEx* mgmtdNodes = App_getMgmtNodes(this->app);
   TargetStateStore* targetStateStore = App_getTargetStateStore(this->app);
   MirrorBuddyGroupMapper* buddyGroupMapper = App_getMirrorBuddyGroupMapper(this->app);

   Node* mgmtdNode;
   fhgfs_bool downloadRes;

   UInt16List buddyGroupIDs;
   UInt16List primaryTargetIDs;
   UInt16List secondaryTargetIDs;

   UInt16List targetIDs;
   UInt8List targetReachabilityStates;
   UInt8List targetConsistencyStates;

   mgmtdNode = NodeStoreEx_referenceFirstNode(mgmtdNodes);
   if(!mgmtdNode)
      return;

   UInt16List_init(&buddyGroupIDs);
   UInt16List_init(&primaryTargetIDs);
   UInt16List_init(&secondaryTargetIDs);

   UInt16List_init(&targetIDs);
   UInt8List_init(&targetReachabilityStates);
   UInt8List_init(&targetConsistencyStates);

   downloadRes = NodesTk_downloadStatesAndBuddyGroups(this->app, mgmtdNode,
      &buddyGroupIDs, &primaryTargetIDs, &secondaryTargetIDs,
      &targetIDs, &targetReachabilityStates, &targetConsistencyStates);

   if(downloadRes)
   {
      TargetStateStore_syncStatesAndGroupsFromLists(targetStateStore, buddyGroupMapper,
         &targetIDs, &targetReachabilityStates, &targetConsistencyStates,
         &buddyGroupIDs, &primaryTargetIDs, &secondaryTargetIDs);

      Time_setToNow(this->lastSuccessfulTargetStatesUpdateT);
      Logger_log(log, Log_DEBUG, logContext, "Target states synced.");
   }
   else
   if( (this->targetOfflineTimeoutMS != 0)
      && (Time_elapsedMS(this->lastSuccessfulTargetStatesUpdateT) > this->targetOfflineTimeoutMS) )
   {
      fhgfs_bool setStateRes =
         TargetStateStore_setAllStates(targetStateStore, TargetReachabilityState_OFFLINE);

      if(setStateRes)
         Logger_log(log, Log_WARNING, logContext,
            "Target state sync failed. All targets set to offline.");
   }
   else
   {
      fhgfs_bool setStatesRes =
         TargetStateStore_setAllStates(targetStateStore, TargetReachabilityState_POFFLINE);

      if(setStatesRes)
      {
         Logger_log(log, Log_WARNING, logContext,
            "Target state sync failed. All targets set to probably-offline.");
      }
   }

   // cleanup

   UInt16List_uninit(&buddyGroupIDs);
   UInt16List_uninit(&primaryTargetIDs);
   UInt16List_uninit(&secondaryTargetIDs);

   UInt16List_uninit(&targetIDs);
   UInt8List_uninit(&targetReachabilityStates);
   UInt8List_uninit(&targetConsistencyStates);

   NodeStoreEx_releaseNode(mgmtdNodes, &mgmtdNode);
}


/**
 * Try to close all delayedClose files.
 */
void __InternodeSyncer_delayedCloseComm(InternodeSyncer* this)
{
   PointerListIter iter;

   Mutex_lock(this->delayedCloseMutex); // L O C K

   PointerListIter_init(&iter, this->delayedCloseQueue);

   while(!PointerListIter_end(&iter) && !Thread_getSelfTerminate( (Thread*)this) )
   {
      DelayedCloseEntry* currentClose = PointerListIter_value(&iter);

      EntryInfo* entryInfo;
      RemotingIOInfo ioInfo;
      FhgfsOpsErr closeRes;

      __InternodeSyncer_delayedClosePrepareRemoting(this, currentClose, &entryInfo, &ioInfo);

      // note: unlock, so that more entries can be added to the queue during remoting
      Mutex_unlock(this->delayedCloseMutex); // U N L O C K

      closeRes = FhgfsOpsRemoting_closefileEx(entryInfo, &ioInfo, fhgfs_false);

      Mutex_lock(this->delayedCloseMutex); // R E L O C K

      if(closeRes == FhgfsOpsErr_COMMUNICATION)
      { // comm error => we will try again later
         PointerListIter_next(&iter);
      }
      else
      { /* anything other than communication error means our job is done and we can delete the
           entry */
         __InternodeSyncer_delayedCloseFreeEntry(this, currentClose);

         iter = PointerListIter_remove(&iter);
      }
   }


   Mutex_unlock(this->delayedCloseMutex); // U N L O C K
}

/**
 * Try to unlock all delayedEntryUnlock files.
 */
void __InternodeSyncer_delayedEntryUnlockComm(InternodeSyncer* this)
{
   PointerListIter iter;

   Mutex_lock(this->delayedEntryUnlockMutex); // L O C K

   PointerListIter_init(&iter, this->delayedEntryUnlockQueue);

   while(!PointerListIter_end(&iter) && !Thread_getSelfTerminate( (Thread*)this) )
   {
      DelayedEntryUnlockEntry* currentUnlock = PointerListIter_value(&iter);

      EntryInfo* entryInfo;
      RemotingIOInfo ioInfo;
      FhgfsOpsErr unlockRes;

      __InternodeSyncer_delayedEntryUnlockPrepareRemoting(this, currentUnlock, &entryInfo, &ioInfo);

      // note: unlock, so that more entries can be added to the queue during remoting
      Mutex_unlock(this->delayedEntryUnlockMutex); // U N L O C K

      unlockRes = FhgfsOpsRemoting_flockEntryEx(entryInfo, &ioInfo, currentUnlock->clientFD, 0,
         ENTRYLOCKTYPE_CANCEL, fhgfs_false);

      Mutex_lock(this->delayedEntryUnlockMutex); // R E L O C K

      if(unlockRes == FhgfsOpsErr_COMMUNICATION)
      { // comm error => we will try again later
         PointerListIter_next(&iter);
      }
      else
      { /* anything other than communication error means our job is done and we can delete the
           entry */
         __InternodeSyncer_delayedEntryUnlockFreeEntry(this, currentUnlock);

         iter = PointerListIter_remove(&iter);
      }
   }


   Mutex_unlock(this->delayedEntryUnlockMutex); // U N L O C K
}

/**
 * Try to unlock all delayedRangeUnlock files.
 */
void __InternodeSyncer_delayedRangeUnlockComm(InternodeSyncer* this)
{
   PointerListIter iter;

   Mutex_lock(this->delayedRangeUnlockMutex); // L O C K

   PointerListIter_init(&iter, this->delayedRangeUnlockQueue);

   while(!PointerListIter_end(&iter) && !Thread_getSelfTerminate( (Thread*)this) )
   {
      DelayedRangeUnlockEntry* currentUnlock = PointerListIter_value(&iter);

      EntryInfo* entryInfo;
      RemotingIOInfo ioInfo;
      FhgfsOpsErr unlockRes;

      __InternodeSyncer_delayedRangeUnlockPrepareRemoting(this, currentUnlock, &entryInfo, &ioInfo);

      // note: unlock, so that more entries can be added to the queue during remoting
      Mutex_unlock(this->delayedRangeUnlockMutex); // U N L O C K

      unlockRes = FhgfsOpsRemoting_flockRangeEx(entryInfo, &ioInfo, currentUnlock->ownerPID,
         ENTRYLOCKTYPE_CANCEL, 0, ~0ULL, fhgfs_false);

      Mutex_lock(this->delayedRangeUnlockMutex); // R E L O C K

      if(unlockRes == FhgfsOpsErr_COMMUNICATION)
      { // comm error => we will try again later
         PointerListIter_next(&iter);
      }
      else
      { /* anything other than communication error means our job is done and we can delete the
           entry */
         __InternodeSyncer_delayedRangeUnlockFreeEntry(this, currentUnlock);

         iter = PointerListIter_remove(&iter);
      }
   }


   Mutex_unlock(this->delayedRangeUnlockMutex); // U N L O C K
}


/**
 * Prepare remoting args for delayed close.
 *
 * Note: This uses only references to the closeEntry, so no cleanup call required.
 */
void __InternodeSyncer_delayedClosePrepareRemoting(InternodeSyncer* this,
   DelayedCloseEntry* closeEntry, EntryInfo** outEntryInfo, RemotingIOInfo* outIOInfo)
{
   *outEntryInfo = &closeEntry->entryInfo;

   outIOInfo->app = this->app;
   outIOInfo->fileHandleID = closeEntry->fileHandleID;
   outIOInfo->pattern = NULL;
   outIOInfo->accessFlags = closeEntry->accessFlags;
   outIOInfo->needsAppendLockCleanup = &closeEntry->needsAppendLockCleanup;
   outIOInfo->maxUsedTargetIndex = &closeEntry->maxUsedTargetIndex;
   outIOInfo->firstWriteDone = NULL;
   outIOInfo->userID = 0;
   outIOInfo->groupID = 0;
}

/**
 * Prepare remoting args for delayed entry unlock.
 *
 * Note: This uses only references to the unlockEntry, so no cleanup call required.
 */
void __InternodeSyncer_delayedEntryUnlockPrepareRemoting(InternodeSyncer* this,
   DelayedEntryUnlockEntry* unlockEntry, EntryInfo** outEntryInfo, RemotingIOInfo* outIOInfo)
{
   *outEntryInfo = &unlockEntry->entryInfo;

   outIOInfo->app = this->app;
   outIOInfo->fileHandleID = unlockEntry->fileHandleID;
   outIOInfo->pattern = NULL;
   outIOInfo->accessFlags = 0;
   outIOInfo->maxUsedTargetIndex = NULL;
   outIOInfo->firstWriteDone = NULL;
   outIOInfo->userID = 0;
   outIOInfo->groupID = 0;
}

/**
 * Prepare remoting args for delayed range unlock.
 *
 * Note: This uses only references to the unlockEntry, so no cleanup call required.
 */
void __InternodeSyncer_delayedRangeUnlockPrepareRemoting(InternodeSyncer* this,
   DelayedRangeUnlockEntry* unlockEntry, EntryInfo** outEntryInfo, RemotingIOInfo* outIOInfo)
{
   *outEntryInfo = &unlockEntry->entryInfo;

   outIOInfo->app = this->app;
   outIOInfo->fileHandleID = unlockEntry->fileHandleID;
   outIOInfo->pattern = NULL;
   outIOInfo->accessFlags = 0;
   outIOInfo->maxUsedTargetIndex = NULL;
   outIOInfo->firstWriteDone = NULL;
   outIOInfo->userID = 0;
   outIOInfo->groupID = 0;
}

/**
 * Frees/uninits all sub-fields and kfrees the closeEntry itself (but does not remove it from the
 * queue).
 */
void __InternodeSyncer_delayedCloseFreeEntry(InternodeSyncer* this, DelayedCloseEntry* closeEntry)
{
   EntryInfo_uninit(&closeEntry->entryInfo);

   os_kfree(closeEntry->fileHandleID);

   AtomicInt_uninit(&closeEntry->maxUsedTargetIndex);

   Time_uninit(&closeEntry->ageT);

   os_kfree(closeEntry);
}

/**
 * Frees/uninits all sub-fields and kfrees the unlockEntry itself (but does not remove it from the
 * queue).
 */
void __InternodeSyncer_delayedEntryUnlockFreeEntry(InternodeSyncer* this,
   DelayedEntryUnlockEntry* unlockEntry)
{
   EntryInfo_uninit(&unlockEntry->entryInfo);

   os_kfree(unlockEntry->fileHandleID);

   Time_uninit(&unlockEntry->ageT);

   os_kfree(unlockEntry);
}

/**
 * Frees/uninits all sub-fields and kfrees the unlockEntry itself (but does not remove it from the
 * queue).
 */
void __InternodeSyncer_delayedRangeUnlockFreeEntry(InternodeSyncer* this,
   DelayedRangeUnlockEntry* unlockEntry)
{
   EntryInfo_uninit(&unlockEntry->entryInfo);

   os_kfree(unlockEntry->fileHandleID);

   Time_uninit(&unlockEntry->ageT);

   os_kfree(unlockEntry);
}

/**
 * Add an entry that could not be closed on the server due to communication error and should be
 * retried again later.
 *
 * @param entryInfo will be copied
 * @param ioInfo will be copied
 */
void InternodeSyncer_delayedCloseAdd(InternodeSyncer* this, const EntryInfo* entryInfo,
   const RemotingIOInfo* ioInfo)
{
   DelayedCloseEntry* newClose = (DelayedCloseEntry*)os_kmalloc(sizeof(*newClose) );

   Time_init(&newClose->ageT);

   EntryInfo_dup(entryInfo, &newClose->entryInfo);

   newClose->fileHandleID = StringTk_strDup(ioInfo->fileHandleID);
   newClose->accessFlags = ioInfo->accessFlags;

   newClose->needsAppendLockCleanup =
      (ioInfo->needsAppendLockCleanup && *ioInfo->needsAppendLockCleanup);

   AtomicInt_init(&newClose->maxUsedTargetIndex, AtomicInt_read(ioInfo->maxUsedTargetIndex) );

   Mutex_lock(this->delayedCloseMutex); // L O C K

   PointerList_append(this->delayedCloseQueue, newClose);

   Mutex_unlock(this->delayedCloseMutex); // U N L O C K
}

/**
 * Add an entry that could not be unlocked on the server due to communication error and should be
 * retried again later.
 *
 * @param entryInfo will be copied
 * @param ioInfo will be copied
 */
void InternodeSyncer_delayedEntryUnlockAdd(InternodeSyncer* this, const EntryInfo* entryInfo,
   const RemotingIOInfo* ioInfo, int64_t clientFD)
{
   DelayedEntryUnlockEntry* newUnlock = (DelayedEntryUnlockEntry*)os_kmalloc(sizeof(*newUnlock) );

   Time_init(&newUnlock->ageT);

   EntryInfo_dup(entryInfo, &newUnlock->entryInfo);

   newUnlock->fileHandleID = StringTk_strDup(ioInfo->fileHandleID);
   newUnlock->clientFD = clientFD;

   Mutex_lock(this->delayedEntryUnlockMutex); // L O C K

   PointerList_append(this->delayedEntryUnlockQueue, newUnlock);

   Mutex_unlock(this->delayedEntryUnlockMutex); // U N L O C K
}

/**
 * Add an entry that could not be unlocked on the server due to communication error and should be
 * retried again later.
 *
 * @param entryInfo will be copied
 * @param ioInfo will be copied
 */
void InternodeSyncer_delayedRangeUnlockAdd(InternodeSyncer* this, const EntryInfo* entryInfo,
   const RemotingIOInfo* ioInfo, int ownerPID)
{
   DelayedRangeUnlockEntry* newUnlock = (DelayedRangeUnlockEntry*)os_kmalloc(sizeof(*newUnlock) );

   Time_init(&newUnlock->ageT);

   EntryInfo_dup(entryInfo, &newUnlock->entryInfo);

   newUnlock->fileHandleID = StringTk_strDup(ioInfo->fileHandleID);
   newUnlock->ownerPID = ownerPID;

   Mutex_lock(this->delayedRangeUnlockMutex); // L O C K

   PointerList_append(this->delayedRangeUnlockQueue, newUnlock);

   Mutex_unlock(this->delayedRangeUnlockMutex); // U N L O C K
}

size_t InternodeSyncer_getDelayedCloseQueueSize(InternodeSyncer* this)
{
   size_t retVal;

   Mutex_lock(this->delayedCloseMutex); // L O C K

   retVal = PointerList_length(this->delayedCloseQueue);

   Mutex_unlock(this->delayedCloseMutex); // U N L O C K

   return retVal;
}

size_t InternodeSyncer_getDelayedEntryUnlockQueueSize(InternodeSyncer* this)
{
   size_t retVal;

   Mutex_lock(this->delayedEntryUnlockMutex); // L O C K

   retVal = PointerList_length(this->delayedEntryUnlockQueue);

   Mutex_unlock(this->delayedEntryUnlockMutex); // U N L O C K

   return retVal;
}

size_t InternodeSyncer_getDelayedRangeUnlockQueueSize(InternodeSyncer* this)
{
   size_t retVal;

   Mutex_lock(this->delayedRangeUnlockMutex); // L O C K

   retVal = PointerList_length(this->delayedRangeUnlockQueue);

   Mutex_unlock(this->delayedRangeUnlockMutex); // U N L O C K

   return retVal;
}

/**
 * Drop/reset idle conns from all server stores.
 */
void __InternodeSyncer_dropIdleConns(InternodeSyncer* this)
{
   Logger* log = App_getLogger(this->app);
   const char* logContext = "Idle disconnect";

   unsigned numDroppedConns = 0;

   numDroppedConns += __InternodeSyncer_dropIdleConnsByStore(this, App_getMgmtNodes(this->app) );
   numDroppedConns += __InternodeSyncer_dropIdleConnsByStore(this, App_getMetaNodes(this->app) );
   numDroppedConns += __InternodeSyncer_dropIdleConnsByStore(this, App_getStorageNodes(this->app) );

   if(numDroppedConns)
   {
      Logger_logFormatted(log, Log_DEBUG, logContext, "Dropped idle connections: %u",
         numDroppedConns);
   }
}

/**
 * Walk over all nodes in the given store and drop/reset idle connections.
 *
 * @return number of dropped connections
 */
unsigned __InternodeSyncer_dropIdleConnsByStore(InternodeSyncer* this, NodeStoreEx* nodes)
{
   unsigned numDroppedConns = 0;

   Node* node = NodeStoreEx_referenceFirstNode(nodes);
   while(node)
   {
      NodeConnPool* connPool = Node_getConnPool(node);

      numDroppedConns += NodeConnPool_disconnectAndResetIdleStreams(connPool);

      node = NodeStoreEx_referenceNextNodeAndReleaseOld(nodes, node); // iterate to next node
   }

   return numDroppedConns;
}


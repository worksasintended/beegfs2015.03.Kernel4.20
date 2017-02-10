#include <common/net/message/nodes/GetNodeCapacityPoolsRespMsg.h>
#include <common/net/message/nodes/ChangeTargetConsistencyStatesMsg.h>
#include <common/net/message/nodes/ChangeTargetConsistencyStatesRespMsg.h>
#include <common/net/message/storage/SetStorageTargetInfoMsg.h>
#include <common/net/message/storage/SetStorageTargetInfoRespMsg.h>
#include <common/net/message/storage/quota/RequestExceededQuotaMsg.h>
#include <common/net/message/storage/quota/RequestExceededQuotaRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/NodesTk.h>
#include <common/toolkit/Time.h>
#include <common/nodes/NodeStore.h>
#include <common/nodes/TargetCapacityPools.h>
#include <program/Program.h>
#include <app/App.h>
#include <app/config/Config.h>
#include "InternodeSyncer.h"


InternodeSyncer::InternodeSyncer() throw(ComponentInitException)
   : PThread("XNodeSync"),
     log("XNodeSync"),
     forcePoolsUpdate(true),
     forceTargetStatesUpdate(true)

{
   // nothing to be done here
}

InternodeSyncer::~InternodeSyncer()
{
   // nothing to be done here
}


void InternodeSyncer::run()
{
   try
   {
      registerSignalHandler();

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

   const int sleepIntervalMS = 3*1000; // 3sec

   const unsigned updateCapacityPoolsMS = cfg->getSysUpdateTargetStatesSecs() * 2000;
   const unsigned updateTargetStatesMS = cfg->getSysUpdateTargetStatesSecs() * 1000;
   const unsigned metaCacheSweepNormalMS = 5*1000; // 5sec
   const unsigned metaCacheSweepStressedMS = 2*1000; // 2sec
   const unsigned idleDisconnectIntervalMS = 70*60*1000; /* 70 minutes (must be less than half the
      streamlis idle disconnect interval to avoid cases where streamlis disconnects first) */
   const unsigned updateIDTimeMS = 60 * 1000; // 1 min

   Time lastCapacityUpdateT;
   Time lastMetaCacheSweepT;
   Time lastIdleDisconnectT;
   Time lastTimeIDSet;
   Time lastTargetStatesUpdateT;

   unsigned currentCacheSweepMS = metaCacheSweepNormalMS; // (adapted inside the loop below)


   while(!waitForSelfTerminateOrder(sleepIntervalMS) )
   {
      bool doCapacityPoolsUpdate = getAndResetForcePoolsUpdate()
            || (lastCapacityUpdateT.elapsedMS() > updateCapacityPoolsMS);
      bool doTargetStatesUpdate = getAndResetForceTargetStatesUpdate()
            || (lastTargetStatesUpdateT.elapsedMS() > updateTargetStatesMS);


      if(doCapacityPoolsUpdate)
      {
         updateMetaCapacityPools();

         updateStorageCapacityPools();
         updateTargetBuddyCapacityPools();

         lastCapacityUpdateT.setToNow();
      }

      if(lastMetaCacheSweepT.elapsedMS() > currentCacheSweepMS)
      {
         bool flushTriggered = app->getMetaStore()->cacheSweepAsync();

         currentCacheSweepMS = (flushTriggered ? metaCacheSweepStressedMS : metaCacheSweepNormalMS);

         lastMetaCacheSweepT.setToNow();
      }

      if(lastIdleDisconnectT.elapsedMS() > idleDisconnectIntervalMS)
      {
         dropIdleConns();
         lastIdleDisconnectT.setToNow();
      }

      if(lastTimeIDSet.elapsedMS() > updateIDTimeMS)
      {
         StorageTk::resetIDCounterToNow();
         lastTimeIDSet.setToNow();
      }

      if(doTargetStatesUpdate)
      {
         publishNodeState();
         publishNodeCapacity();

         updateMetaStates();

         lastTargetStatesUpdateT.setToNow();
      }

   }
}

void InternodeSyncer::updateMetaCapacityPools()
{
   NodeCapacityPools* pools = Program::getApp()->getMetaCapacityPools();

   UInt16List listNormal;
   UInt16List listLow;
   UInt16List listEmergency;

   bool downloadRes = downloadCapacityPools(CapacityPoolQuery_META,
      &listNormal, &listLow, &listEmergency);

   if(!downloadRes)
      return;

   pools->syncPoolsFromLists(listNormal, listLow, listEmergency);
}

void InternodeSyncer::updateStorageCapacityPools()
{
   App* app = Program::getApp();
   Config* cfg = app->getConfig();
   TargetCapacityPools* pools = app->getStorageCapacityPools();
   TargetMapper* targetMapper = app->getTargetMapper();

   UInt16List listNormal;
   UInt16List listLow;
   UInt16List listEmergency;

   bool downloadRes = downloadCapacityPools(CapacityPoolQuery_STORAGE,
      &listNormal, &listLow, &listEmergency);

   if(!downloadRes)
      return;

   TargetMap targetMap;

   if(cfg->getSysTargetAttachmentMap() )
      targetMap = *cfg->getSysTargetAttachmentMap(); // user-provided custom assignment map
   else
      targetMapper->getMapping(targetMap);

   pools->syncPoolsFromLists(listNormal, listLow, listEmergency, targetMap);
}

void InternodeSyncer::updateTargetBuddyCapacityPools()
{
   NodeCapacityPools* pools = Program::getApp()->getStorageBuddyCapacityPools();

   UInt16List listNormal;
   UInt16List listLow;
   UInt16List listEmergency;

   bool downloadRes = downloadCapacityPools(CapacityPoolQuery_STORAGEBUDDIES,
      &listNormal, &listLow, &listEmergency);

   if(!downloadRes)
      return;

   pools->syncPoolsFromLists(listNormal, listLow, listEmergency);
}

/**
 * @return false on error
 */
bool InternodeSyncer::downloadCapacityPools(CapacityPoolQueryType poolType,
   UInt16List* outListNormal, UInt16List* outListLow, UInt16List* outListEmergency)
{
   NodeStore* mgmtNodes = Program::getApp()->getMgmtNodes();

   bool retVal = true;

   Node* node = mgmtNodes->referenceFirstNode();
   if(!node)
      return false;

   GetNodeCapacityPoolsMsg msg(poolType);
   RequestResponseArgs rrArgs(node, &msg, NETMSGTYPE_GetNodeCapacityPoolsResp);
   GetNodeCapacityPoolsRespMsg* respMsgCast;

#ifndef BEEGFS_DEBUG
   rrArgs.logFlags |= REQUESTRESPONSEARGS_LOGFLAG_CONNESTABLISHFAILED
                    | REQUESTRESPONSEARGS_LOGFLAG_RETRY;
#endif // BEEGFS_DEBUG

   // connect & communicate
   bool commRes = MessagingTk::requestResponse(&rrArgs);
   if(!commRes)
   {
      retVal = false;
      goto clean_up;
   }

   // handle result
   respMsgCast = (GetNodeCapacityPoolsRespMsg*)rrArgs.outRespMsg;

   respMsgCast->parseLists(outListNormal, outListLow, outListEmergency);

   // cleanup
clean_up:
   mgmtNodes->releaseNode(&node);

   return retVal;
}

/**
 * Download and sync metadata server target states and mirror buddy groups.
 */
void InternodeSyncer::updateMetaStates()
{
   NodeStore* mgmtNodes = Program::getApp()->getMgmtNodes();
   TargetStateStore* targetStateStore = Program::getApp()->getMetaStateStore();

   Node* node = mgmtNodes->referenceFirstNode();
   if(!node)
      return;

   UInt16List targetIDs;
   UInt8List reachabilityStates;
   UInt8List consistencyStates;

   bool downloadRes = NodesTk::downloadTargetStates(node, NODETYPE_Meta, &targetIDs,
      &reachabilityStates, &consistencyStates, true);

   if(downloadRes)
      targetStateStore->syncStatesFromLists(targetIDs, reachabilityStates, consistencyStates);

   mgmtNodes->releaseNode(&node);
}

/**
 * Send local consistency state to mgmt node.
 */
void InternodeSyncer::publishNodeState()
{
   App* app = Program::getApp();
   NodeStore* mgmtNodes = app->getMgmtNodes();
   const uint16_t localNodeID = app->getLocalNodeNumID();

   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   if (!mgmtNode)
      return;

   // As long as we don't have meta-HA, meta consistency state is always GOOD.
   UInt8List nodeStateList(1, TargetConsistencyState_GOOD);
   UInt16List nodeIDList(1, localNodeID);

   ChangeTargetConsistencyStatesMsg msg(NODETYPE_Meta, &nodeIDList, &nodeStateList, &nodeStateList);
   RequestResponseArgs rrArgs(mgmtNode, &msg, NETMSGTYPE_ChangeTargetConsistencyStatesResp);

#ifndef BEEGFS_DEBUG
   rrArgs.logFlags |= REQUESTRESPONSEARGS_LOGFLAG_CONNESTABLISHFAILED
                    | REQUESTRESPONSEARGS_LOGFLAG_RETRY;
#endif // BEEGFS_DEBUG

   bool sendRes = MessagingTk::requestResponse(&rrArgs);

   static bool failureLogged = false;

   if (!sendRes)
   {
      if (!failureLogged)
         log.log(Log_CRITICAL, "Pushing node state to management node failed.");

      failureLogged = true;
   }
   else
   {
      ChangeTargetConsistencyStatesRespMsg* respMsgCast =
         static_cast<ChangeTargetConsistencyStatesRespMsg*>(rrArgs.outRespMsg);

      if ( (FhgfsOpsErr)respMsgCast->getValue() != FhgfsOpsErr_SUCCESS)
         log.log(Log_CRITICAL, "Management node did not accept node state change.");

      failureLogged = false;
   }

   mgmtNodes->releaseNode(&mgmtNode);
}

/**
 * Send local node free space / free inode info to management node.
 */
void InternodeSyncer::publishNodeCapacity()
{
   App* app = Program::getApp();
   NodeStore* mgmtNodes = app->getMgmtNodes();

   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   if (!mgmtNode)
   {
      log.logErr("Management node not defined.");
      return;
   }

   int64_t sizeTotal = 0;
   int64_t sizeFree = 0;
   int64_t inodesTotal = 0;
   int64_t inodesFree = 0;

   std::string metaPath = app->getMetaPath();
   getStatInfo(&sizeTotal, &sizeFree, &inodesTotal, &inodesFree);

   StorageTargetInfo targetInfo(app->getLocalNodeNumID(), metaPath, sizeTotal, sizeFree,
      inodesTotal, inodesFree, TargetConsistencyState_GOOD);
   // Note: As long as we don't have meta-HA, consistency state will always be GOOD.

   StorageTargetInfoList targetInfoList(1, targetInfo);

   SetStorageTargetInfoMsg msg(NODETYPE_Meta, &targetInfoList);
   RequestResponseArgs rrArgs(mgmtNode, &msg, NETMSGTYPE_SetStorageTargetInfoResp);

#ifndef BEEGFS_DEBUG
   rrArgs.logFlags |= REQUESTRESPONSEARGS_LOGFLAG_CONNESTABLISHFAILED
                    | REQUESTRESPONSEARGS_LOGFLAG_RETRY;
#endif // BEEGFS_DEBUG

   bool sendRes = MessagingTk::requestResponse(&rrArgs);

   static bool failureLogged = false;
   if (!sendRes)
   {
      if (!failureLogged)
         log.log(Log_CRITICAL, "Pushing node free space to management node failed.");

      failureLogged = true;
   }
   else
   {
      SetStorageTargetInfoRespMsg* respMsgCast = (SetStorageTargetInfoRespMsg*)rrArgs.outRespMsg;
      if ( (FhgfsOpsErr)respMsgCast->getValue() != FhgfsOpsErr_SUCCESS)
         log.log(Log_CRITICAL, "Management node did not accept free space info message.");

      failureLogged = false;
   }

   mgmtNodes->releaseNode(&mgmtNode);
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

void InternodeSyncer::getStatInfo(int64_t* outSizeTotal, int64_t* outSizeFree,
   int64_t* outInodesTotal, int64_t* outInodesFree)
{
   const char* logContext = "GetStorageTargetInfoMsg (stat path)";

   std::string targetPathStr = Program::getApp()->getMetaPath();

   bool statSuccess = StorageTk::statStoragePath(targetPathStr, outSizeTotal, outSizeFree,
      outInodesTotal, outInodesFree);

   if(unlikely(!statSuccess) )
   { // error
      LogContext(logContext).logErr("Unable to statfs() storage path: " + targetPathStr +
         " (SysErr: " + System::getErrString() );

      *outSizeTotal = -1;
      *outSizeFree = -1;
      *outInodesTotal = -1;
      *outInodesFree = -1;
   }

   // read and use value from manual free space override file (if it exists)
   StorageTk::statStoragePathOverride(targetPathStr, outSizeFree, outInodesFree);
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

   respMsgCast->getExceededQuotaIDs()->swap(*outIDList);
   error = respMsgCast->getError();

   retVal = true;

   // cleanup
   SAFE_DELETE(respMsg);
   SAFE_FREE(respBuf);

err_exit:
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
               "but not in the configuration of this metadata server. "
               "The configuration from the management daemon overrides the local setting.");
         }
         else
         {
            LogContext(logContext).log(Log_DEBUG,
               "Quota enforcement enabled by management daemon.");
         }

         app->getConfig()->setQuotaEnableEnforcement(true);
      }
      else
      {
         if(cfg->getQuotaEnableEnforcement() )
         {
            LogContext(logContext).log(Log_DEBUG,
               "Quota enforcement is enabled in the configuration of this metadata server, "
               "but not on the management daemon. "
               "The configuration from the management daemon overrides the local setting.");
         }
         else
         {
            LogContext(logContext).log(Log_DEBUG,
               "Quota enforcement disabled by management daemon.");
         }

         app->getConfig()->setQuotaEnableEnforcement(false);
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

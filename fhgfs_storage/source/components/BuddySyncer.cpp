#include "BuddySyncer.h"

#include <common/net/message/storage/GetStorageTargetInfoMsg.h>
#include <common/net/message/storage/GetStorageTargetInfoRespMsg.h>
#include <common/toolkit/StringTk.h>
#include <app/config/Config.h>
#include <program/Program.h>

BuddySyncer::BuddySyncer() throw (ComponentInitException)
   : PThread("BuddySync"), log("BuddySync")
{ }

void BuddySyncer::run()
{
   try
   {
      registerSignalHandler();
      syncLoop();
      log.log(Log_DEBUG, "Component stopped.");
   }
   catch (std::exception& e)
   {
      PThread::getCurrentThreadApp()->handleComponentException(e);
   }
}

void BuddySyncer::syncLoop()
{
   Config* cfg = Program::getApp()->getConfig();

   const int sleepIntervalMS = 3*1000; // 3sec
   const unsigned updateBuddyMS = cfg->getSysUpdateTargetStatesSecs() * 2000;

   Time lastBuddyUpdateT;

   while (!waitForSelfTerminateOrder(sleepIntervalMS))
   {
      if (lastBuddyUpdateT.elapsedMS() > updateBuddyMS)
      {
         requestBuddyTargetStates();
         lastBuddyUpdateT.setToNow();
      }
   }
}

void BuddySyncer::requestBuddyTargetStates()
{
   TargetMapper* targetMapper = Program::getApp()->getTargetMapper();
   MirrorBuddyGroupMapper* buddyGroupMapper = Program::getApp()->getMirrorBuddyGroupMapper();
   StorageTargets* storageTargets = Program::getApp()->getStorageTargets();
   NodeStore* storageNodes = Program::getApp()->getStorageNodes();
   TargetStateStore* targetStateStore = Program::getApp()->getTargetStateStore();
   UInt16List localStorageTargetIDs;
   StorageTargetInfoList storageTargetInfoList;

   log.log(LogTopic_STATESYNC, Log_DEBUG, "Requesting buddy target states.");

   storageTargets->getAllTargetIDs(&localStorageTargetIDs);

   // loop over all local targets
   for (UInt16ListIter iter = localStorageTargetIDs.begin();
         iter != localStorageTargetIDs.end();
         iter++)
   {
      uint16_t targetID = *iter;
      TargetConsistencyState buddyTargetConsistencyState;

      // check if target is part of a buddy group
      uint16_t buddyTargetID = buddyGroupMapper->getBuddyTargetID(targetID);
      if (!buddyTargetID)
         continue;

      // this target is part of a buddy group

      uint16_t nodeID = targetMapper->getNodeID(buddyTargetID);
      if (!nodeID)
      { // mapping to node not found
         log.log(LogTopic_STATESYNC, Log_ERR,
            "Node-mapping for target ID " + StringTk::uintToStr(buddyTargetID) + " not found.");
         continue;
      }

      Node* node = storageNodes->referenceNode(nodeID);
      if (!node)
      { // node not found
         log.log(LogTopic_STATESYNC, Log_ERR,
            "Unknown storage node. nodeID: " + StringTk::uintToStr(nodeID) + "; targetID: "
               + StringTk::uintToStr(targetID));
         continue;
      }

      // get reachability state of buddy target ID
      CombinedTargetState currentState;
      targetStateStore->getState(buddyTargetID, currentState);

      bool commRes;
      char* respBuf = NULL;
      NetMessage* respMsg = NULL;
      GetStorageTargetInfoRespMsg* respMsgCast;

      if (currentState.reachabilityState == TargetReachabilityState_ONLINE)
      {
         // communicate
         UInt16List queryTargetIDs;
         queryTargetIDs.push_back(buddyTargetID);
         GetStorageTargetInfoMsg msg(&queryTargetIDs);

         // connect & communicate
         commRes = MessagingTk::requestResponse(node, &msg,
         NETMSGTYPE_GetStorageTargetInfoResp, &respBuf, &respMsg);
         if (!commRes)
         { // communication failed
            log.log(LogTopic_STATESYNC, Log_WARNING,
               "Communication with buddy target failed. "
                  "nodeID: " + StringTk::uintToStr(nodeID) + "; buddy targetID: "
                  + StringTk::uintToStr(buddyTargetID));

            goto cleanup;
         }

         // handle response
         respMsgCast = (GetStorageTargetInfoRespMsg*)respMsg;
         respMsgCast->parseStorageTargetInfos(&storageTargetInfoList);

         // get received target information
         // (note: we only requested a single target info, so the first one must be the
         // requested one)
         buddyTargetConsistencyState = storageTargetInfoList.empty()
            ? TargetConsistencyState_BAD
            : storageTargetInfoList.front().getState();

         // set last comm timestamp, but ignore it if we think buddy needs a resync
         const bool buddyNeedsResync = storageTargets->getBuddyNeedsResync(targetID);
         if ((buddyTargetConsistencyState == TargetConsistencyState_GOOD) && !buddyNeedsResync)
            storageTargets->writeLastBuddyCommTimestamp(targetID);
      }

      cleanup:
      storageNodes->releaseNode(&node);
      SAFE_DELETE(respMsg);
      SAFE_FREE(respBuf);
   }
}


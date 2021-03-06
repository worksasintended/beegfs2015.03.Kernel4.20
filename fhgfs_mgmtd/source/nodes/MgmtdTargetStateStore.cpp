#include <common/net/message/nodes/SetTargetConsistencyStatesMsg.h>
#include <common/net/message/nodes/SetTargetConsistencyStatesRespMsg.h>
#include <common/toolkit/serialization/Serialization.h>
#include <common/toolkit/MessagingTk.h>
#include <components/HeartbeatManager.h>
#include <program/Program.h>

#include "MgmtdTargetStateStore.h"

#define MGMTDTARGETSTATESTORE_TMPFILE_EXT ".tmp"
#define MGMTDTARGETSTATESTORE_SERBUF_SIZE (4*1024*1024)

MgmtdTargetStateStore::MgmtdTargetStateStore(NodeType nodeType) : TargetStateStore(),
   targetsToResyncSetDirty(false),
   nodeType(nodeType)
{
}

/**
 * targetIDs and consistencyStates is a 1:1 mapping.
 * @param setOnline whether to set targets for which a report was received to online and reset their
 *                  lastChangedTime or not.
 */
void MgmtdTargetStateStore::setConsistencyStatesFromLists(const UInt16List& targetIDs,
   const UInt8List& consistencyStates, bool setOnline)
{
   HeartbeatManager* heartbeatManager = Program::getApp()->getHeartbeatMgr();

   bool statesChanged = false;

   SafeRWLock lock(&rwlock, SafeRWLock_WRITE); // L O C K

   UInt16ListConstIter targetIDsIter = targetIDs.begin();
   UInt8ListConstIter consistencyStatesIter = consistencyStates.begin();

   for (; targetIDsIter != targetIDs.end(); ++targetIDsIter, ++consistencyStatesIter)
   {
      const uint16_t targetID = *targetIDsIter;
      TargetStateInfo& targetState = statesMap[targetID];

      const TargetConsistencyState newConsistencyState =
         static_cast<TargetConsistencyState>(*consistencyStatesIter);

      const TargetReachabilityState newReachabilityState =
         setOnline ? TargetReachabilityState_ONLINE : targetState.reachabilityState;

      if (targetState.reachabilityState != newReachabilityState
            || targetState.consistencyState != newConsistencyState)
      {
         targetState.reachabilityState = newReachabilityState;
         targetState.consistencyState = newConsistencyState;

         targetsToResyncUpdate(newConsistencyState, targetID);

         statesChanged = true;
      }

      if (setOnline)
         targetState.lastChangedTime.setToNow();
      // Note: If setOnline is false, we let the timer run out so the target can be POFFLINEd.
   }

   lock.unlock(); // U N L O C K

   if (statesChanged)
      heartbeatManager->notifyAsyncRefreshTargetStates();
}

/**
 * Changes the consistency state for all targets in the targetID list from a consistent known state
 * to a new state, i.e. if for each target the current target state matches the oldState for the
 * target, the change is stored. If for at least one target the oldState does not match the state
 * currently stored in the statesMap, no changes are committed to the statesMap and
 * FhgfsOpsErr_AGAIN is returned.
 *
 * targetIDs, oldStates and newStates is a 1:1 Mapping.
 *
 * @param buddyGroups Pointer to MirrorBuddyGroupMapper. If defined, a switchover will be triggered
 *                    if a secondary comes ONLINE and is GOOD while the primary is OFFLINE. Ignored
 *                    if NULL.
 */
FhgfsOpsErr MgmtdTargetStateStore::changeConsistencyStatesFromLists(const UInt16List& targetIDs,
   const UInt8List& oldStates, const UInt8List& newStates, MirrorBuddyGroupMapper* buddyGroups)
{
   HeartbeatManager* heartbeatManager = Program::getApp()->getHeartbeatMgr();

   const char* logContext = "Change consistency states";

   UInt16ListConstIter targetIDsIter = targetIDs.begin();
   UInt8ListConstIter oldStatesIter = oldStates.begin();
   UInt8ListConstIter newStatesIter = newStates.begin();
   FhgfsOpsErr res = FhgfsOpsErr_SUCCESS;

   typedef std::vector<std::pair<uint16_t, TargetConsistencyState> > StateUpdateVec;
   StateUpdateVec updatedStates;
   updatedStates.reserve(statesMap.size() );

   SafeRWLock lock(&rwlock, SafeRWLock_WRITE); // L O C K

   for ( ; targetIDsIter != targetIDs.end() && oldStatesIter != oldStates.end()
      && newStatesIter != newStates.end();
      ++targetIDsIter, ++oldStatesIter, ++newStatesIter)
   {
      const TargetStateInfo& targetState = statesMap[*targetIDsIter];

      if (targetState.consistencyState == *oldStatesIter)
      {
         updatedStates.push_back(StateUpdateVec::value_type(*targetIDsIter,
            static_cast<TargetConsistencyState>(*newStatesIter) ) );
      }
      else
      {
         res = FhgfsOpsErr_AGAIN;
         break;
      }
   }

   bool statesChanged = false;

   if (res == FhgfsOpsErr_SUCCESS)
   {
      for (StateUpdateVec::const_iterator updatesIter = updatedStates.begin();
           updatesIter != updatedStates.end(); ++updatesIter)
      {
         const uint16_t targetID = updatesIter->first;
         TargetStateInfo& targetState = statesMap[targetID];

         const TargetConsistencyState newConsistencyState = updatesIter->second;

         bool targetComingOnline = targetState.reachabilityState != TargetReachabilityState_ONLINE;

         if (targetState.consistencyState != newConsistencyState
            || targetState.reachabilityState != TargetReachabilityState_ONLINE)
         {
            targetState.consistencyState = newConsistencyState;

            // Set target to online if we successfully received a state report.
            targetState.reachabilityState = TargetReachabilityState_ONLINE;

            targetsToResyncUpdate(newConsistencyState, targetID);

            statesChanged = true;
         }

         targetState.lastChangedTime.setToNow();

         // If a secondary target comes back online and the primary is offline, switch buddies.
         if (buddyGroups &&
             targetComingOnline &&
             (newConsistencyState == TargetConsistencyState_GOOD) )
         {
            buddyGroups->rwlock.writeLock(); // L O C K buddyGroups

            uint16_t primaryTargetID;
            bool getStateRes;
            CombinedTargetState primaryTargetState;

            LOG_DEBUG(logContext, Log_DEBUG, nodeTypeStr(true) + " coming online, checking if "
               "switchover is necessary. " + nodeTypeStr(true) + " ID: "
               + StringTk::uintToStr(targetID) );

            bool isPrimary;
            uint16_t buddyGroupID = buddyGroups->getBuddyGroupIDUnlocked(targetID, &isPrimary);

            if (isPrimary || !buddyGroupID) // If target is primary or unmapped, do nothing.
               goto bgroups_unlock;

            primaryTargetID = buddyGroups->getPrimaryTargetIDUnlocked(buddyGroupID);

            getStateRes = getStateUnlocked(primaryTargetID, primaryTargetState);
            if (!getStateRes)
            {
               LogContext(logContext).log(Log_ERR, "Failed to get state for mirror "
                  + nodeTypeStr(false) + " ID " + StringTk::uintToStr(primaryTargetID) );

               goto bgroups_unlock;
            }
            if (primaryTargetState.reachabilityState == TargetReachabilityState_OFFLINE)
               buddyGroups->switchover(buddyGroupID);

         bgroups_unlock:
            buddyGroups->rwlock.unlock(); // U N L O C K buddyGroups
         }

         // Log if a target is coming back online.
         if (targetComingOnline)
            LogContext(logContext).log(Log_WARNING,
               nodeTypeStr(true) + " is coming online. ID: " + StringTk::uintToStr(targetID) );
      }
   }

   lock.unlock(); // U N L O C K

   if (statesChanged)
      heartbeatManager->notifyAsyncRefreshTargetStates();

   return res;
}

/**
 * Automatically set targets to (P)OFFLINE if there was no state report received in a while.
 * Note: This uses the lastChangedTime of the TargetStateInfo object. It does not update the
 * timestamp though, so that the timestamp is effectively the time of the last non-timeout state
 * change.
 *
 * @param pofflineTimeoutMS Interval in which a target state update must have occurred so the target
                          is not POFFLINEd.
 * @param offlineTimeoutMS Time after which a target is OFFLINEd.
 * @param buddyGroups if provided, a switch will be made in the buddy groups when necessary (may
 *    be NULL).
 * @returns whether any reachability state was changed.
 */
bool MgmtdTargetStateStore::autoOfflineTargets(const unsigned pofflineTimeoutMS,
   const unsigned offlineTimeoutMS, MirrorBuddyGroupMapper* buddyGroups)
{
   const char* logContext = "Auto-offline";
   LogContext(logContext).log(LogTopic_STATESYNC, Log_DEBUG,
         "Checking for offline nodes. NodeType: " + nodeTypeStr(true));

   bool retVal = false;
   UInt16List offlinedTargets;

   SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE); // L O C K targetStates

   /* buddyGroups lock right from the start here is necessary to have state changes and
      corresponding member switch atomic for outside viewers */
   if(buddyGroups)
      buddyGroups->rwlock.writeLock(); // L O C K buddyGroups

   Time nowTime;

   for (TargetStateInfoMapIter stateInfoIter = statesMap.begin(); stateInfoIter != statesMap.end();
        ++stateInfoIter)
   {
      uint16_t targetID = stateInfoIter->first;
      TargetStateInfo& targetStateInfo = stateInfoIter->second;
      const unsigned timeSinceLastUpdateMS = nowTime.elapsedSinceMS(
         &targetStateInfo.lastChangedTime);

      if (timeSinceLastUpdateMS > offlineTimeoutMS)
      {
         if (targetStateInfo.reachabilityState != TargetReachabilityState_OFFLINE)
         {
            LogContext(logContext).log(LogTopic_STATESYNC, Log_WARNING,
               "No state report received from " + nodeTypeStr(false) + " for " +
               StringTk::uintToStr(timeSinceLastUpdateMS / 1000) + " seconds. "
               "Setting " + nodeTypeStr(false) + " to offline. "
               + nodeTypeStr(true) + " ID: " + StringTk::uintToStr(targetID) );

            offlinedTargets.push_back(targetID);

            targetStateInfo.reachabilityState = TargetReachabilityState_OFFLINE;
            retVal = true;
         }
      }
      else
      if (timeSinceLastUpdateMS > pofflineTimeoutMS)
      {
         if (targetStateInfo.reachabilityState != TargetReachabilityState_POFFLINE)
         {
            LogContext(logContext).log(LogTopic_STATESYNC, Log_WARNING,
               "No state report received from " + nodeTypeStr(false) + " for "
               + StringTk::uintToStr(timeSinceLastUpdateMS / 1000) + " seconds. "
               "Setting " + nodeTypeStr(false) + " to probably-offline. " +
               nodeTypeStr(true) + " ID: " + StringTk::uintToStr(targetID) );

            targetStateInfo.reachabilityState = TargetReachabilityState_POFFLINE;
            retVal = true;
         }
      }
   }

   // do switchover in buddy groups if necessary
   if (buddyGroups)
   {
      for (UInt16ListIter targetIDIter = offlinedTargets.begin();
           targetIDIter != offlinedTargets.end(); ++targetIDIter)
      {
         bool isPrimary;
         uint16_t buddyGroupID = buddyGroups->getBuddyGroupIDUnlocked(*targetIDIter, &isPrimary);

         if (!isPrimary) // Target is secondary or unmapped -> no switchover necessary/possble.
            continue;

         uint16_t secondaryTargetID = buddyGroups->getSecondaryTargetIDUnlocked(buddyGroupID);

         // Get secondary's state and make sure it's online and good.
         CombinedTargetState secondaryTargetState;
         bool getStateRes = getStateUnlocked(secondaryTargetID, secondaryTargetState);
         if (!getStateRes)
         {
            LogContext(logContext).log(LogTopic_STATESYNC, Log_ERR,
               "Tried to switch mirror group members, "
               "but refusing to switch because secondary " + nodeTypeStr(false) +
               " state is unknown. Primary " + nodeTypeStr(false) + "ID: "
               + StringTk::uintToStr(*targetIDIter) );
            continue;
         }

         if ( (secondaryTargetState.reachabilityState != TargetReachabilityState_ONLINE) ||
              (secondaryTargetState.consistencyState != TargetConsistencyState_GOOD) )
         {
            LogContext(logContext).log(LogTopic_STATESYNC, Log_ERR,
               "Tried to switch mirror group members, "
               "but refusing to switch due to secondary " + nodeTypeStr(false) + " state. "
               "Secondary " + nodeTypeStr(false) + " ID: " + StringTk::uintToStr(*targetIDIter) +
               "; secondary state: " + TargetStateStore::stateToStr(secondaryTargetState) );
            continue;
         }

         buddyGroups->switchover(buddyGroupID);
      }

      buddyGroups->rwlock.unlock(); // U N L O C K buddyGroups
   }

   safeLock.unlock(); // U N L O C K targetStates

   return retVal;
}

/**
 * If both targets of any mirror buddy group need a resync, sets the primary to GOOD.
 *
 * @returns true if any target state was changed, false otherwise.
 */
bool MgmtdTargetStateStore::resolveDoubleResync()
{
   const char* logContext = "Resolve double resync";
   LogContext(logContext).log(LogTopic_STATESYNC, Log_DEBUG, "Checking for double-resync.");

   App* app = Program::getApp();
   MirrorBuddyGroupMapper* mirrorBuddyGroupMapper = app->getMirrorBuddyGroupMapper();
   bool res = false;

   UInt16List goodTargetsList; // List of targets that should be set to GOOD.

   UInt16List buddyGroupIDs;
   MirrorBuddyGroupList buddyGroups;

   mirrorBuddyGroupMapper->getMappingAsLists(buddyGroupIDs, buddyGroups);

   UInt16ListIter buddyGroupIDIter = buddyGroupIDs.begin();
   MirrorBuddyGroupListIter buddyGroupIter = buddyGroups.begin();

   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   for ( ; buddyGroupIDIter != buddyGroupIDs.end(); ++buddyGroupIDIter, ++buddyGroupIter)
   {
      const MirrorBuddyGroup& mirrorBuddyGroup = *buddyGroupIter;
      const uint16_t primaryTargetID = mirrorBuddyGroup.firstTargetID;
      const uint16_t secondaryTargetID = mirrorBuddyGroup.secondTargetID;

      static const CombinedTargetState onlineNeedsResyncState(
         TargetReachabilityState_ONLINE, TargetConsistencyState_NEEDS_RESYNC);

      CombinedTargetState primaryState;
      const bool primaryStateRes = getStateUnlocked(primaryTargetID, primaryState);

      if (!primaryStateRes)
      {
         LogContext(logContext).log(LogTopic_STATESYNC,Log_ERR,
            "Failed to get state for " + nodeTypeStr(false) + " "
            + StringTk::uintToStr(primaryTargetID));

         continue;
      }

      if (primaryState == onlineNeedsResyncState)
      {
         CombinedTargetState secondaryState;
         const bool secondaryStateRes = getStateUnlocked(secondaryTargetID, secondaryState);

         if (!secondaryStateRes)
         {
            LogContext(logContext).log(LogTopic_STATESYNC, Log_ERR,
               "Failed to get state for " + nodeTypeStr(false) + " "
               + StringTk::uintToStr(primaryTargetID) );

            continue;
         }

         if (secondaryState == onlineNeedsResyncState)
         {
            // Both targets are online and need a resync. This means both have possibly corrupt data
            // on them. We can't tell which is better, but we have to decide for one or the other.
            // So we take the current primary because it is probably more up to date.

            LogContext(logContext).log(LogTopic_STATESYNC, Log_WARNING,
               "Both " + nodeTypeStr(false) + "s of a mirror group need a resync. "
               "Setting primary " + nodeTypeStr(false) + " to state good; "
               "Primary " + nodeTypeStr(false) + " ID: " + StringTk::uintToStr(primaryTargetID) );

            goodTargetsList.push_back(primaryTargetID);
         }
      }
   }

   safeLock.unlock(); // U N L O C K

   // Now look at the list of targets collected in the iteration and set their consistency state
   // (on the storage servers and in the local state store) to GOOD.
   if (!goodTargetsList.empty() )
   {
      NodeStore* storageNodes = app->getStorageNodes();
      TargetMapper* targetMapper = app->getTargetMapper();

      for (UInt16ListIter goodTargetsIter = goodTargetsList.begin();
           goodTargetsIter != goodTargetsList.end(); ++goodTargetsIter)
      {
         const uint16_t targetID = *goodTargetsIter;

         FhgfsOpsErr refNodeErr;
         Node* storageNode = storageNodes->referenceNodeByTargetID(targetID, targetMapper,
            &refNodeErr);

         if (refNodeErr != FhgfsOpsErr_SUCCESS)
         {
            LogContext(logContext).log(LogTopic_STATESYNC, Log_ERR,
               "Error referencing storge node for target ID "
               + StringTk::uintToStr(targetID) );

            continue;
         }

         UInt16List targetIDs(1, targetID);
         UInt8List targetStates(1, TargetConsistencyState_GOOD);

         SetTargetConsistencyStatesMsg msg(&targetIDs, &targetStates, false);

         // request/response
         char* respBuf = NULL;
         NetMessage* respMsg = NULL;
         bool commRes = MessagingTk::requestResponse(
            storageNode, &msg, NETMSGTYPE_SetTargetConsistencyStatesResp, &respBuf, &respMsg);
         if (!commRes)
         {
            LogContext(logContext).log(LogTopic_STATESYNC, Log_ERR,
               "Unable to set primary " + nodeTypeStr(false) +
               " to target state good for " + nodeTypeStr(false) +
               " ID " + StringTk::uintToStr(targetID) );

            continue;
         }

         SetTargetConsistencyStatesRespMsg* respMsgCast =
            static_cast<SetTargetConsistencyStatesRespMsg*>(respMsg);
         FhgfsOpsErr result = respMsgCast->getResult();

         if (result != FhgfsOpsErr_SUCCESS)
         {
            LogContext(logContext).log(LogTopic_STATESYNC, Log_ERR,
               "Error setting primary " + nodeTypeStr(false) +
               " to state good for " + nodeTypeStr(false) +
               " ID " + StringTk::uintToStr(targetID) );
         }
         else
         {
            LogContext(logContext).log(LogTopic_STATESYNC, Log_DEBUG,
               "Sucessfully set primary " + nodeTypeStr(false) +
               " to state good for " + nodeTypeStr(false) +
               " ID " + StringTk::uintToStr(targetID) );

            setConsistencyState(targetID, TargetConsistencyState_GOOD);
            res = true;
         }

         SAFE_DELETE(respMsg);
         SAFE_FREE(respBuf);
      }
   }

   return res;
}

void MgmtdTargetStateStore::saveStates()
{
   const char* logContext = "Save target states";

   std::vector<char> buf(Serialization::serialLenTargetStateInfoMap(&statesMap));

   // serialize into buffer
   {
      SafeRWLock lock(&rwlock, SafeRWLock_READ);

      if (!Serialization::serializeTargetStateInfoMap(&buf[0], &statesMap))
      {
         LogContext(logContext).log(Log_ERR, "Failed to serialize target state store.");
         return;
      }

      lock.unlock();
   }

   Path statesPath(*Program::getApp()->getMgmtdPath(), stateStorePath);
   std::string statesTmpPath(statesPath.getPathAsStr() + MGMTDTARGETSTATESTORE_TMPFILE_EXT);

   // write to tmp file
   const int openFlags = O_CREAT | O_TRUNC | O_RDWR;
   int fd = open(statesTmpPath.c_str(), openFlags, 0644);
   if (fd == -1)
   {
      LogContext(logContext).log(Log_ERR, "Could not open temporary file: "
            + statesTmpPath + "; SysErr: " + System::getErrString());
      return;
   }

   size_t writeRes = write(fd, &buf[0], buf.size());
   if (writeRes != buf.size())
   {
      LogContext(logContext).log(Log_ERR, "Writing to file " + statesTmpPath +
            " failed. SysErr: " + System::getErrString());
      close(fd);
      return;
   }

   fsync(fd);
   close(fd);

   // move over
   if (rename(statesTmpPath.c_str(), statesPath.getPathAsStr().c_str()))
   {
      LogContext(logContext).log(Log_ERR, "Renaming file " + statesTmpPath + " to "
            + statesPath.getPathAsStr() + " failed; SysErr: " + System::getErrString());
   }
}

bool MgmtdTargetStateStore::loadStates()
{
   if (stateStorePath.empty())
      throw InvalidConfigException("State store path not set.");

   Path statesPath(*Program::getApp()->getMgmtdPath(), stateStorePath);

   int fd = open(statesPath.getPathAsStr().c_str(), O_RDONLY, 0);
   if (fd == -1)
   {
      LogContext(__func__).log(Log_NOTICE, "Unable to load state store file. SysErr: "
            + System::getErrString());
      return false;
   }

   std::vector<char> buf(MGMTDTARGETSTATESTORE_SERBUF_SIZE);
   int readRes = read(fd, &buf[0], buf.size());
   if (readRes <= 0)
   {
      LogContext(__func__).log(Log_ERR, "Unable to read state store file. SysErr: "
            + System::getErrString());
      return false;
   }

   // deserialize
   const char* infoStart;
   unsigned elemNum;
   unsigned len;

   bool preprocRes = Serialization::deserializeTargetStateInfoMapPreprocess(&buf[0], readRes,
         &infoStart, &elemNum, &len);

   if (!preprocRes)
   {
      LogContext(__func__).log(Log_ERR, "Unable to preprocess state store file.");
      return false;
   }

   TargetStateInfoMap tmpMap;

   bool deserRes = Serialization::deserializeTargetStateInfoMap(elemNum, infoStart, &tmpMap);

   if (!deserRes)
   {
      LogContext(__func__).log(Log_ERR, "Unable to deserialize state store file.");
      return false;
   }

   SafeRWLock lock(&rwlock, SafeRWLock_WRITE);
   tmpMap.swap(statesMap);
   setAllStatesUnlocked(TargetReachabilityState_POFFLINE);
   lock.unlock();

   return true;
}

/**
 * Reads the list of targets which need a resync from the targets_need_resync file.
 * Note: We assume the set is empty here, because this method is only called at startup.
 */
bool MgmtdTargetStateStore::loadTargetsToResyncFromFile()
{
   const char* logContext = "Read resync file";

   if (!targetsToResyncStorePath.length() )
      return false;

   App* app = Program::getApp();

   Path mgmtdPath = *app->getMgmtdPath();
   Path targetsToResyncPath(mgmtdPath, targetsToResyncStorePath);

   StringList targetsToResyncList;

   bool fileExists = StorageTk::pathExists(targetsToResyncPath.getPathAsStr() );
   if (fileExists)
      ICommonConfig::loadStringListFile(targetsToResyncPath.getPathAsStr().c_str(),
         targetsToResyncList);
   else
      return false;

   SafeRWLock safeLock(&targetsToResyncSetLock, SafeRWLock_WRITE); // L O C K

   for (StringListIter targetsIter = targetsToResyncList.begin();
        targetsIter != targetsToResyncList.end(); ++targetsIter)
   {
      uint16_t targetID = StringTk::strToUInt(*targetsIter);

      if (targetID != 0)
         targetsToResync.insert(targetID);
      else
         LogContext(logContext).log(Log_ERR,
            "Failed to deserialize " + nodeTypeStr(false) + " ID " + *targetsIter);
   }

   targetsToResyncSetDirty = true;

   safeLock.unlock(); // U N L O C K

   SafeRWLock safeReadLock(&targetsToResyncSetLock, SafeRWLock_READ); // L O C K

   // Set targets in the list to NEEDS_RESYNC.
   for (TargetIDSetCIter targetIDIter = targetsToResync.begin();
        targetIDIter != targetsToResync.end(); ++targetIDIter)
      addOrUpdate(*targetIDIter, CombinedTargetState(TargetReachabilityState_POFFLINE,
         TargetConsistencyState_NEEDS_RESYNC) );

   safeReadLock.unlock(); // U N L O C K

   return true;
}

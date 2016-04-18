#include <common/net/message/nodes/SetTargetConsistencyStatesMsg.h>
#include <common/net/message/nodes/SetTargetConsistencyStatesRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <components/HeartbeatManager.h>
#include <program/Program.h>

#include "MgmtdTargetStateStore.h"

#define MGMTDTARGETSTATESTORE_TMPFILE_EXT ".tmp"

MgmtdTargetStateStore::MgmtdTargetStateStore() : TargetStateStore(),
   targetsToResyncSetDirty(false)
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

   const char* logContext = "Change target consistency states";

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

            LOG_DEBUG(logContext, Log_DEBUG, "Target coming online, checking if "
               "switchover is necessary. Target ID: " + StringTk::uintToStr(targetID) );

            bool isPrimary;
            uint16_t buddyGroupID = buddyGroups->getBuddyGroupIDUnlocked(targetID, &isPrimary);

            if (isPrimary || !buddyGroupID) // If target is primary or unmapped, do nothing.
               goto bgroups_unlock;

            primaryTargetID = buddyGroups->getPrimaryTargetIDUnlocked(buddyGroupID);

            getStateRes = getStateUnlocked(primaryTargetID, primaryTargetState);
            if (!getStateRes)
            {
               LogContext(logContext).log(Log_ERR, "Failed to get state for mirror target "
                  + StringTk::uintToStr(primaryTargetID) );

               goto bgroups_unlock;
            }
            if (primaryTargetState.reachabilityState == TargetReachabilityState_OFFLINE)
               buddyGroups->switchover(buddyGroupID);

         bgroups_unlock:
            buddyGroups->rwlock.unlock(); // U N L O C K buddyGroups
         }
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
   const char* logContext = "Auto-offline targets";

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
            LogContext(logContext).log(Log_WARNING,
               "No state report received from target for " +
               StringTk::uintToStr(timeSinceLastUpdateMS / 1000) + " seconds. "
               "Setting target to offline. "
               "targetID: " + StringTk::uintToStr(targetID) );

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
            LogContext(logContext).log(Log_WARNING,
               "No state report received from target for "
               + StringTk::uintToStr(timeSinceLastUpdateMS / 1000) + " seconds. "
               "Setting target to probably-offline. " +
               "targetID: " + StringTk::uintToStr(targetID) );

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
            LogContext(logContext).log(Log_ERR, "Tried to switch mirror group members, "
               "but refusing to switch because secondary target state is unknown. "
               "primary targetID: " + StringTk::uintToStr(*targetIDIter) );
            continue;
         }

         if ( (secondaryTargetState.reachabilityState != TargetReachabilityState_ONLINE) ||
              (secondaryTargetState.consistencyState != TargetConsistencyState_GOOD) )
         {
            LogContext(logContext).log(Log_ERR, "Tried to switch mirror group members, "
               "but refusing to switch due to secondary target state. "
               "secondary targetID: " + StringTk::uintToStr(*targetIDIter) + "; "
               "secondary state: " + TargetStateStore::stateToStr(secondaryTargetState) );
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
         LOG_DEBUG(logContext, Log_ERR, "Failed to get state for target "
            + StringTk::uintToStr(primaryTargetID) );

         continue;
      }

      if (primaryState == onlineNeedsResyncState)
      {
         CombinedTargetState secondaryState;
         const bool secondaryStateRes = getState(secondaryTargetID, secondaryState);

         if (!secondaryStateRes)
         {
            LOG_DEBUG(logContext, Log_ERR, "Failed to get state for target "
               + StringTk::uintToStr(primaryTargetID) );

            continue;
         }

         if (secondaryState == onlineNeedsResyncState)
         {
            // Both targets are online and need a resync. This means both have possibly corrupt data
            // on them. We can't tell which is better, but we have to decide for one or the other.
            // So we take the current primary because it is probably more up to date.

            LogContext(logContext).log(Log_WARNING, "Both targets of a mirror group need a "
               "resync. Setting primary target to state good; Primary target ID: "
               + StringTk::uintToStr(primaryTargetID) );

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
            LogContext(logContext).logErr("Error referencing storge node for target ID "
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
            LogContext(logContext).logErr("Unable to set primary target to target state good for "
               "target ID " + StringTk::uintToStr(targetID) );

            continue;
         }

         SetTargetConsistencyStatesRespMsg* respMsgCast =
            static_cast<SetTargetConsistencyStatesRespMsg*>(respMsg);
         FhgfsOpsErr result = respMsgCast->getResult();

         if (result != FhgfsOpsErr_SUCCESS)
         {
            LogContext(logContext).logErr("Error setting primary target to state good for "
               "target ID " + StringTk::uintToStr(targetID) );
         }
         else
         {
            LOG_DEBUG(logContext, Log_DEBUG, "Sucessfully set primary target to state good for "
               "target ID " + StringTk::uintToStr(targetID) );

            setConsistencyState(targetID, TargetConsistencyState_GOOD);
            res = true;
         }

         SAFE_DELETE(respMsg);
         SAFE_FREE(respBuf);
      }
   }

   return res;
}

/**
 * Saves the list of targets which need a resync to the targets_need_resync file.
 */
void MgmtdTargetStateStore::saveTargetsToResyncFile()
{
   const char* logContext = "Save targets to resync file";

   App* app = Program::getApp();

   Path mgmtdPath = *app->getMgmtdPath();
   Path targetsToResyncPath(mgmtdPath, targetsToResyncStorePath);

   std::string targetsToResyncTmpPath =
      targetsToResyncPath.getPathAsStr() + MGMTDTARGETSTATESTORE_TMPFILE_EXT;

   std::string targetsToResyncStr;

   {
      SafeRWLock safeLock(&targetsToResyncSetLock, SafeRWLock_READ); // L O C K

      for (TargetIDSetCIter targetIter = targetsToResync.begin();
           targetIter != targetsToResync.end(); ++targetIter)
         targetsToResyncStr += StringTk::uintToStr(*targetIter) + "\n";

      safeLock.unlock(); // U N L O C K
   }

   // Create tmp file.
   const int openFlags = O_CREAT | O_TRUNC | O_RDWR;
   int fd = open(targetsToResyncTmpPath.c_str(), openFlags, 0644);
   if (fd == -1)
   { // error
      LogContext(logContext).log(Log_ERR, "Could not open temporary file: "
         + targetsToResyncTmpPath + "; SysErr: " + System::getErrString() );

      return;
   }

   ssize_t writeRes = write(fd, targetsToResyncStr.c_str(), targetsToResyncStr.length() );
   if (writeRes != (ssize_t)targetsToResyncStr.length() )
   {
      LogContext(logContext).log(Log_ERR, "Writing to file " + targetsToResyncTmpPath +
         " failed; SysErr: " + System::getErrString() );

      close(fd);
      return;
   }

   fsync(fd);
   close(fd);

   // Rename tmp file to actual file name.
   int renameRes =
      rename(targetsToResyncTmpPath.c_str(), targetsToResyncPath.getPathAsStr().c_str() );
   if (renameRes == -1)
   {
      LogContext(logContext).log(Log_ERR, "Renaming file " + targetsToResyncPath.getPathAsStr()
         + " to " + targetsToResyncPath.getPathAsStr() + " failed; SysErr: "
         + System::getErrString() );
   }

   { // Clear dirty flag.
      SafeRWLock safeLock(&targetsToResyncSetLock, SafeRWLock_READ); // L O C K
      targetsToResyncSetDirty = false;
      safeLock.unlock(); // U N L O C K
   }
}

/**
 * Reads the list of targets which need a resync from the targets_need_resync file.
 * Note: We assume the set is empty here, because this method is only called at startup.
 */
bool MgmtdTargetStateStore::loadTargetsToResyncFromFile() throw (InvalidConfigException)
{
   const char* logContext = "Read targets to resync file";

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

   SafeRWLock safeLock(&targetsToResyncSetLock, SafeRWLock_WRITE); // L O C K

   for (StringListIter targetsIter = targetsToResyncList.begin();
        targetsIter != targetsToResyncList.end(); ++targetsIter)
   {
      uint16_t targetID = StringTk::strToUInt(*targetsIter);

      if (targetID != 0)
         targetsToResync.insert(targetID);
      else
         LogContext(logContext).log(Log_ERR, "Failed to deserialize target ID " + *targetsIter);
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

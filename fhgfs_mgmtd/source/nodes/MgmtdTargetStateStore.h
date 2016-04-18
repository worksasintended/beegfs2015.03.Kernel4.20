#ifndef MGMTDTARGETSTATESTORE_H_
#define MGMTDTARGETSTATESTORE_H_

#include <common/nodes/MirrorBuddyGroupMapper.h>
#include <common/nodes/TargetStateStore.h>

class MgmtdTargetStateStore : public TargetStateStore
{
   public:
      MgmtdTargetStateStore();
      void setConsistencyStatesFromLists(const UInt16List& targetIDs,
         const UInt8List& consistencyStates, bool setOnline);
      FhgfsOpsErr changeConsistencyStatesFromLists(const UInt16List& targetIDs,
         const UInt8List& oldStates, const UInt8List& newStates,
         MirrorBuddyGroupMapper* buddyGroups);

      bool autoOfflineTargets(const unsigned pofflineTimeout, const unsigned offlineTimeout,
         MirrorBuddyGroupMapper* buddyGroups);
      bool resolveDoubleResync();

      void saveTargetsToResyncFile();
      bool loadTargetsToResyncFromFile() throw (InvalidConfigException);

   private:

      typedef std::set<uint16_t> TargetIDSet;
      typedef TargetIDSet::iterator TargetIDSetIter;
      typedef TargetIDSet::const_iterator TargetIDSetCIter;

      // For persistent storage of target ids that need resync.
      RWLock targetsToResyncSetLock;
      TargetIDSet targetsToResync;
      bool targetsToResyncSetDirty;
      std::string targetsToResyncStorePath;

   public:
      // getters & setters

      bool isToResyncSetDirty()
      {
         SafeRWLock safeLock(&targetsToResyncSetLock, SafeRWLock_READ); // L O C K

         bool res = targetsToResyncSetDirty;

         safeLock.unlock(); // U N L O C K

         return res;
      }

      void setTargetsToResyncStorePath(const std::string& storePath)
      {
         this->targetsToResyncStorePath = storePath;
      }


   private:

      void targetsToResyncUpdate(TargetConsistencyState consistencyState, uint16_t targetID)
      {
         SafeRWLock safeLock(&targetsToResyncSetLock, SafeRWLock_WRITE); // L O C K

         bool changed = false;

         if (consistencyState == TargetConsistencyState_NEEDS_RESYNC)
            changed = targetsToResync.insert(targetID).second;
         else
            changed = (targetsToResync.erase(targetID) > 0);

         targetsToResyncSetDirty |= changed;

         safeLock.unlock(); // L O C K
      }

      /**
       * Sets the consistency state of a single target, not changing the reachability state.
       * @returns false if target ID was not found, true otherwise.
       */
      bool setConsistencyState(const uint16_t targetID, const TargetConsistencyState state)
      {
         bool res;

         SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE); // L O C K

         TargetStateInfoMapIter iter = this->statesMap.find(targetID);

         if (iter == this->statesMap.end() )
            res = false;
         else
         {
            iter->second.consistencyState = state;
            res = true;
         }

         safeLock.unlock(); // U N L O C K

         return res;
      }
};

#endif /*MGMTDTARGETSTATESTORE_H_*/

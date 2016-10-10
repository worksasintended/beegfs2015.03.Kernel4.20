#ifndef INTERNODESYNCER_H_
#define INTERNODESYNCER_H_

#include <common/app/log/LogContext.h>
#include <common/components/ComponentInitException.h>
#include <common/threading/PThread.h>
#include <common/Common.h>


class InternodeSyncer : public PThread
{
   public:
      InternodeSyncer() throw(ComponentInitException);
      virtual ~InternodeSyncer();

      static bool downloadAndSyncTargetStates(UInt16List& outTargetIDs,
         UInt8List& outReachabilityStates,UInt8List& outConsistencyStates);
      static bool downloadAndSyncNodes();
      static bool downloadAndSyncTargetMappings();
      static bool downloadAndSyncMirrorBuddyGroups();

      static bool downloadAllExceededQuotaLists();
      static bool downloadExceededQuotaList(QuotaDataType idType, QuotaLimitType exType,
         UIntList* outIDList, FhgfsOpsErr& error);

      static void syncClientSessions(NodeList* clientsList);

      void publishTargetState(uint16_t targetID, TargetConsistencyState targetState);

      bool publishLocalTargetStateChanges(UInt16List& targetIDs, UInt8List& oldStates,
         UInt8List& newStates);

   private:
      LogContext log;

      Mutex forceTargetStatesUpdateMutex;
      bool forceTargetStatesUpdate; // true to force update of target states

      virtual void run();
      void syncLoop();

      void dropIdleConns();
      unsigned dropIdleConnsByStore(NodeStoreServersEx* nodes);

      void updateTargetStatesAndBuddyGroups();
      void publishTargetCapacities();

      bool forceMgmtdPoolsRefresh();

      static void printSyncNodesResults(NodeType nodeType, UInt16List* addedNodes,
         UInt16List* removedNodes);

      bool publishTargetStateChanges(UInt16List& targetIDs, UInt8List& oldStates,
         UInt8List& newStates);

   public:
      // inliners

      void setForceTargetStatesUpdate()
      {
         SafeMutexLock safeLock(&forceTargetStatesUpdateMutex);

         this->forceTargetStatesUpdate = true;

         safeLock.unlock();
      }

      bool getAndResetForceTargetStatesUpdate()
      {
         SafeMutexLock safeLock(&forceTargetStatesUpdateMutex);

         bool retVal = this->forceTargetStatesUpdate;

         this->forceTargetStatesUpdate = false;

         safeLock.unlock();

         return retVal;
      }
};


#endif /* INTERNODESYNCER_H_ */

#ifndef INTERNODESYNCER_H_
#define INTERNODESYNCER_H_

#include <common/app/log/LogContext.h>
#include <common/components/ComponentInitException.h>
#include <common/nodes/MirrorBuddyGroupMapper.h>
#include <common/threading/PThread.h>
#include <common/Common.h>


class InternodeSyncer : public PThread
{
   public:
      InternodeSyncer() throw(ComponentInitException);
      virtual ~InternodeSyncer();

      bool downloadAndSyncNodes(UInt16List& addedStorageNodes, UInt16List& removedStorageNodes,
         UInt16List& addedMetaNodes, UInt16List& removedMetaNodes);
      bool downloadAndSyncTargetMappings();
      bool downloadAndSyncMirrorBuddyGroups();
      bool downloadAndSyncTargetStates();

   private:
      LogContext log;
      Mutex forceNodesAndTargetStatesUpdateMutex;
      bool forceNodesAndTargetStatesUpdate;

      TargetMap originalTargetMap;
      MirrorBuddyGroupMap originalMirrorBuddyGroupMap;

      virtual void run();
      void syncLoop();
      void handleNodeChanges(NodeType nodeType, UInt16List& addedNodes, UInt16List& removedNodes);
      void handleTargetMappingChanges();
      void handleBuddyGroupChanges();

   public:

   private:
      static void printSyncNodesResults(NodeType nodeType, UInt16List* addedNodes,
         UInt16List* removedNodes);

      void setForceNodesAndTargetStatesUpdate()
      {
         SafeMutexLock safeLock(&forceNodesAndTargetStatesUpdateMutex);

         this->forceNodesAndTargetStatesUpdate = true;

         safeLock.unlock();
      }

      bool getAndResetForceNodesAndTargetStatesUpdate()
      {
         SafeMutexLock safeLock(&forceNodesAndTargetStatesUpdateMutex);

         bool retVal = this->forceNodesAndTargetStatesUpdate;

         this->forceNodesAndTargetStatesUpdate = false;

         safeLock.unlock();

         return retVal;
      }
};


#endif /* INTERNODESYNCER_H_ */

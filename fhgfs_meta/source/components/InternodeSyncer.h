#ifndef INTERNODESYNCER_H_
#define INTERNODESYNCER_H_

#include <common/app/log/LogContext.h>
#include <common/components/ComponentInitException.h>
#include <common/net/message/nodes/GetNodeCapacityPoolsMsg.h>
#include <common/threading/PThread.h>
#include <common/Common.h>


class InternodeSyncer : public PThread
{
   public:
      InternodeSyncer() throw(ComponentInitException);
      virtual ~InternodeSyncer();


   private:
      LogContext log;

      Mutex forcePoolsUpdateMutex;
      Mutex forceTargetStatesUpdateMutex;
      bool forcePoolsUpdate; // true to force update of capacity pools
      bool forceTargetStatesUpdate; // true to force update of target states

      virtual void run();
      void syncLoop();

      void updateMetaCapacityPools();
      void updateStorageCapacityPools();
      void updateTargetBuddyCapacityPools();
      bool downloadCapacityPools(CapacityPoolQueryType poolType, UInt16List* outListNormal,
         UInt16List* outListLow, UInt16List* outListEmergency);
      void updateMetaStates();
      void publishNodeState();
      void publishNodeCapacity();

      void dropIdleConns();
      unsigned dropIdleConnsByStore(NodeStoreServersEx* nodes);

      void getStatInfo(int64_t* outSizeTotal, int64_t* outSizeFree, int64_t* outInodesTotal,
         int64_t* outInodesFree);


   public:
      // getters & setters
      void setForcePoolsUpdate()
      {
         SafeMutexLock safeLock(&forcePoolsUpdateMutex);

         this->forcePoolsUpdate = true;

         safeLock.unlock();
      }

      void setForceTargetStatesUpdate()
      {
         SafeMutexLock safeLock(&forceTargetStatesUpdateMutex);

         this->forceTargetStatesUpdate = true;

         safeLock.unlock();
      }

      // inliners

      bool getAndResetForcePoolsUpdate()
      {
         SafeMutexLock safeLock(&forcePoolsUpdateMutex);

         bool retVal = this->forcePoolsUpdate;

         this->forcePoolsUpdate = false;

         safeLock.unlock();

         return retVal;
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

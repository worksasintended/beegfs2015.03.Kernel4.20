#ifndef HEARTBEATMANAGER_H_
#define HEARTBEATMANAGER_H_

#include <app/config/Config.h>
#include <common/app/log/LogContext.h>
#include <common/components/ComponentInitException.h>
#include <common/net/message/NetMessage.h>
#include <common/threading/PThread.h>
#include <common/threading/SafeMutexLock.h>
#include <common/threading/Condition.h>
#include <common/Common.h>
#include <components/DatagramListener.h>
#include <nodes/NodeStoreEx.h>
#include <nodes/NodeStoreClientsEx.h>
#include <nodes/NodeStoreServersEx.h>


class HeartbeatManager : public PThread
{
   public:
      HeartbeatManager(DatagramListener* dgramLis) throw(ComponentInitException);
      virtual ~HeartbeatManager();
      
      void waitForMmgtInit();
      void syncClients(NodeList* clientsList, bool allowRemoteComm);
   
      
   protected:
      LogContext log;
      Config* cfg;
      AbstractDatagramListener* dgramLis;


   private:
      bool mgmtInitDone;
      Mutex mgmtInitDoneMutex;
      Condition mgmtInitDoneCond; // signaled when init is done

      Mutex forceTargetStatesUpdateMutex;
      bool forceTargetStatesUpdate; // true to force update of target states

      NodeStoreServersEx* mgmtNodes;
      NodeStoreServersEx* metaNodes;
      NodeStoreServersEx* storageNodes;
      NodeStoreClientsEx* clientNodes;
      
      bool nodeRegistered; // true if the mgmt host ack'ed our heartbeat
   
      
      void run();
      virtual void requestLoop();
      
      void mgmtInit();
      void signalMgmtInitDone();

      void downloadAndSyncNodes();
      void printSyncResults(NodeType nodeType, UInt16List* addedNodes, UInt16List* removedNodes);
      void downloadAndSyncTargetMappings();
      void downloadAndSyncTargetStatesAndBuddyGroups();
      bool registerNode();
      bool forceMgmtdPoolsRefresh();


   public:
      // getters & setters

      void setForceTargetStatesUpdate()
      {
         SafeMutexLock safeLock(&forceTargetStatesUpdateMutex);

         this->forceTargetStatesUpdate = true;

         safeLock.unlock();
      }

      // inliners

      bool getAndResetForceTargetStatesUpdate()
      {
         SafeMutexLock safeLock(&forceTargetStatesUpdateMutex);

         bool retVal = this->forceTargetStatesUpdate;

         this->forceTargetStatesUpdate = false;

         safeLock.unlock();

         return retVal;
      }


};

#endif /*HEARTBEATMANAGER_H_*/

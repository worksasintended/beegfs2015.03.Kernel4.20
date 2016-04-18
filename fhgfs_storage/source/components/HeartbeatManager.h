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


class HeartbeatManager : public PThread
{
   public:
      HeartbeatManager() throw(ComponentInitException);
      virtual ~HeartbeatManager();

      static bool registerNode(AbstractDatagramListener* dgramLis);
      static bool registerTargetMappings();

      
   protected:
      LogContext log;

   
   private:
      void run();
      virtual void requestLoop();
};

#endif /*HEARTBEATMANAGER_H_*/

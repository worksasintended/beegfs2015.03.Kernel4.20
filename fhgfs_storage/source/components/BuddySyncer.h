#ifndef META_BUDDYSYNCER_H
#define META_BUDDYSYNCER_H

#include <common/app/log/LogContext.h>
#include <common/components/ComponentInitException.h>
#include <common/threading/PThread.h>

class BuddySyncer : public PThread
{
   public:
      BuddySyncer() throw (ComponentInitException);

   private:
      LogContext log;

      virtual void run();
      void syncLoop();
      void requestBuddyTargetStates();
};

#endif /* META_BUDDYSYNCER_H */

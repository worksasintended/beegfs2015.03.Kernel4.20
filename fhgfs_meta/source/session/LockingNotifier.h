#ifndef LOCKINGNOTIFIER_H_
#define LOCKINGNOTIFIER_H_

#include <common/Common.h>
#include <storage/Locking.h>
#include <components/worker/LockEntryNotificationWork.h>
#include <components/worker/LockRangeNotificationWork.h>


/**
 * Creates work packages to notify waiting clients/processes about file lock grants.
 */
class LockingNotifier
{
   public:
      static void notifyWaitersEntryLock(LockEntryNotifyType lockType, std::string parentEntryID,
         std::string entryID, LockEntryNotifyList* notifyList);
      static void notifyWaitersRangeLock(std::string parentEntryID, std::string entryID,
         LockRangeNotifyList* notifyList);


   private:
      LockingNotifier() {}

};

#endif /* LOCKINGNOTIFIER_H_ */

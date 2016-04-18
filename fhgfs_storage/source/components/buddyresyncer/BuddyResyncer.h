#ifndef BUDDYRESYNCER_H_
#define BUDDYRESYNCER_H_

#include <common/Common.h>
#include <components/buddyresyncer/BuddyResyncJob.h>

/**
 * This is not a component that represents a separate thread by itself. Instead, it is the
 * controlling frontend for slave threads, which are started and stopped on request (i.e. it is not
 * automatically started when the app is started).
 *
 * Callers should only use methods in this controlling frontend and not access the slave's methods
 * directly.
 */
class BuddyResyncer
{
   public:
      BuddyResyncer();
      ~BuddyResyncer();

      FhgfsOpsErr startResync(uint16_t targetID, bool& outIsNewJob,
         BuddyResyncJobStats& outJobStats);

   private:
      BuddyResyncJobMap resyncJobMap;
      Mutex resyncJobMapMutex;

   public:
      void stopAllJobs() { } // TODO: implement
      void waitForStopAllJobs() { } // TODO: implement

      BuddyResyncJob* getResyncJob(uint16_t targetID)
      {
         BuddyResyncJob* job = NULL;

         SafeMutexLock mutexLock(&resyncJobMapMutex);

         BuddyResyncJobMapIter iter = resyncJobMap.find(targetID);
         if (iter != resyncJobMap.end())
            job = iter->second;

         mutexLock.unlock();

         return job;
      }

   private:
      BuddyResyncJob* addResyncJob(uint16_t targetID, bool& outIsNew)
      {
         BuddyResyncJob* job = NULL;

         SafeMutexLock mutexLock(&resyncJobMapMutex);

         BuddyResyncJobMapIter iter = resyncJobMap.find(targetID);
         if (iter != resyncJobMap.end())
         {
            job = iter->second;
            outIsNew = false;
         }
         else
         {
            job = new BuddyResyncJob(targetID);
            resyncJobMap[targetID] = job;
            outIsNew = true;
         }

         mutexLock.unlock();

         return job;
      }
};

#endif /* BUDDYRESYNCER_H_ */

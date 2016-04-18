#ifndef BUDDYRESYNCJOB_H_
#define BUDDYRESYNCJOB_H_

#include <common/storage/mirroring/BuddyResyncJobStats.h>
#include <components/buddyresyncer/BuddyResyncerDirSyncSlave.h>
#include <components/buddyresyncer/BuddyResyncerFileSyncSlave.h>
#include <components/buddyresyncer/BuddyResyncerGatherSlave.h>

#define GATHERSLAVEQUEUE_MAXSIZE 5000

class BuddyResyncJob : public PThread
{
   friend class GenericDebugMsgEx;

   public:
      BuddyResyncJob(uint16_t targetID);
      virtual ~BuddyResyncJob();

      virtual void run();

      void abort();
      void getJobStats(BuddyResyncJobStats& outStats);

   private:
      uint16_t targetID;
      Mutex statusMutex;
      BuddyResyncJobStatus status;

      int64_t startTime;
      int64_t endTime;

      ChunkSyncCandidateStore syncCandidates;
      BuddyResyncerGatherSlaveWorkQueue gatherSlavesWorkQueue;

      BuddyResyncerGatherSlaveVec gatherSlaveVec;
      BuddyResyncerFileSyncSlaveVec fileSyncSlaveVec;
      BuddyResyncerDirSyncSlaveVec dirSyncSlaveVec;

      // this thread walks over the top dir structures itself, so we need to track that
      AtomicUInt64 numDirsDiscovered;
      AtomicUInt64 numDirsMatched;

      AtomicInt16 shallAbort; // quasi-boolean

      bool checkTopLevelDir(std::string& path, int64_t lastBuddyCommTimeSecs);
      bool walkDirs(std::string chunksPath, std::string relPath, int level,
         int64_t lastBuddyCommTimeSecs);

      bool startGatherSlaves();
      bool startSyncSlaves();
      void joinGatherSlaves();
      void joinSyncSlaves();

   public:
      uint16_t getTargetID() const
      {
         return targetID;
      }

      BuddyResyncJobStatus getStatus()
      {
         BuddyResyncJobStatus retVal;

         SafeMutexLock mutexLock(&statusMutex);
         retVal = status;
         mutexLock.unlock();

         return retVal;
      }

      bool isRunning()
      {
         bool retVal;

         SafeMutexLock mutexLock(&statusMutex);
         if (status == BuddyResyncJobStatus_RUNNING)
            retVal = true;
         else
            retVal = false;
         mutexLock.unlock();

         return retVal;
      }

   private:
      void setStatus(BuddyResyncJobStatus status)
      {
         SafeMutexLock mutexLock(&statusMutex);
         this->status = status;
         mutexLock.unlock();
      }

      void informBuddy();
};

typedef std::map<uint16_t, BuddyResyncJob*> BuddyResyncJobMap; //mapping: targetID, job
typedef BuddyResyncJobMap::iterator BuddyResyncJobMapIter;


#endif /* BUDDYRESYNCJOB_H_ */

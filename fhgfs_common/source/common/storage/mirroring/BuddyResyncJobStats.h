#ifndef BUDDYRESYNCJOBSTATS_H_
#define BUDDYRESYNCJOBSTATS_H_

#include <common/Common.h>

enum BuddyResyncJobStatus
{
   BuddyResyncJobStatus_NOTSTARTED = 0,
   BuddyResyncJobStatus_RUNNING,
   BuddyResyncJobStatus_SUCCESS,
   BuddyResyncJobStatus_INTERRUPTED,
   BuddyResyncJobStatus_FAILURE,
   BuddyResyncJobStatus_ERRORS
};

class BuddyResyncJobStats
{
   public:
      BuddyResyncJobStats(BuddyResyncJobStatus status, int64_t startTime, int64_t endTime,
         uint64_t discoveredFiles, uint64_t discoveredDirs, uint64_t matchedFiles,
         uint64_t matchedDirs, uint64_t syncedFiles, uint64_t syncedDirs, uint64_t errorFiles = 0,
         uint64_t errorDirs = 0)
      {
         this->status = status;
         this->startTime = startTime;
         this->endTime = endTime;
         this->discoveredFiles = discoveredFiles;
         this->discoveredDirs = discoveredDirs;
         this->matchedFiles = matchedFiles;
         this->matchedDirs = matchedDirs;
         this->syncedFiles = syncedFiles;
         this->syncedDirs = syncedDirs;
         this->errorFiles = errorFiles;
         this->errorDirs = errorDirs;
      }

      BuddyResyncJobStats(BuddyResyncJobStats& jobStats) :
         status(jobStats.status), startTime(jobStats.startTime), endTime(jobStats.endTime),
         discoveredFiles(jobStats.discoveredFiles), discoveredDirs(jobStats.discoveredDirs),
         matchedFiles(jobStats.matchedFiles), syncedFiles(jobStats.syncedFiles),
         syncedDirs(jobStats.syncedDirs), errorFiles(jobStats.errorFiles),
         errorDirs(jobStats.errorDirs)
      {
      }
      
      BuddyResyncJobStats()
      {
         this->status = BuddyResyncJobStatus_NOTSTARTED;
         this->startTime = 0;
         this->endTime = 0;
         this->discoveredFiles = 0;
         this->discoveredDirs = 0;
         this->matchedFiles = 0;
         this->matchedDirs = 0;
         this->syncedFiles = 0;
         this->syncedDirs = 0;
         this->errorFiles = 0;
         this->errorDirs = 0;
      }

      size_t serialize(char* outBuf);
      bool deserialize(const char* buf, size_t bufLen, unsigned* outLen);
      unsigned serialLen(void);

   private:
      BuddyResyncJobStatus status;
      int64_t startTime;
      int64_t endTime;
      uint64_t discoveredFiles;
      uint64_t discoveredDirs;
      uint64_t matchedFiles;
      uint64_t matchedDirs;
      uint64_t syncedFiles;
      uint64_t syncedDirs;
      uint64_t errorFiles;
      uint64_t errorDirs;

   public:
      BuddyResyncJobStatus getStatus() const
      {
         return status;
      }

      int64_t getStartTime() const
      {
         return startTime;
      }

      int64_t getEndTime() const
      {
         return endTime;
      }

      uint64_t getDiscoveredDirs() const
      {
         return discoveredDirs;
      }
      
      uint64_t getDiscoveredFiles() const
      {
         return discoveredFiles;
      }
      
      uint64_t getMatchedDirs() const
      {
         return matchedDirs;
      }
      
      uint64_t getMatchedFiles() const
      {
         return matchedFiles;
      }
      
      uint64_t getSyncedDirs() const
      {
         return syncedDirs;
      }
      
      uint64_t getSyncedFiles() const
      {
         return syncedFiles;
      }

      uint64_t getErrorDirs() const
      {
         return errorDirs;
      }

      uint64_t getErrorFiles() const
      {
         return errorFiles;
      }

      void update(BuddyResyncJobStatus status, int64_t startTime, int64_t endTime,
         uint64_t discoveredFiles, uint64_t matchedFiles, uint64_t discoveredDirs,
         uint64_t matchedDirs, uint64_t syncedFiles, uint64_t syncedDirs, uint64_t errorFiles,
         uint64_t errorDirs)
      {
         this->status = status;
         this->startTime = startTime;
         this->endTime = endTime;
         this->discoveredFiles = discoveredFiles;
         this->discoveredDirs = discoveredDirs;
         this->matchedFiles = matchedFiles;
         this->matchedDirs = matchedDirs;
         this->syncedFiles = syncedFiles;
         this->syncedDirs = syncedDirs;
         this->errorFiles = errorFiles;
         this->errorDirs = errorDirs;
      }

      void update(BuddyResyncJobStatus status, int64_t endTime, uint64_t discoveredFiles,
         uint64_t matchedFiles, uint64_t discoveredDirs, uint64_t matchedDirs, uint64_t syncedFiles,
         uint64_t syncedDirs, uint64_t errorFiles, uint64_t errorDirs)
      {
         int64_t startTime = this->startTime;
         update(status, startTime, endTime, discoveredFiles, matchedFiles, discoveredDirs,
            matchedDirs, syncedFiles, syncedDirs, errorFiles, errorDirs);
      }

      void update(BuddyResyncJobStatus status, uint64_t discoveredFiles, uint64_t matchedFiles,
         uint64_t discoveredDirs, uint64_t matchedDirs, uint64_t syncedFiles, uint64_t syncedDirs,
         uint64_t errorFiles, uint64_t errorDirs)
      {
         int64_t startTime = this->startTime;
         int64_t endTime = this->endTime;
         update(status, startTime, endTime, discoveredFiles, matchedFiles, discoveredDirs,
            matchedDirs, syncedFiles, syncedDirs, errorFiles, errorDirs);
      }

      void update(BuddyResyncJobStats& stats)
      {
         this->status = stats.status;
         this->startTime = stats.startTime;
         this->endTime = stats.endTime;
         this->discoveredFiles = stats.discoveredFiles;
         this->discoveredDirs = stats.discoveredDirs;
         this->matchedFiles = stats.matchedFiles;
         this->matchedDirs = stats.matchedDirs;
         this->syncedFiles = stats.syncedFiles;
         this->syncedDirs = stats.syncedDirs;
         this->errorFiles = stats.errorFiles;
         this->errorDirs = stats.errorDirs;
      }
};

#endif /* BUDDYRESYNCJOBSTATS_H_ */

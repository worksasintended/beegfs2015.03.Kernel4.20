#ifndef CONFIG_H_
#define CONFIG_H_

#include <common/app/config/AbstractConfig.h>

/**
 * Find out whether this distro hash sync_file_range() support (added in linux-2.6.17, glibc 2.6).
 * Note: Problem is that RHEL 5 defines SYNC_FILE_RANGE_WRITE, but uses glibc 2.5 which has no
 * sync_file_range support, so linker complains about undefined reference.
 */
#ifdef __GNUC__
   #include <features.h>
   #include <fcntl.h>
   #if __GLIBC_PREREQ(2, 6) && defined(SYNC_FILE_RANGE_WRITE)
      #define CONFIG_DISTRO_HAS_SYNC_FILE_RANGE
   #endif
#endif


class Config : public AbstractConfig
{
   public:
      Config(int argc, char** argv) throw(InvalidConfigException);
   
   private:

      // configurables

      std::string connInterfacesFile; // implicitly generates connInterfacesList
      std::string connInterfacesList; // comma-separated list

      bool        debugRunComponentThreads;
      bool        debugRunStartupTests;
      bool        debugRunComponentTests;
      bool        debugRunIntegrationTests;

      std::string storeStorageDirectory;
      bool        storeAllowFirstRunInit;
      bool        storeInitHashDirs; // true to force hash dirs creation on startup

      unsigned    tuneNumStreamListeners;
      unsigned    tuneNumWorkers;
      unsigned    tuneWorkerBufSize;
      unsigned    tuneProcessFDLimit; // 0 means "don't touch limit"
      bool        tuneWorkerNumaAffinity;
      bool        tuneListenerNumaAffinity;
      int         tuneBindToNumaZone; // bind all threads to this zone, -1 means no binding
      int         tuneListenerPrioShift;
      ssize_t     tuneFileReadSize;
      ssize_t     tuneFileReadAheadTriggerSize; // after how much seq read to start read-ahead
      ssize_t     tuneFileReadAheadSize; // read-ahead with posix_fadvise(..., POSIX_FADV_WILLNEED)
      ssize_t     tuneFileWriteSize;
      ssize_t     tuneFileWriteSyncSize; // after how many of per session data to sync_file_range()
      bool        tuneUsePerUserMsgQueues; // true to use UserWorkContainer for MultiWorkQueue
      unsigned    tuneDirCacheLimit;
      bool        tuneEarlyStat;          // stat the chunk file before closing it
      unsigned    tuneNumResyncGatherSlaves;
      unsigned    tuneNumResyncSlaves;
      bool        tuneUseAggressiveStreamPoll; // true to not sleep on epoll in streamlisv2
      bool        tuneUsePerTargetWorkers; // true to have tuneNumWorkers separate for each target

      bool        quotaEnableEnforcement;
      bool        quotaDisableZfsSupport;

      int64_t     sysResyncSafetyThresholdMins; // minutes to add to last buddy comm timestamp
      unsigned    sysTargetOfflineTimeoutSecs;

      bool        runDaemonized;

      std::string pidFile;


      // internals

      virtual void loadDefaults(bool addDashes);
      virtual void applyConfigMap(bool enableException, bool addDashes)
         throw(InvalidConfigException);
      virtual void initImplicitVals() throw(InvalidConfigException);
      std::string createDefaultCfgFilename();


      
   public:
      // getters & setters

      std::string getConnInterfacesList() const
      {
         return connInterfacesList;
      }

      bool getDebugRunComponentThreads() const
      {
         return debugRunComponentThreads;
      }

      bool getDebugRunStartupTests() const
      {
         return debugRunStartupTests;
      }

      bool getDebugRunComponentTests() const
      {
         return debugRunComponentTests;
      }

      bool getDebugRunIntegrationTests() const
      {
         return debugRunIntegrationTests;
      }
      
      std::string getStoreStorageDirectoryV2() const
      {
         return storeStorageDirectory;
      }
      
      bool getStoreAllowFirstRunInit() const
      {
         return storeAllowFirstRunInit;
      }

      bool getStoreInitHashDirs() const
      {
         return storeInitHashDirs;
      }

      unsigned getTuneNumStreamListeners() const
      {
         return tuneNumStreamListeners;
      }

      unsigned getTuneNumWorkers() const
      {
         return tuneNumWorkers;
      }

      unsigned getTuneWorkerBufSize() const
      {
         return tuneWorkerBufSize;
      }

      unsigned getTuneProcessFDLimit() const
      {
         return tuneProcessFDLimit;
      }

      bool getTuneWorkerNumaAffinity() const
      {
         return tuneWorkerNumaAffinity;
      }

      bool getTuneListenerNumaAffinity() const
      {
         return tuneListenerNumaAffinity;
      }

      int getTuneBindToNumaZone() const
      {
         return tuneBindToNumaZone;
      }

      int getTuneListenerPrioShift() const
      {
         return tuneListenerPrioShift;
      }

      ssize_t getTuneFileReadSize() const
      {
         return tuneFileReadSize;
      }

      ssize_t getTuneFileReadAheadTriggerSize() const
      {
         return tuneFileReadAheadTriggerSize;
      }

      ssize_t getTuneFileReadAheadSize() const
      {
         return tuneFileReadAheadSize;
      }

      ssize_t getTuneFileWriteSize() const
      {
         return tuneFileWriteSize;
      }

      ssize_t getTuneFileWriteSyncSize() const
      {
         return this->tuneFileWriteSyncSize;
      }

      bool getTuneUsePerUserMsgQueues() const
      {
         return tuneUsePerUserMsgQueues;
      }

      bool getRunDaemonized() const
      {
         return runDaemonized;
      }

      std::string getPIDFile() const
      {
         return pidFile;
      }

      unsigned getTuneDirCacheLimit() const
      {
         return tuneDirCacheLimit;
      }

      bool getTuneEarlyStat() const
      {
         return this->tuneEarlyStat;
      }

      bool getQuotaEnableEnforcement() const
      {
         return quotaEnableEnforcement;
      }

      void setQuotaEnableEnforcement(bool doQuotaEnforcement)
      {
         quotaEnableEnforcement = doQuotaEnforcement;
      }

      bool getQuotaDisableZfsSupport() const
      {
         return quotaDisableZfsSupport;
      }

      unsigned getTuneNumResyncGatherSlaves() const
      {
         return tuneNumResyncGatherSlaves;
      }

      unsigned getTuneNumResyncSlaves() const
      {
         return tuneNumResyncSlaves;
      }

      bool getTuneUseAggressiveStreamPoll() const
      {
         return tuneUseAggressiveStreamPoll;
      }

      bool getTuneUsePerTargetWorkers() const
      {
         return tuneUsePerTargetWorkers;
      }

      int64_t getSysResyncSafetyThresholdMins() const
      {
         return sysResyncSafetyThresholdMins;
      }

      unsigned getSysTargetOfflineTimeoutSecs() const
      {
         return sysTargetOfflineTimeoutSecs;
      }

};

#endif /*CONFIG_H_*/

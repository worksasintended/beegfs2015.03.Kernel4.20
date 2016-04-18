#ifndef STORAGEBENCHSLAVE_H_
#define STORAGEBENCHSLAVE_H_

#include <common/app/log/LogContext.h>
#include <common/benchmark/StorageBench.h>
#include <common/threading/Condition.h>
#include <common/threading/PThread.h>
#include <common/toolkit/Pipe.h>
#include <common/toolkit/TimeFine.h>
#include <common/Common.h>


// struct for the informations about a thread which simulates a client
struct StorageBenchThreadData
{
   uint16_t targetID;
   int targetThreadID;
   int64_t engagedSize; // amount of data which was submitted for write/read
   int fileDescriptor;
   int64_t neededTime;
};


// map for the informations about a thread; key: virtual threadID, value: information about thread
typedef std::map<int, StorageBenchThreadData> StorageBenchThreadDataMap;
typedef StorageBenchThreadDataMap::iterator StorageBenchThreadDataMapIter;
typedef StorageBenchThreadDataMap::const_iterator StorageBenchThreadDataMapCIter;
typedef StorageBenchThreadDataMap::value_type StorageBenchThreadDataMapVal;



class StorageBenchSlave : public PThread
{
   public:
      StorageBenchSlave() : PThread("StorageBenchSlave")
      {
         this->threadCommunication = new Pipe(false, true);

         log.setContext("Storage benchmark");

         this->benchType = StorageBenchType_NONE;
         this->blocksize = 1;    //useless defaults
         this->size = 1;         //useless defaults
         this->numThreads = 1;      //useless defaults
         this->numThreadsDone = 0;

         this->status = StorageBenchStatus_UNINITIALIZED;
         this->lastRunErrorCode = STORAGEBENCH_ERROR_NO_ERROR;

         this->transferData = NULL;
         this->targetIDs = NULL;
      }

      virtual ~StorageBenchSlave()
      {
         SAFE_DELETE(this->threadCommunication);
         SAFE_DELETE(this->targetIDs);
         SAFE_FREE(this->transferData);
      }

      int initAndStartStorageBench(UInt16List* targetIDs, int64_t blocksize, int64_t size,
         int threads, StorageBenchType type);

      int cleanup(UInt16List* targetIDs);
      int stopBenchmark();
      StorageBenchStatus getStatusWithResults(UInt16List* targetIDs,
         StorageBenchResultsMap* outResults);
      void shutdownBenchmark();
      void waitForShutdownBenchmark();

   protected:

   private:
      Pipe* threadCommunication;
      Mutex statusMutex;
      Condition statusChangeCond;

      LogContext log;
      int lastRunErrorCode; // STORAGEBENCH_ERROR_...

      StorageBenchStatus status;
      StorageBenchType benchType;
      int64_t blocksize;
      int64_t size;
      int numThreads;
      unsigned int numThreadsDone;

      UInt16List* targetIDs;
      StorageBenchThreadDataMap threadData;
      char* transferData;

      TimeFine startTime;


      virtual void run();

      int initStorageBench(UInt16List* targetIDs, int64_t blocksize, int64_t size,
         int threads, StorageBenchType type);
      bool initTransferData(void);
      void initThreadData();
      void freeTransferData();

      bool checkReadData(void);
      bool createBenchmarkFolder(void);
      bool openFiles(void);
      bool closeFiles(void);

      int64_t getNextPackageSize(int threadID);
      int64_t getResult(uint16_t targetId);
      void getResults(UInt16List* targetIds, StorageBenchResultsMap* results);
      void getAllResults(StorageBenchResultsMap* results);

      void setStatus(StorageBenchStatus newStatus)
      {
         SafeMutexLock safeLock(&statusMutex);

         this->status = newStatus;
         this->statusChangeCond.broadcast();

         safeLock.unlock();
      }

   public:
      //public inliners
      int getLastRunErrorCode()
      {
         return this->lastRunErrorCode;
      }

      StorageBenchStatus getStatus()
      {
         StorageBenchStatus retVal;

         SafeMutexLock safeLock(&statusMutex);

         retVal = this->status;

         safeLock.unlock();

         return retVal;
      }

      StorageBenchType getType()
      {
         return this->benchType;
      }

      UInt16List* getTargetIDs()
      {
         return this->targetIDs;
      }
};

#endif /* STORAGEBENCHSLAVE_H_ */

#ifndef CHUNKFETCHERSLAVE_H_
#define CHUNKFETCHERSLAVE_H_

#include <common/app/log/LogContext.h>
#include <common/components/ComponentInitException.h>
#include <common/fsck/FsckChunk.h>
#include <common/threading/PThread.h>

#include <ftw.h>

class ChunkFetcher; //forward decl.

/**
 * This component runs over all chunks of one target and gathers information suitable for fsck
 *
 * This component is not auto-started when the app starts. It is started and stopped by the
 * ChunkFetcher.
 */
class ChunkFetcherSlave : public PThread
{
   friend class ChunkFetcher; // (to grant access to internal mutex)

   public:
   ChunkFetcherSlave(uint16_t targetID) throw(ComponentInitException);
   virtual ~ChunkFetcherSlave();

   private:
      LogContext log;

      ChunkFetcher* parent;

      Mutex statusMutex; // protects isRunning
      Condition isRunningChangeCond;

      bool isRunning; // true if an instance of this component is currently running

      uint16_t targetID;

      static Mutex staticMapsMutex;
      static std::map<std::string, uint16_t> currentTargetIDs;
      static std::map<std::string, uint16_t> currentBuddyGroupIDs;
      static std::map<std::string, size_t> basePathLenghts;

      virtual void run();

   public:
      // getters & setters
      bool getIsRunning(bool isRunning)
      {
         SafeMutexLock safeLock(&statusMutex);

         bool retVal = this->isRunning;

         safeLock.unlock();

         return retVal;
      }

   private:
      void walkAllChunks();

      static int handleDiscoveredChunk(const char* path, const struct stat* statBuf, int ftwEntryType,
         struct FTW* ftwBuf);

      // getters & setters

      void setIsRunning(bool isRunning)
      {
         SafeMutexLock safeLock(&statusMutex);

         this->isRunning = isRunning;
         isRunningChangeCond.broadcast();

         safeLock.unlock();
      }
};

#endif /* CHUNKFETCHERSLAVE_H_ */

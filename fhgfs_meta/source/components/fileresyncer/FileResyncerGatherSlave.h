#ifndef FILERESYNCERGATHERSLAVE_H_
#define FILERESYNCERGATHERSLAVE_H_

#include <common/app/log/LogContext.h>
#include <common/components/ComponentInitException.h>
#include <common/threading/PThread.h>

#include <ftw.h>


/**
 * This component runs over all metadata to identify files that might need resync.
 *
 * This component is not auto-started when the app starts. It is started and stopped by the
 * its control frontend (FileResyncer).
 */
class FileResyncerGatherSlave : public PThread
{
   friend class FileResyncer; // (to grant access to internal mutex)

   public:
      FileResyncerGatherSlave() throw(ComponentInitException);
      virtual ~FileResyncerGatherSlave();


   private:
      LogContext log;

      Mutex statusMutex; // protects numRefreshed + isRunning
      Condition isRunningChangeCond;

      UInt16Set resyncMirrorTargets; // IDs of the targets to be resynced (empty means "all")

      AtomicUInt64 numEntriesDiscovered;  // number of entries discovered so far
      AtomicUInt64 numFilesMatched;  // number of files with matching mirrorTargets so far

      bool isRunning; // true if an instance of this component is currently running

      static FileResyncerGatherSlave* thisStatic; // nftw() callback needs access to this thread


      void walkAllMetadata();


      virtual void run();

      static int handleDiscoveredDentriesSubentry(const char* path, const struct stat* statBuf,
         int ftwEntryType, struct FTW* ftwBuf);
      static int handleDiscoveredInodesSubentry(const char* path, const struct stat* statBuf,
         int ftwEntryType, struct FTW* ftwBuf);
      static bool testTargetMatch(UInt16Set* resyncMirrorTargets, StripePattern* pattern);

      static std::string getFtwEntryTypeStr(int ftwEntryType);


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
      // getters & setters

      void setIsRunning(bool isRunning)
      {
         SafeMutexLock safeLock(&statusMutex);

         this->isRunning = isRunning;
         isRunningChangeCond.broadcast();

         safeLock.unlock();
      }

};


#endif /* FILERESYNCERGATHERSLAVE_H_ */

#ifndef CHUNKSYNCCANDIDATESTORE_H
#define CHUNKSYNCCANDIDATESTORE_H

#include <common/threading/SafeMutexLock.h>
#include <common/toolkit/ListTk.h>
#include <components/buddyresyncer/ChunkSyncCandidateDir.h>
#include <components/buddyresyncer/ChunkSyncCandidateFile.h>

#define CHUNKSYNCCANDIDATESTORE_MAX_QUEUE_SIZE 50000

class ChunkSyncCandidateStore
{
   public:
       ChunkSyncCandidateStore()
       {
          numQueuedFiles = 0;
          numQueuedDirs = 0;
       };

   private:
      ChunkSyncCandidateFileList candidatesFile;
      Mutex candidatesFileMutex;
      Condition filesAddedCond;
      Condition filesFetchedCond;

      ChunkSyncCandidateDirList candidatesDir;
      Mutex candidatesDirMutex;
      Condition dirsAddedCond;
      Condition dirsFetchedCond;

      // mainly used to avoid constant calling of size() method of lists
      unsigned numQueuedFiles;
      unsigned numQueuedDirs;

   public:
      void add(ChunkSyncCandidateFile& entry, PThread* caller)
      {
         const unsigned waitTimeoutMS = 1000;

         SafeMutexLock mutexLock(&candidatesFileMutex);

         // wait if list is too big
         while (numQueuedFiles > CHUNKSYNCCANDIDATESTORE_MAX_QUEUE_SIZE)
         {
            if ( (caller) && (unlikely(caller->getSelfTerminate()) ) )
               break; // ignore limit if selfTerminate was set to avoid hanging on shutdown

            filesFetchedCond.timedwait(&candidatesFileMutex, waitTimeoutMS);
         }

         this->candidatesFile.push_back(entry);

         numQueuedFiles++;

         filesAddedCond.signal();

         mutexLock.unlock();
      }

      void fetch(ChunkSyncCandidateFile& outCandidate, PThread* caller)
      {
         unsigned waitTimeMS = 3000;

         SafeMutexLock mutexLock(&candidatesFileMutex);

         while (candidatesFile.empty())
         {
            if((caller) && (unlikely(caller->getSelfTerminate())))
            {
               outCandidate.setRelativePath("");
               outCandidate.setTargetID(0);
               goto unlock_and_return;
            }

            filesAddedCond.timedwait(&candidatesFileMutex, waitTimeMS);
         }

         outCandidate = candidatesFile.front();
         candidatesFile.pop_front();
         numQueuedFiles--;
         filesFetchedCond.signal();

         unlock_and_return:
         mutexLock.unlock();
      }

      void add(ChunkSyncCandidateDir& entry, PThread* caller)
      {
         const unsigned waitTimeoutMS = 3000;

         SafeMutexLock mutexLock(&candidatesDirMutex);

         // wait if list is too big
         while (numQueuedDirs > CHUNKSYNCCANDIDATESTORE_MAX_QUEUE_SIZE)
         {
            if ( (caller) && (unlikely(caller->getSelfTerminate()) ) )
               break; // ignore limit if selfTerminate was set to avoid hanging on shutdown

            dirsFetchedCond.timedwait(&candidatesDirMutex, waitTimeoutMS);
         }

         this->candidatesDir.push_back(entry);
         numQueuedDirs++;

         dirsAddedCond.signal();

         mutexLock.unlock();
      }

      void fetch(ChunkSyncCandidateDir& outCandidate, PThread* caller)
      {
         unsigned waitTimeMS = 3000;

         SafeMutexLock mutexLock(&candidatesDirMutex);

         while (candidatesDir.empty())
         {
            if((caller) && (unlikely(caller->getSelfTerminate())))
            {
               outCandidate.setRelativePath("");
               outCandidate.setTargetID(0);
               goto unlock_and_return;
            }

            dirsAddedCond.timedwait(&candidatesDirMutex, waitTimeMS);
         }

         outCandidate = candidatesDir.front();
         candidatesDir.pop_front();
         numQueuedDirs--;
         dirsFetchedCond.signal();

         unlock_and_return:
         mutexLock.unlock();
      }

      bool isFilesEmpty()
      {
         bool retVal;

         SafeMutexLock mutexLock(&candidatesFileMutex);
         retVal = candidatesFile.empty();
         mutexLock.unlock();

         return retVal;
      }

      bool isDirsEmpty()
      {
         bool retVal;

         SafeMutexLock mutexLock(&candidatesDirMutex);
         retVal = candidatesDir.empty();
         mutexLock.unlock();

         return retVal;
      }

      void waitForFiles(unsigned timeoutMS)
      {
         SafeMutexLock mutexLock(&candidatesFileMutex);

         if (candidatesFile.empty())
         {
            if (timeoutMS == 0)
               filesAddedCond.wait(&candidatesFileMutex);
            else
               filesAddedCond.timedwait(&candidatesFileMutex, timeoutMS);
         }

         mutexLock.unlock();
      }

      void waitForDirs(unsigned timeoutMS)
      {
         SafeMutexLock mutexLock(&candidatesDirMutex);

         if (candidatesDir.empty())
         {
            if (timeoutMS == 0)
               dirsAddedCond.wait(&candidatesDirMutex);
            else
               dirsAddedCond.timedwait(&candidatesDirMutex, timeoutMS);
         }

         mutexLock.unlock();
      }

      size_t getNumFiles()
      {
         SafeMutexLock mutexLock(&candidatesFileMutex);

         size_t retVal = candidatesFile.size();

         mutexLock.unlock();

         return retVal;
      }

      size_t getNumDirs()
      {
         SafeMutexLock mutexLock(&candidatesDirMutex);

         size_t retVal = candidatesDir.size();

         mutexLock.unlock();

         return retVal;
      }

      void clear()
      {
         SafeMutexLock dirMutexLock(&candidatesDirMutex);
         candidatesDir.clear();
         numQueuedDirs = 0;
         dirMutexLock.unlock();

         SafeMutexLock fileMutexLock(&candidatesFileMutex);
         candidatesFile.clear();
         numQueuedFiles = 0;
         fileMutexLock.unlock();
      }
};


#endif /* CHUNKSYNCCANDIDATESTORE_H */

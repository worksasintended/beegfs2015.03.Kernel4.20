#ifndef FILERESYNCER_H_
#define FILERESYNCER_H_

#include <common/Common.h>
#include "FileResyncerGatherSlave.h"


/**
 * This is not a component that represents a separate thread by itself. Instead, it is the
 * controlling frontend for a slave thread, which is started and stopped on request (i.e. it is not
 * automatically started when the app is started).
 *
 * Callers should only use methods in this controlling frontend and not access the slave's methods
 * directly.
 *
 * The slave thread will run over all local metadata to identify files that need resynchonization.
 */
class FileResyncer
{
   public:
      FileResyncer();
      ~FileResyncer();

      bool startResync();
      void stopResync();
      void waitForStopResync();


   private:
      LogContext log;
      FileResyncerGatherSlave gatherSlave;



   public:
      // getters & setters
      bool getIsRunning()
      {
         SafeMutexLock safeLock(&gatherSlave.statusMutex);

         bool retVal = gatherSlave.isRunning;

         safeLock.unlock();

         return retVal;
      }

      void getStats(uint64_t* numEntriesDiscovered, uint64_t* numFilesMatched)
      {
         SafeMutexLock safeLock(&gatherSlave.statusMutex);

         *numEntriesDiscovered = gatherSlave.numEntriesDiscovered.read();
         *numFilesMatched = gatherSlave.numFilesMatched.read();

         safeLock.unlock();
      }


};


#endif /* FILERESYNCER_H_ */

#include <common/toolkit/Time.h>
#include <program/Program.h>
#include <app/App.h>
#include <app/config/Config.h>
#include "FileResyncer.h"


FileResyncer::FileResyncer()
{
   log.setContext("FileResync");
}

FileResyncer::~FileResyncer()
{
   // nothing to be done here
}

/**
 * Start refresher slave if it's not running already.
 *
 * @return true if successfully started or already running, false if startup problem occurred.
 */
bool FileResyncer::startResync()
{
   const char* logContext = "FileResync (start)";
   bool retVal = true; // false if error occurred

   SafeMutexLock safeLock(&gatherSlave.statusMutex);

   if(!gatherSlave.isRunning)
   { // slave thread not running yet => start it
      gatherSlave.resetSelfTerminate();

      /* reset stats counters here (instead of slave thread) to avoid showing old values if caller
         queries status before slave thread resets them */
      gatherSlave.numEntriesDiscovered.setZero();
      gatherSlave.numFilesMatched.setZero();

      try
      {
         gatherSlave.start();

         gatherSlave.isRunning = true;
      }
      catch(PThreadCreateException& e)
      {
         LogContext(logContext).logErr(std::string("Unable to start thread: ") + e.what() );
         retVal = false;
      }
   }

   safeLock.unlock();

   return retVal;
}

void FileResyncer::stopResync()
{
   SafeMutexLock safeLock(&gatherSlave.statusMutex);

   if(gatherSlave.isRunning)
      gatherSlave.selfTerminate();

   safeLock.unlock();
}

void FileResyncer::waitForStopResync()
{
   SafeMutexLock safeLock(&gatherSlave.statusMutex);

   while(gatherSlave.isRunning)
      gatherSlave.isRunningChangeCond.wait(&gatherSlave.statusMutex);

   safeLock.unlock();
}

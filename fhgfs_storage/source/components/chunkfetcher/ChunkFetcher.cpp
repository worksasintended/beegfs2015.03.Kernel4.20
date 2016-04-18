#include "ChunkFetcher.h"

#include <program/Program.h>

#include <common/Common.h>

ChunkFetcher::ChunkFetcher()
{
   log.setContext("ChunkFetcher");

   // for each targetID, put one fetcher thread into list
   UInt16List targetIDList;
   Program::getApp()->getStorageTargets()->getAllTargetIDs(&targetIDList);

   for (UInt16ListIter iter = targetIDList.begin(); iter != targetIDList.end(); iter++)
   {
      ChunkFetcherSlave slave(*iter);
      this->slaves.push_back(slave);
   }
}

ChunkFetcher::~ChunkFetcher()
{
}

/**
 * Start fetcher slaves if they are not running already.
 *
 * @return true if successfully started or already running, false if startup problem occurred.
 */
bool ChunkFetcher::startFetching()
{
   const char* logContext = "ChunkFetcher (start)";
   bool retVal = true; // false if error occurred

   for(ChunkFetcherSlaveListIter iter = slaves.begin(); iter != slaves.end(); iter++)
   {
      SafeMutexLock safeLock(&(iter->statusMutex));

      if(!iter->isRunning)
      {
         // slave thread not running yet => start it
         iter->resetSelfTerminate();

         try
         {
            iter->start();

            iter->isRunning = true;
         }
         catch (PThreadCreateException& e)
         {
            LogContext(logContext).logErr(std::string("Unable to start thread: ") + e.what());
            retVal = false;
         }
      }

      safeLock.unlock();
   }

   return retVal;
}

void ChunkFetcher::stopFetching()
{
   for(ChunkFetcherSlaveListIter iter = slaves.begin(); iter != slaves.end(); iter++)
   {
      SafeMutexLock safeLock(&(iter->statusMutex));

      if(iter->isRunning)
      {
         iter->selfTerminate();
      }

      safeLock.unlock();
   }
}

void ChunkFetcher::waitForStopFetching()
{
   for(ChunkFetcherSlaveListIter iter = slaves.begin(); iter != slaves.end(); iter++)
   {
      SafeMutexLock safeLock(&(iter->statusMutex));

      while (iter->isRunning)
      {
         iter->isRunningChangeCond.wait(&(iter->statusMutex));
      }

      safeLock.unlock();
   }
}

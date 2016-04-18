#include <program/Program.h>

#include "BuddyResyncer.h"

BuddyResyncer::BuddyResyncer()
{
}

BuddyResyncer::~BuddyResyncer()
{
   // delete remaining jobs
   for (BuddyResyncJobMapIter iter = resyncJobMap.begin(); iter != resyncJobMap.end(); iter++)
   {
      BuddyResyncJob* job = iter->second;
      if( job->isRunning() )
         job->abort();

      SAFE_DELETE(job);
   }
}

/**
 * @return FhgfsOpsErr_SUCCESS if everything was successfully started, FhgfsOpsErr_INUSE if already
 * running, error otherwise
 */
FhgfsOpsErr BuddyResyncer::startResync(uint16_t targetID, bool& outIsNewJob,
   BuddyResyncJobStats& outJobStats)
{
   FhgfsOpsErr retVal = FhgfsOpsErr_SUCCESS;

   // try to add an existing resync job; if it already exists, we get that
   BuddyResyncJob* resyncJob = addResyncJob(targetID, outIsNewJob);

   if (!outIsNewJob) // job already existed
   {
      resyncJob->getJobStats(outJobStats);

      if(resyncJob->isRunning())    // is this job already running?
         return FhgfsOpsErr_INUSE;
   }

   // job is ready and not running
   resyncJob->start();

   return retVal;
}

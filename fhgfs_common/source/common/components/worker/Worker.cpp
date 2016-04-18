#include <common/toolkit/TimeFine.h>
#include "Worker.h"

Worker::Worker(std::string workerID, MultiWorkQueue* workQueue, QueueWorkType workType)
   throw(ComponentInitException) :
   PThread(workerID)
{
   log.setContext(workerID);

   this->workQueue = workQueue;
   this->workType = workType;

   this->personalWorkQueue = new PersonalWorkQueue();

   HighResolutionStatsTk::resetStats(&this->stats);

   this->bufInLen = WORKER_BUFIN_SIZE;
   this->bufIn = NULL;
   this->bufOutLen = WORKER_BUFOUT_SIZE;
   this->bufOut = NULL;

   this->terminateWithFullQueue = true;
}

void Worker::run()
{
   try
   {
      registerSignalHandler();

      initBuffers();

      /* note: we're not directly calling workLoop(workType) below, because:
         1) we want to check that the given workType value is really valid.
         2) we hope that the compiler can use this explicit check and value passing to optimize away
            the if-condition inside waitForWorkByType (so that a worker does not need to check its
            worktype for every incoming work). */

      if(workType == QueueWorkType_DIRECT)
         workLoop(QueueWorkType_DIRECT);
      else
      if(workType == QueueWorkType_INDIRECT)
         workLoop(QueueWorkType_INDIRECT);
      else
      if(workType == QueueWorkType_MIRROR)
         workLoop(QueueWorkType_MIRROR);
      else
         throw ComponentInitException(
            "Unknown/invalid work type given: " + StringTk::intToStr(workType) );

      log.log(Log_DEBUG, "Component stopped.");
   }
   catch(std::exception& e)
   {
      PThread::getCurrentThreadApp()->handleComponentException(e);
   }

}

void Worker::workLoop(QueueWorkType workType)
{
   log.log(Log_DEBUG,
      "Ready ("
      "TID: " + StringTk::uint64ToStr(System::getTID() ) + "; "
      "WorkType: " + StringTk::intToStr(workType) + ")");

   workQueue->incNumWorkers(); // add this worker to queue stats

   while(!getSelfTerminate() || !maySelfTerminateNow() )
   {
      //log.log(Log_DEBUG, "Waiting for work...");

      Work* work = waitForWorkByType(stats, personalWorkQueue, workType);

      //log.log(Log_DEBUG, "Got work");

      #ifdef BEEGFS_DEBUG_PROFILING
         TimeFine workStartTime;
      #endif

      HighResolutionStatsTk::resetStats(&stats); // prepare stats

      // process the work packet
      work->process(bufIn, bufInLen, bufOut, bufOutLen);

      // update stats
      stats.incVals.workRequests = 1;
      HighResolutionStatsTk::addHighResIncStats(*work->getHighResolutionStats(), stats);

      #ifdef BEEGFS_DEBUG_PROFILING
         TimeFine workEndTime;
         int workElapsedMS = workEndTime.elapsedSinceMS(&workStartTime);
         int workElapsedMicro = workEndTime.elapsedSinceMicro(&workStartTime);

         if(workEndTime.elapsedSinceMS(&workStartTime) >= 10)
            LOG_DEBUG_CONTEXT(log, Log_DEBUG, "Work processed. Elapsed ms: " +
               StringTk::intToStr(workElapsedMS) );
         else
            LOG_DEBUG_CONTEXT(log, Log_DEBUG, "Work processed. Elapsed us: " +
               StringTk::intToStr(workElapsedMicro) );
      #endif

      // cleanup
      delete(work);
   }
}

Work* Worker::waitForWorkByType(HighResolutionStats& newStats, PersonalWorkQueue* personalWorkQueue,
   QueueWorkType workType)
{
   /* note: we hope the if-conditions below are optimized away when this is called from
      Worker::workLoop(), that's why we have the explicit work type arg in Worker::run() */

   if(workType == QueueWorkType_DIRECT)
      return workQueue->waitForDirectWork(newStats, personalWorkQueue);
   else
   if(workType == QueueWorkType_INDIRECT)
      return workQueue->waitForAnyWork(newStats, personalWorkQueue);
   else
   if(workType == QueueWorkType_MIRROR)
      return workQueue->waitForMirrorWork(newStats, personalWorkQueue);
   else
   { // should never happen
      throw WorkerException("Unknown/invalid work type given: " +
         StringTk::intToStr(workType) );
   }
}


/**
 * Note: For delayed buffer allocation during run(), because of NUMA-archs.
 */
void Worker::initBuffers()
{
   if(this->bufInLen)
   {
      void* bufInVoid = NULL;
      int inAllocRes = posix_memalign(&bufInVoid, sysconf(_SC_PAGESIZE), bufInLen);
      IGNORE_UNUSED_VARIABLE(inAllocRes);
      this->bufIn = (char*)bufInVoid;
   }

   if(this->bufOutLen)
   {
      void* bufOutVoid = NULL;
      int outAllocRes = posix_memalign(&bufOutVoid, sysconf(_SC_PAGESIZE), bufOutLen);
      IGNORE_UNUSED_VARIABLE(outAllocRes);
      this->bufOut = (char*)bufOutVoid;
   }
}

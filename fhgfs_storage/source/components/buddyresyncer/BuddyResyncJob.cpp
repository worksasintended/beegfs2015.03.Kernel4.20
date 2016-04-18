#include <program/Program.h>

#include <common/components/worker/IncSyncedCounterWork.h>
#include <common/net/message/nodes/SetTargetConsistencyStatesMsg.h>
#include <common/net/message/nodes/SetTargetConsistencyStatesRespMsg.h>
#include <common/net/message/storage/mirroring/StorageResyncStartedMsg.h>
#include <common/net/message/storage/mirroring/StorageResyncStartedRespMsg.h>
#include <common/toolkit/StringTk.h>
#include "BuddyResyncJob.h"

#define BUDDYRESYNCJOB_MAXDIRWALKDEPTH 2

BuddyResyncJob::BuddyResyncJob(uint16_t targetID) :
   PThread("BuddyResyncJob_" + StringTk::uintToStr(targetID))
{
   App* app = Program::getApp();
   unsigned numGatherSlaves = app->getConfig()->getTuneNumResyncGatherSlaves();
   unsigned numSyncSlavesTotal = app->getConfig()->getTuneNumResyncSlaves();
   unsigned numFileSyncSlaves = BEEGFS_MAX((numSyncSlavesTotal / 2), 1);
   unsigned numDirSyncSlaves = BEEGFS_MAX((numSyncSlavesTotal / 2), 1);

   this->targetID = targetID;
   this->startTime = 0;
   this->endTime = 0;
   setStatus(BuddyResyncJobStatus_NOTSTARTED);

   // prepare slaves (vectors) and result vector
   for (unsigned i = 0; i < numGatherSlaves; i++)
   {
      BuddyResyncerGatherSlave* slave = NULL;
      gatherSlaveVec.push_back(slave);
   }

   for (unsigned i = 0; i < numFileSyncSlaves; i++)
   {
      BuddyResyncerFileSyncSlave* slave = NULL;
      fileSyncSlaveVec.push_back(slave);
   }

   for (unsigned i = 0; i < numDirSyncSlaves; i++)
   {
      BuddyResyncerDirSyncSlave* slave = NULL;
      dirSyncSlaveVec.push_back(slave);
   }
}

BuddyResyncJob::~BuddyResyncJob()
{
   for(BuddyResyncerGatherSlaveVecIter iter = gatherSlaveVec.begin(); iter != gatherSlaveVec.end();
      iter++)
   {
      BuddyResyncerGatherSlave* slave = *iter;
      SAFE_DELETE(slave);
   }

   for(BuddyResyncerFileSyncSlaveVecIter iter = fileSyncSlaveVec.begin();
      iter != fileSyncSlaveVec.end(); iter++)
   {
      BuddyResyncerFileSyncSlave* slave = *iter;
      SAFE_DELETE(slave);
   }

   for(BuddyResyncerDirSyncSlaveVecIter iter = dirSyncSlaveVec.begin();
      iter != dirSyncSlaveVec.end(); iter++)
   {
      BuddyResyncerDirSyncSlave* slave = *iter;
      SAFE_DELETE(slave);
   }
}

void BuddyResyncJob::run()
{
   // make sure only one job at a time can run!
   SafeMutexLock mutexLock(&statusMutex);

   if (status == BuddyResyncJobStatus_RUNNING)
   {
      LogContext(__func__).logErr("Refusing to run same BuddyResyncJob twice!");
      return;
   }
   else
   {
      status = BuddyResyncJobStatus_RUNNING;
      startTime = time(NULL);
      endTime = 0;
   }

   mutexLock.unlock();

   App* app = Program::getApp();
   StorageTargets* storageTargets = app->getStorageTargets();
   MirrorBuddyGroupMapper* buddyGroupMapper = app->getMirrorBuddyGroupMapper();
   TargetMapper* targetMapper = app->getTargetMapper();
   NodeStoreServers* storageNodes = app->getStorageNodes();
   WorkerList* workerList = app->getWorkers();

   bool startGatherSlavesRes;
   bool startSyncSlavesRes;

   std::string targetPath;
   std::string chunksPath;

   bool buddyCommIsOverride;
   int64_t lastBuddyCommTimeSecs;
   int64_t lastBuddyCommSafetyThresholdSecs;
   bool checkTopLevelDirRes;
   bool walkRes;

   shallAbort.setZero();

   // delete sync candidates and gather queue; just in case there was something from a previous run
   syncCandidates.clear();
   gatherSlavesWorkQueue.clear();

   storageTargets->setResyncInProgress(targetID, true);

   LogContext(__func__).log(Log_NOTICE,
      "Started resync of targetID " + StringTk::uintToStr(targetID));

   // before starting the threads make sure every worker knows about the resync (the current work
   // package must be finished), for that we use a dummy package
   Mutex mutex;
   Condition counterIncrementedCond;

   SynchronizedCounter numReadyWorkers;
   size_t numWorkers = workerList->size();
   for (WorkerListIter iter = workerList->begin(); iter != workerList->end(); iter++)
   {
      Worker* worker = *iter;
      PersonalWorkQueue* personalQueue = worker->getPersonalWorkQueue();
      MultiWorkQueue* workQueue = worker->getWorkQueue();
      IncSyncedCounterWork* incCounterWork = new IncSyncedCounterWork(&numReadyWorkers);

      workQueue->addPersonalWork(incCounterWork, personalQueue);
   }

   numReadyWorkers.waitForCount(numWorkers);

   // notify buddy, that resync started and wait for confirmation
   bool commRes;
   char* respBuf = NULL;
   NetMessage* respMsg = NULL;

   uint16_t buddyTargetID = buddyGroupMapper->getBuddyTargetID(targetID);
   uint16_t nodeID = targetMapper->getNodeID(buddyTargetID);
   Node* buddyNode = storageNodes->referenceNode(nodeID);
   StorageResyncStartedMsg storageResyncStartedMsg(buddyTargetID);
   commRes = MessagingTk::requestResponse(
      buddyNode, &storageResyncStartedMsg, NETMSGTYPE_StorageResyncStartedResp, &respBuf, &respMsg);

   storageNodes->releaseNode(&buddyNode);

   if(!commRes)
   {
      LogContext(__func__).logErr("Unable to notify buddy about resync attempt. Resync will not "
         "start. targetID: " + StringTk::uintToStr(targetID));
      setStatus(BuddyResyncJobStatus_FAILURE);
      goto cleanup;
   }

   SAFE_DELETE(respMsg);
   SAFE_FREE(respBuf);

   startGatherSlavesRes = startGatherSlaves();
   if (!startGatherSlavesRes)
   {
      setStatus(BuddyResyncJobStatus_FAILURE);
      goto cleanup;
   }

   startSyncSlavesRes = startSyncSlaves();
   if (!startSyncSlavesRes)
   {
      setStatus(BuddyResyncJobStatus_FAILURE);

      // terminate gather slaves
      for (size_t i = 0; i < gatherSlaveVec.size(); i++)
         gatherSlaveVec[i]->selfTerminate();

      goto cleanup;
   }

   numDirsDiscovered.setZero();
   numDirsMatched.setZero();

   // walk over the directories until we reach a certain level and then pass the direcories to
   // gather slaves to parallelize it
   app->getStorageTargets()->getPath(targetID, &targetPath);
   chunksPath = targetPath + "/" + CONFIG_BUDDYMIRROR_SUBDIR_NAME;

   lastBuddyCommTimeSecs =
      app->getStorageTargets()->readLastBuddyCommTimestamp(targetID, &buddyCommIsOverride);
   lastBuddyCommSafetyThresholdSecs = app->getConfig()->getSysResyncSafetyThresholdMins()*60;
   if ( (lastBuddyCommSafetyThresholdSecs == 0) && (!buddyCommIsOverride) ) // ignore timestamp file
      lastBuddyCommTimeSecs = 0;
   else
   if (lastBuddyCommTimeSecs > lastBuddyCommSafetyThresholdSecs)
      lastBuddyCommTimeSecs -= lastBuddyCommSafetyThresholdSecs;

   checkTopLevelDirRes = checkTopLevelDir(chunksPath, lastBuddyCommTimeSecs);
   if (!checkTopLevelDirRes)
   {
      setStatus(BuddyResyncJobStatus_FAILURE);

      // terminate gather slaves
      for (size_t i = 0; i < gatherSlaveVec.size(); i++)
         gatherSlaveVec[i]->selfTerminate();

      // terminate sync slaves
      for (size_t i = 0; i < fileSyncSlaveVec.size(); i++)
         fileSyncSlaveVec[i]->selfTerminate();

      for (size_t i = 0; i < dirSyncSlaveVec.size(); i++)
         dirSyncSlaveVec[i]->selfTerminate();

      goto cleanup;
   }

   walkRes = walkDirs(chunksPath, "", 0, lastBuddyCommTimeSecs);
   if (!walkRes)
   {
      setStatus(BuddyResyncJobStatus_FAILURE);

      // terminate gather slaves
      for (size_t i = 0; i < gatherSlaveVec.size(); i++)
         gatherSlaveVec[i]->selfTerminate();

      // terminate sync slaves
      for (size_t i = 0; i < fileSyncSlaveVec.size(); i++)
         fileSyncSlaveVec[i]->selfTerminate();

      for (size_t i = 0; i < dirSyncSlaveVec.size(); i++)
         dirSyncSlaveVec[i]->selfTerminate();

      goto cleanup;
   }

   // all directories are read => tell gather slave to stop when work queue is empty and wait for
   // all to stop
   for(size_t i = 0; i < gatherSlaveVec.size(); i++)
   {
      if (likely(shallAbort.read() == 0))
         gatherSlaveVec[i]->setOnlyTerminateIfIdle(true);
      else
         gatherSlaveVec[i]->setOnlyTerminateIfIdle(false);

      gatherSlaveVec[i]->selfTerminate();
   }

   joinGatherSlaves();

   // gather slaves have finished => tell sync slaves to stop when work packages are empty and wait
   for(size_t i = 0; i < fileSyncSlaveVec.size(); i++)
   {
      if (likely(shallAbort.read() == 0))
         fileSyncSlaveVec[i]->setOnlyTerminateIfIdle(true);
      else
         fileSyncSlaveVec[i]->setOnlyTerminateIfIdle(false);

      fileSyncSlaveVec[i]->selfTerminate();
   }

   for(size_t i = 0; i < dirSyncSlaveVec.size(); i++)
   {
      if (likely(shallAbort.read() == 0))
         dirSyncSlaveVec[i]->setOnlyTerminateIfIdle(false);
      else
         dirSyncSlaveVec[i]->setOnlyTerminateIfIdle(true);

      dirSyncSlaveVec[i]->selfTerminate();
   }

   joinSyncSlaves();

   cleanup:
   // wait for gather slaves to stop
   for(BuddyResyncerGatherSlaveVecIter iter = gatherSlaveVec.begin();
      iter != gatherSlaveVec.end(); iter++)
   {
      BuddyResyncerGatherSlave* slave = *iter;
      if(slave)
      {
         SafeMutexLock safeLock(&(slave->statusMutex));
         while (slave->isRunning)
            slave->isRunningChangeCond.wait(&(slave->statusMutex));
         safeLock.unlock();
      }
   }

   bool syncErrors = false;

   // wait for sync slaves to stop and save if any errors occured
   for(BuddyResyncerFileSyncSlaveVecIter iter = fileSyncSlaveVec.begin();
      iter != fileSyncSlaveVec.end(); iter++)
   {
      BuddyResyncerFileSyncSlave* slave = *iter;
      if(slave)
      {
         SafeMutexLock safeLock(&(slave->statusMutex));
         while (slave->isRunning)
            slave->isRunningChangeCond.wait(&(slave->statusMutex));
         safeLock.unlock();

         if (slave->getErrorCount() != 0)
            syncErrors = true;
      }
   }

   for(BuddyResyncerDirSyncSlaveVecIter iter = dirSyncSlaveVec.begin();
      iter != dirSyncSlaveVec.end(); iter++)
   {
      BuddyResyncerDirSyncSlave* slave = *iter;
      if(slave)
      {
         SafeMutexLock safeLock(&(slave->statusMutex));
         while (slave->isRunning)
            slave->isRunningChangeCond.wait(&(slave->statusMutex));
         safeLock.unlock();

         if (slave->getErrorCount() != 0)
            syncErrors = true;
      }
   }

   if (getStatus() == BuddyResyncJobStatus_RUNNING) // status not set to anything special
   {                                                // (e.g. FAILURE)
      if (shallAbort.read() != 0) // job aborted?
      {
         setStatus(BuddyResyncJobStatus_INTERRUPTED);
         informBuddy();
      }
      else
      if (syncErrors) // any file sync errors or success?
      {
         setStatus(BuddyResyncJobStatus_ERRORS);
         informBuddy();
      }
      else
      {
         setStatus(BuddyResyncJobStatus_SUCCESS);
         // delete timestamp override file if it exists
         storageTargets->rmLastBuddyCommOverride(targetID);
         storageTargets->setBuddyNeedsResync(targetID, false);
         informBuddy();
      }
   }

   storageTargets->setResyncInProgress(targetID, false);
   endTime = time(NULL);
}

void BuddyResyncJob::abort()
{
   shallAbort.set(1); // tell the file walk in this class to abort

   // set setOnlyTerminateIfIdle on the slaves to false; they will be stopped by the main loop then
   for(BuddyResyncerGatherSlaveVecIter iter = gatherSlaveVec.begin(); iter != gatherSlaveVec.end();
      iter++)
   {
      BuddyResyncerGatherSlave* slave = *iter;
      if(slave)
      {
         slave->setOnlyTerminateIfIdle(false);
      }
   }

   // stop sync slaves
   for(BuddyResyncerFileSyncSlaveVecIter iter = fileSyncSlaveVec.begin();
      iter != fileSyncSlaveVec.end(); iter++)
   {
      BuddyResyncerFileSyncSlave* slave = *iter;
      if(slave)
      {
         slave->setOnlyTerminateIfIdle(false);
      }
   }

   for(BuddyResyncerDirSyncSlaveVecIter iter = dirSyncSlaveVec.begin();
      iter != dirSyncSlaveVec.end(); iter++)
   {
      BuddyResyncerDirSyncSlave* slave = *iter;
      if(slave)
      {
         slave->setOnlyTerminateIfIdle(false);
      }
   }
}

bool BuddyResyncJob::startGatherSlaves()
{
   int slaveResultsIndex = 0;
   // create a gather slaves if they don't exist yet and start them
   for (size_t i = 0; i < gatherSlaveVec.size(); i++)
   {
      if(!gatherSlaveVec[i])
         gatherSlaveVec[i] = new BuddyResyncerGatherSlave(targetID, &syncCandidates,
            &gatherSlavesWorkQueue, i);

      try
      {
         gatherSlaveVec[i]->resetSelfTerminate();
         gatherSlaveVec[i]->start();
         gatherSlaveVec[i]->setIsRunning(true);
      }
      catch (PThreadCreateException& e)
      {
         LogContext(__func__).logErr(std::string("Unable to start thread: ") + e.what());

         return false;
      }

      slaveResultsIndex++;
   }

   return true;
}

bool BuddyResyncJob::startSyncSlaves()
{
   // create sync slaves and start them
   for(size_t i = 0; i < fileSyncSlaveVec.size(); i++)
   {
      if(!fileSyncSlaveVec[i])
         fileSyncSlaveVec[i] = new BuddyResyncerFileSyncSlave(targetID, &syncCandidates, i);

      try
      {
         fileSyncSlaveVec[i]->resetSelfTerminate();
         fileSyncSlaveVec[i]->start();
         fileSyncSlaveVec[i]->setIsRunning(true);
      }
      catch (PThreadCreateException& e)
      {
         LogContext(__func__).logErr(std::string("Unable to start thread: ") + e.what());

         // stop already started sync slaves
         for(size_t j = 0; j < i; j++)
            fileSyncSlaveVec[j]->selfTerminate();

         return false;
      }
   }

   for(size_t i = 0; i < dirSyncSlaveVec.size(); i++)
   {
      if(!dirSyncSlaveVec[i])
         dirSyncSlaveVec[i] = new BuddyResyncerDirSyncSlave(targetID, &syncCandidates, i);

      try
      {
         dirSyncSlaveVec[i]->resetSelfTerminate();
         dirSyncSlaveVec[i]->start();
         dirSyncSlaveVec[i]->setIsRunning(true);
      }
      catch (PThreadCreateException& e)
      {
         LogContext(__func__).logErr(std::string("Unable to start thread: ") + e.what());

         // stop already started sync slaves
         for (size_t j = 0; j < fileSyncSlaveVec.size(); j++)
            fileSyncSlaveVec[j]->selfTerminate();

         for (size_t j = 0; j < i; j++)
            dirSyncSlaveVec[j]->selfTerminate();

         return false;
      }
   }

   return true;
}

void BuddyResyncJob::joinGatherSlaves()
{
   for (size_t i = 0; i < gatherSlaveVec.size(); i++)
      gatherSlaveVec[i]->join();
}

void BuddyResyncJob::joinSyncSlaves()
{
   for (size_t i = 0; i < fileSyncSlaveVec.size(); i++)
      fileSyncSlaveVec[i]->join();

   for (size_t i = 0; i < dirSyncSlaveVec.size(); i++)
      dirSyncSlaveVec[i]->join();
}

void BuddyResyncJob::getJobStats(BuddyResyncJobStats& outStats)
{
   uint64_t discoveredFiles = 0;
   uint64_t matchedFiles = 0;
   uint64_t discoveredDirs = numDirsDiscovered.read();
   uint64_t matchedDirs = numDirsMatched.read();
   uint64_t syncedFiles = 0;
   uint64_t syncedDirs = 0;
   uint64_t errorFiles = 0;
   uint64_t errorDirs = 0;

   for(size_t i = 0; i < gatherSlaveVec.size(); i++)
   {
      BuddyResyncerGatherSlave* slave = gatherSlaveVec[i];
      if(slave)
      {
         uint64_t tmpDiscoveredFiles = 0;
         uint64_t tmpMatchedFiles = 0;
         uint64_t tmpDiscoveredDirs = 0;
         uint64_t tmpMatchedDirs = 0;
         slave->getCounters(tmpDiscoveredFiles, tmpMatchedFiles, tmpDiscoveredDirs, tmpMatchedDirs);

         discoveredFiles += tmpDiscoveredFiles;
         matchedFiles += tmpMatchedFiles;
         discoveredDirs += tmpDiscoveredDirs;
         matchedDirs += tmpMatchedDirs;
      }
   }

   for(size_t i = 0; i < fileSyncSlaveVec.size(); i++)
   {
      BuddyResyncerFileSyncSlave* slave = fileSyncSlaveVec[i];
      if(slave)
      {
         syncedFiles += slave->getNumChunksSynced();
         errorFiles += slave->getErrorCount();
      }
   }

   for (size_t i = 0; i < dirSyncSlaveVec.size(); i++)
   {
      BuddyResyncerDirSyncSlave* slave = dirSyncSlaveVec[i];
      if (slave)
      {
         syncedDirs += slave->getNumDirsSynced();
         discoveredDirs += slave->getNumAdditionalDirsMatched();
         matchedDirs += slave->getNumAdditionalDirsMatched();
         errorDirs += slave->getErrorCount();
      }
   }

   outStats.update(status, startTime, endTime, discoveredFiles, matchedFiles, discoveredDirs,
      matchedDirs, syncedFiles, syncedDirs, errorFiles, errorDirs);
}

void BuddyResyncJob::informBuddy()
{
   App* app = Program::getApp();
   NodeStore* storageNodes = app->getStorageNodes();
   MirrorBuddyGroupMapper* buddyGroupMapper = app->getMirrorBuddyGroupMapper();
   TargetMapper* targetMapper = app->getTargetMapper();

   BuddyResyncJobStatus status = getStatus();
   TargetConsistencyState newTargetState;
   if ( (status == BuddyResyncJobStatus_ERRORS) || (status == BuddyResyncJobStatus_INTERRUPTED))
      newTargetState = TargetConsistencyState_BAD;
   else
   if (status == BuddyResyncJobStatus_SUCCESS)
      newTargetState = TargetConsistencyState_GOOD;
   else
   {
      LogContext(__func__).log(Log_NOTICE, "Refusing to set a state for buddy target, because "
         "resync status isn't well-defined. "
         "localTargetID: " + StringTk::uintToStr(targetID) + "; "
         "resyncStatus: " + StringTk::intToStr(status));
      return;
   }

   uint16_t buddyTargetID = buddyGroupMapper->getBuddyTargetID(targetID);
   uint16_t nodeID = targetMapper->getNodeID(buddyTargetID);
   Node* storageNode = storageNodes->referenceNode(nodeID);

   if (!storageNode)
   {
      LogContext(__func__).logErr(
         "Unable to inform buddy about finished resync. targetID: " + StringTk::uintToStr(targetID)
            + "; buddyTargetID: " + StringTk::uintToStr(buddyTargetID) + "; buddyNodeID: "
            + StringTk::uintToStr(nodeID) + "; error: unknown storage node");
      return;
   }

   bool commRes;
   char* respBuf = NULL;
   NetMessage* respMsg = NULL;
   SetTargetConsistencyStatesRespMsg* respMsgCast;
   FhgfsOpsErr result;
   UInt16List targetIDs;
   UInt8List states;

   targetIDs.push_back(buddyTargetID);
   states.push_back(newTargetState);

   SetTargetConsistencyStatesMsg msg(&targetIDs, &states, false);

   // request/response
   commRes = MessagingTk::requestResponse(
      storageNode, &msg, NETMSGTYPE_SetTargetConsistencyStatesResp, &respBuf, &respMsg);
   if(!commRes)
   {
      LogContext(__func__).logErr(
         "Unable to inform buddy about finished resync. "
         "targetID: " + StringTk::uintToStr(targetID) + "; "
         "buddyTargetID: " + StringTk::uintToStr(buddyTargetID) + "; "
         "buddyNodeID: " + StringTk::uintToStr(nodeID) + "; "
         "error: Communication error");
      goto err_cleanup;
   }

   respMsgCast = (SetTargetConsistencyStatesRespMsg*) respMsg;
   result = respMsgCast->getResult();

   if(result != FhgfsOpsErr_SUCCESS)
   {
      LogContext(__func__).logErr(
         "Error while informing buddy about finished resync. "
         "targetID: " + StringTk::uintToStr(targetID) + "; "
         "buddyTargetID: " + StringTk::uintToStr(buddyTargetID) + "; "
         "buddyNodeID: " + StringTk::uintToStr(nodeID) + "; "
         "error: " + FhgfsOpsErrTk::toErrString(result) );
   }

err_cleanup:
   storageNodes->releaseNode(&storageNode);
   SAFE_DELETE(respMsg);
   SAFE_FREE(respBuf);
}

/*
 * check the CONFIG_BUDDYMIRROR_SUBDIR_NAME directory
 */
bool BuddyResyncJob::checkTopLevelDir(std::string& path, int64_t lastBuddyCommTimeSecs)
{
   struct stat statBuf;
   int statRes = stat(path.c_str(), &statBuf);

   if(statRes != 0)
   {
      LogContext(__func__).log(Log_WARNING,
         "Couldn't stat chunks directory; resync job can't run. targetID: "
         + StringTk::uintToStr(targetID) + "; path: " + path
         + "; Error: " + System::getErrString(errno));

      return false;
   }

   numDirsDiscovered.increase();
   int64_t dirMTime = (int64_t) statBuf.st_mtim.tv_sec;
   if(dirMTime > lastBuddyCommTimeSecs)
   { // sync candidate
      ChunkSyncCandidateDir candidate("", targetID);
      syncCandidates.add(candidate, this);
      numDirsMatched.increase();
   }

   return true;
}

/*
 * recursively walk through buddy mir directory until a depth of BUDDYRESYNCJOB_MAXDIRWALKDEPTH is
 * reached; everything with a greater depth gets passed to the GatherSlaves to work on it in
 * parallel
 */
bool BuddyResyncJob::walkDirs(std::string chunksPath, std::string relPath, int level,
   int64_t lastBuddyCommTimeSecs)
{
   bool retVal = true;

   DIR* dirHandle;
   struct dirent* dirEntry;

   dirHandle = opendir(std::string(chunksPath + "/" + relPath).c_str());

   if(!dirHandle)
   {
      LogContext(__func__).logErr("Unable to open path. "
         "targetID: " + StringTk::uintToStr(targetID) + "; "
         "Rel. path: " + relPath + "; "
         "Error: " + System::getErrString(errno) );
      return false;
   }

   while ((dirEntry = StorageTk::readdirFiltered(dirHandle)) != NULL)
   {
      if(shallAbort.read() != 0)
         break;

      // get stat info
      std::string currentRelPath;
      if(unlikely(relPath.empty()))
         currentRelPath = dirEntry->d_name;
      else
         currentRelPath = relPath + "/" + dirEntry->d_name;

      std::string currentFullPath = chunksPath + "/" + currentRelPath;
      struct stat statBuf;
      int statRes = stat(currentFullPath.c_str(), &statBuf);

      if(statRes != 0)
      {
         LogContext(__func__).log(Log_WARNING,
            "Couldn't stat directory, which was discovered previously. Resync job might not be "
               "complete. targetID " + StringTk::uintToStr(targetID) + "; "
               "Rel. path: " + relPath + "; "
               "Error: " + System::getErrString(errno));

         retVal = false;

         break; // => one error aborts it all
      }

      if(S_ISDIR(statBuf.st_mode))
      {
         // if level of dir is smaller than max, take care of it and recurse into it
         if(level < BUDDYRESYNCJOB_MAXDIRWALKDEPTH)
         {
            numDirsDiscovered.increase();
            int64_t dirMTime = (int64_t) statBuf.st_mtim.tv_sec;
            if(dirMTime > lastBuddyCommTimeSecs)
            { // sync candidate
               ChunkSyncCandidateDir candidate(currentRelPath, targetID);
               syncCandidates.add(candidate, this);
               numDirsMatched.increase();
            }

            bool walkRes = walkDirs(chunksPath, currentRelPath, ++level, lastBuddyCommTimeSecs);

            if (!walkRes)
               retVal = false;
         }
         else
            // otherwise pass it to the slaves; NOTE: gather slave takes full path
            gatherSlavesWorkQueue.add(currentFullPath, this);
      }
      else
      {
         LOG_DEBUG(__func__, Log_WARNING, "Found a file in directory structure");
      }
   }

   if(!dirEntry && errno) // error occured
   {
      LogContext(__func__).logErr(
         "Unable to read all directories; chunksPath: " + chunksPath + "; relativePath: " + relPath
            + "; SysErr: " + System::getErrString(errno));

      retVal = false;
   }

   int closedirRes = closedir(dirHandle);
   if (closedirRes != 0)
      LOG_DEBUG(__func__, Log_WARNING,
         "Unable to open path. targetID " + StringTk::uintToStr(targetID) + "; Rel. path: "
         + relPath + "; Error: " + System::getErrString(errno));

   return retVal;
}

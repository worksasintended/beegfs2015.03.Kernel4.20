#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/Time.h>
#include <common/nodes/NodeStore.h>
#include <common/nodes/TargetCapacityPools.h>
#include <components/worker/MirrorMetadataWork.h>
#include <program/Program.h>
#include <app/App.h>
#include <app/config/Config.h>
#include "MetadataMirrorer.h"


#define MIRRORER_MAX_QUEUEDTASKS   10000 /* adding task will block if this threshold is exceeded */
#define MIRRORER_MAX_TASKSMSGLEN   (60*1024) /* 64KB minus some bytes for msg header/footer */
#define MIRRORER_MAX_TASKSPERMSG   64 /* not too many per msg to avoid response timeout */
#define MIRRORER_MAX_MSGSINFLIGHT  128 /* avoid spamming the comm slaves queue */


MetadataMirrorer::MetadataMirrorer() throw(ComponentInitException) : PThread("MetaMirrorer")
{
   log.setContext("MetaMirrorer");

   numTasksQueued = 0;
   numTasksInProgress = 0;
   numTaskMsgsInProgress = 0;

   lastSubmissionNodeID = 0;
}

MetadataMirrorer::~MetadataMirrorer()
{
   // delete all remaining tasks

   // walk all nodes
   for(MirrorIDsMapIter nodesMapIter = nodesMap.begin();
       nodesMapIter != nodesMap.end();
       nodesMapIter++)
   {
      // cleanup tasksInProgress list

      MirrorerTaskList& submissionList = nodesMapIter->second.tasksInProgress;

      for(MirrorerTaskListIter iter = submissionList.begin(); iter != submissionList.end(); iter++)
         delete(*iter);

      // cleanup taskQueue list

      MirrorerTaskList& taskQueue = nodesMapIter->second.taskQueue;

      for(MirrorerTaskListIter iter = taskQueue.begin(); iter != taskQueue.end(); iter++)
         delete(*iter);
   }

}


void MetadataMirrorer::run()
{
   try
   {
      registerSignalHandler();

      flushLoop();

      log.log(Log_DEBUG, "Component stopped.");
   }
   catch(std::exception& e)
   {
      PThread::getCurrentThreadApp()->handleComponentException(e);
   }
}

void MetadataMirrorer::flushLoop()
{
   //App* app = Program::getApp();

   const int sleepIntervalMS = 2500; // 2.5 secs

   SafeMutexLock safeLock(&mutex); // L O C K

   while(!getSelfTerminate() )
   {
      while(!numTasksQueued ||
            (numTaskMsgsInProgress == BEEGFS_MIN(MIRRORER_MAX_MSGSINFLIGHT, nodesMap.size() ) ) )
      { // either no tasks queued or all nodes have a message in progress => sleep
         /* (note: the test above only works, because we remove nodes that have neither queued tasks
            nor msgs in progress.) */
         notifyFlusherCondition.timedwait(&mutex, sleepIntervalMS);

         if(unlikely(getSelfTerminate() ) )
            goto unlock_and_exit;
      }


      /* get iterator at first nodeID after lastSubmissionNodeID
         (note: we use this offset for fairness and to avoid starvation of high nodeIDs in map) */
      MirrorIDsMapIter iter = nodesMap.upper_bound(lastSubmissionNodeID);

      // walk the complete map starting behind last submission offset
      for(size_t i=0; i < nodesMap.size(); )
      {
         // we didn't start at begin(), so check if we walked past the end
         if(iter == nodesMap.end() )
            iter = nodesMap.begin();

         processNodeTasks(iter->first, iter->second);

         if(!numTasksQueued ||
            (numTaskMsgsInProgress == BEEGFS_MIN(MIRRORER_MAX_MSGSINFLIGHT, nodesMap.size() ) ) )
            break; // nothing more to do at the moment

         // prepare next round...
         i++; // (may not be done in for-loop, because of the iter==nodesMap.end() case)
         iter++;
      }

   }

unlock_and_exit:
   safeLock.unlock(); // U N L O C K
}

/**
 * Add a new task to the list of a certain node
 *
 * Note: Handles locking and assignment of mirroredFromNodeID.
 *
 * @param task will be owned by mirrorer after calling this and may no longer be accessed nor
 * deleted by the caller.
 */
void MetadataMirrorer::addTaskToMap(uint16_t mirrorToNodeID, MirrorerTask* task)
{
   const unsigned waitTimeoutMS = 1000; // to avoid hanging on shutdown

   uint16_t localNodeNumID = Program::getApp()->getLocalNodeNumID();

   task->setMirroredFromNodeID(localNodeNumID);

   SafeMutexLock safeLock(&mutex); // L O C K

   // wait if too many tasks currently in queue
   while(numTasksQueued > MIRRORER_MAX_QUEUEDTASKS)
   {
      if(unlikely(getSelfTerminate() ) )
         break; // ignore limit if selfTerminate was set to avoid hanging on shutdown

      notifyAddersCondition.timedwait(&mutex, waitTimeoutMS);
   }

   // add nodeID to map if it's not in there yet
   MirrorIDsMapIter iter = nodesMap.find(mirrorToNodeID);
   if(iter == nodesMap.end() )
   { // nodeID not in map yet => insert it
      iter = nodesMap.insert(MirrorIDsMapVal(mirrorToNodeID, NodeTasks() ) ).first;
   }

   // add task to this node's queue
   iter->second.taskQueue.push_back(task);

   numTasksQueued++;

   notifyFlusherCondition.signal();

   safeLock.unlock(); // U N L O C K
}

/**
 * Process the tasks for a certain nodeID.
 *
 * Note: Caller must hold lock.
 */
void MetadataMirrorer::processNodeTasks(uint16_t nodeID, NodeTasks& tasks)
{
   if(tasks.numTasksInProgress)
      return; // we only may have one msg in flight per node to guarantee order of mirror tasks

#ifdef BEEGFS_DEBUG
   // sanity check
   if(unlikely(tasks.taskQueue.empty() ) )
   {
      LogContext(__func__).logErr("No tasks in progress and empty queue should never happen. "
         "nodeID: " + StringTk::uintToStr(nodeID) );
      return; // code below assumes tasksQueue contains at least one element
   }
#endif

   // no tasks in progress => create new mirror submission msg

   MirrorerTaskList& taskQueue = tasks.taskQueue;
   MirrorerTaskList& submissionList = tasks.tasksInProgress;
   unsigned tasksSerialLen = 0; // length of all tasks in submissionList

   // move elements to submission list until queue empty or mirrorer limit is reached
   for( ; ; )
   {
      // move first taskQueue element to end of submissionList
      tasksSerialLen += taskQueue.front()->serialLen();

      submissionList.splice(submissionList.end(), taskQueue, taskQueue.begin() );

      tasks.numTasksInProgress++;

      /* if-statement components explained:
            ||-1) no more tasks
            ||-2) tasks in progress limit for this node msg is reached
            ||-3) adding next task would exceed msg size limit */
      if( taskQueue.empty() ||
         (tasks.numTasksInProgress == MIRRORER_MAX_TASKSPERMSG) ||
         ( (tasksSerialLen + taskQueue.front()->serialLen() ) > MIRRORER_MAX_TASKSMSGLEN) )
         break;
   }

   // submit comm slaves work
   MirrorMetadataWork* work = new MirrorMetadataWork(nodeID, &tasks.tasksInProgress,
      tasks.numTasksInProgress, tasksSerialLen);
   Program::getApp()->getCommSlaveQueue()->addDirectWork(work);

   // update global stats
   numTasksInProgress += tasks.numTasksInProgress;
   numTasksQueued -= tasks.numTasksInProgress;
   numTaskMsgsInProgress++;

   lastSubmissionNodeID = nodeID;

   notifyAddersCondition.broadcast();
}


/**
 * Remove completed tasks from in-progress list.
 */
void MetadataMirrorer::completeSubmittedTasks(uint16_t nodeID)
{
   const char* logContext = "MetadataMirrorer::completeSubmittedTask";

   SafeMutexLock safeLock(&mutex); // L O C K

   LOG_DEBUG(logContext, Log_SPAM, "Completion for nodeID: " + StringTk::uintToStr(nodeID) );

   MirrorIDsMapIter nodesMapIter = nodesMap.find(nodeID);

   if(unlikely(nodesMapIter == nodesMap.end() ) )
   { // completion of unknown nodeID - should never happen
      LogContext(logContext).log(Log_WARNING,
         "Strange: Received completion for unknown nodeID. "
         "nodeID: " + StringTk::uintToStr(nodeID) );
   }
   else
   { // got the nodeID => remove corresponding tasks

      // cleanup/clear tasksInProgress list

      MirrorerTaskList& submissionList = nodesMapIter->second.tasksInProgress;

      for(MirrorerTaskListIter iter = submissionList.begin(); iter != submissionList.end(); iter++)
         delete(*iter);

      submissionList.clear();

      // update status counters
      numTaskMsgsInProgress--;
      numTasksInProgress -= nodesMapIter->second.numTasksInProgress;
      nodesMapIter->second.numTasksInProgress = 0; // only 1 msg per node => no more in progress

      // remove this node if its task queue is empty
      if(nodesMapIter->second.taskQueue.empty() )
         nodesMap.erase(nodesMapIter);

      // wake up mirrorer to refill the submission queues
      notifyFlusherCondition.broadcast();
   }

   safeLock.unlock(); // U N L O C K
}

/**
 * @param entryID leave empty if only a dentry name file should be created (e.g. for dir dentries)
 */
void MetadataMirrorer::addNewDentry(std::string parentDirID, std::string entryName,
   std::string entryID, bool hasEntryIDHardlink, uint16_t mirrorToNodeID)
{
   const char* logContext = "MetadataMirrorer::addNewDentry";

   MirrorerTask* task = new MirrorerTask();

   bool initRes = task->initAddNewDentry(parentDirID, entryName, entryID, hasEntryIDHardlink);
   if(unlikely(!initRes) )
   {
      LogContext(logContext).log(Log_WARNING,
         "Initialization of new dentry mirror task failed. "
         "parentDirID: " + parentDirID + "; "
         "entryName: " + entryName + "; "
         "entryID: " + entryID + "; "
         "mirrorNode: " + StringTk::uintToStr(mirrorToNodeID) );

      delete(task);
      return;
   }

   addTaskToMap(mirrorToNodeID, task);
}

/**
 * Static version, which just calls the non-static version. This just exists to avoid inclusion of
 * Program.h in some header files that need to call the non-static version, e.g. DirInode.h.
 */
void MetadataMirrorer::addNewDentryStatic(std::string parentDirID, std::string entryName,
   std::string entryID, bool hasEntryIDHardlink, uint16_t mirrorToNodeID)
{
   MetadataMirrorer* mirrorer = Program::getApp()->getMetadataMirrorer();

   mirrorer->addNewDentry(parentDirID, entryName, entryID, hasEntryIDHardlink, mirrorToNodeID);
}

void MetadataMirrorer::addNewInode(std::string entryID, uint16_t mirrorToNodeID)
{
   const char* logContext = "MetadataMirrorer::addNewInode";

   MirrorerTask* task = new MirrorerTask();

   bool initRes = task->initAddNewInode(entryID);
   if(unlikely(!initRes) )
   {
      LogContext(logContext).log(Log_WARNING,
         "Initialization of new inode mirror task failed. "
         "entryID: " + entryID + "; "
         "mirrorNode: " + StringTk::uintToStr(mirrorToNodeID) );

      delete(task);
      return;
   }

   addTaskToMap(mirrorToNodeID, task);
}

/**
 * Static version, which just calls the non-static version. This just exists to avoid inclusion of
 * Program.h in some header files that need to call the non-static version, e.g. DirInode.h.
 */
void MetadataMirrorer::addNewInodeStatic(std::string entryID, uint16_t mirrorToNodeID)
{
   MetadataMirrorer* mirrorer = Program::getApp()->getMetadataMirrorer();

   mirrorer->addNewInode(entryID, mirrorToNodeID);
}

/**
 * @param entryName may be empty if only entryID link should be deleted and not the name
 * dentry file (e.g. after the dentry name file was overwritten by a user's move operation).
 * @param entryID may be empty if only entryName link should be deleted and not the ID
 * dentry file (e.g. for dirs that don't have inlined inodes).
 */
void MetadataMirrorer::removeDentry(std::string parentDirID, std::string entryName,
   std::string entryID, uint16_t mirrorToNodeID)
{
   const char* logContext = "MetadataMirrorer::removeDentry";

   MirrorerTask* task = new MirrorerTask();

   bool initRes = task->initRemoveDentry(parentDirID, entryName, entryID);
   if(unlikely(!initRes) )
   {
      LogContext(logContext).log(Log_WARNING,
         "Initialization of remove dentry mirror task failed. "
         "parentDirID: " + parentDirID + "; "
         "entryName: " + entryName + "; "
         "entryID: " + entryID + "; "
         "mirrorNode: " + StringTk::uintToStr(mirrorToNodeID) );

      delete(task);
      return;
   }

   addTaskToMap(mirrorToNodeID, task);
}

/**
 * Static version, which just calls the non-static version. This just exists to avoid inclusion of
 * Program.h in some header files that need to call the non-static version, e.g. DirInode.h.
 */
void MetadataMirrorer::removeDentryStatic(std::string parentDirID, std::string entryName,
   std::string entryID, uint16_t mirrorToNodeID)
{
   MetadataMirrorer* mirrorer = Program::getApp()->getMetadataMirrorer();

   mirrorer->removeDentry(parentDirID, entryName, entryID, mirrorToNodeID);
}

void MetadataMirrorer::removeInode(std::string entryID, bool hasContentsDir,
   uint16_t mirrorToNodeID)
{
   const char* logContext = "MetadataMirrorer::removeInode";

   MirrorerTask* task = new MirrorerTask();

   bool initRes = task->initRemoveInode(entryID, hasContentsDir);
   if(unlikely(!initRes) )
   {
      LogContext(logContext).log(Log_WARNING,
         "Initialization of remove inode mirror task failed. "
         "entryID: " + entryID + "; "
         "mirrorNode: " + StringTk::uintToStr(mirrorToNodeID) );

      delete(task);
      return;
   }

   addTaskToMap(mirrorToNodeID, task);
}

/**
 * Static version, which just calls the non-static version. This just exists to avoid inclusion of
 * Program.h in some header files that need to call the non-static version, e.g. DirInode.h.
 */
void MetadataMirrorer::removeInodeStatic(std::string entryID, bool hasContentsDir,
   uint16_t mirrorToNodeID)
{
   MetadataMirrorer* mirrorer = Program::getApp()->getMetadataMirrorer();

   mirrorer->removeInode(entryID, hasContentsDir, mirrorToNodeID);
}

void MetadataMirrorer::renameDentryInSameDir(std::string parentDirID, std::string oldEntryName,
   std::string newEntryName, uint16_t mirrorToNodeID)
{
   const char* logContext = "MetadataMirrorer::renameDentryInSameDir";

   MirrorerTask* task = new MirrorerTask();

   bool initRes = task->initRenameDentryInSameDir(parentDirID, oldEntryName, newEntryName);
   if(unlikely(!initRes) )
   {
      LogContext(logContext).log(Log_WARNING,
         "Initialization of rename mirror task failed. "
         "oldName: " + oldEntryName + "; "
         "newName: " + newEntryName + "; "
         "mirrorNode: " + StringTk::uintToStr(mirrorToNodeID) );

      delete(task);
      return;
   }

   addTaskToMap(mirrorToNodeID, task);
}

/**
 * Static version, which just calls the non-static version. This just exists to avoid inclusion of
 * Program.h in some header files that need to call the non-static version, e.g. DirInode.h.
 */
void MetadataMirrorer::renameDentryInSameDirStatic(std::string parentDirID,
   std::string oldEntryName, std::string newEntryName, uint16_t mirrorToNodeID)
{
   MetadataMirrorer* mirrorer = Program::getApp()->getMetadataMirrorer();

   mirrorer->renameDentryInSameDir(parentDirID, oldEntryName, newEntryName, mirrorToNodeID);
}

/**
 * Create a hard link in the same directory.
 */
void MetadataMirrorer::hardlinkInSameDir(std::string parentDirID, std::string fromName,
   FileInode* fromInode, std::string toName, uint16_t mirrorToNodeID)
{
   const char* logContext = "MetadataMirrorer::hardlinkInSameDir";

   MirrorerTask* task = new MirrorerTask();

   bool initRes = task->initHardLinkDentryInSameDir(parentDirID, fromName, fromInode, toName);
   if(unlikely(!initRes) )
   {
      LogContext(logContext).log(Log_WARNING,
         "Initialization of rename mirror task failed. "
         "fromName: " + fromName + "; "
         "toName: "   + toName + "; "
         "mirrorNode: " + StringTk::uintToStr(mirrorToNodeID) );

      delete(task);
      return;
   }

   addTaskToMap(mirrorToNodeID, task);
}

/**
 * Static version, which just calls the non-static version. This just exists to avoid inclusion of
 * Program.h in some header files that need to call the non-static version, e.g. DirInode.h.
 */
void MetadataMirrorer::hardlinkInSameDirStatic(std::string parentDirID,
   std::string fromName, FileInode* fromInode, std::string toName, uint16_t mirrorToNodeID)
{
   MetadataMirrorer* mirrorer = Program::getApp()->getMetadataMirrorer();

   mirrorer->hardlinkInSameDir(parentDirID, fromName, fromInode, toName, mirrorToNodeID);
}


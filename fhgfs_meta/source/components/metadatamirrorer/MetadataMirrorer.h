#ifndef METADATAMIRRORER_H_
#define METADATAMIRRORER_H_

#include <common/app/log/LogContext.h>
#include <common/components/ComponentInitException.h>
#include <common/threading/PThread.h>
#include <common/threading/SafeMutexLock.h>
#include <common/toolkit/SynchronizedCounter.h>
#include <common/Common.h>
#include "MirrorerTask.h"

/**
 * This component manages the async transfer of metadata to a mirror.
 *
 * Note: We will only have one submission msg in flight for a particular nodeID to guarantee
 * order of mirror msg (e.g. avoid first transmitting a mirror file removal and afterwards a mirror
 * file update).
 */
class MetadataMirrorer : public PThread
{
   private:
      // internal type defs

      class NodeTasks
      {
         public:
            NodeTasks() : numTasksInProgress(0) {}

            MirrorerTaskList taskQueue; // tasks to be processed
            MirrorerTaskList tasksInProgress; // tasks that have been submitted to comm slaves queue
            unsigned numTasksInProgress; // tasks in progress for this node
      };

      typedef std::map<uint16_t, NodeTasks> MirrorIDsMap; // target nodeID => mirrorer task
      typedef MirrorIDsMap::iterator MirrorIDsMapIter;
      typedef MirrorIDsMap::const_iterator MirrorIDsMapCIter;
      typedef MirrorIDsMap::value_type MirrorIDsMapVal;


   public:
      MetadataMirrorer() throw(ComponentInitException);
      virtual ~MetadataMirrorer();

      void completeSubmittedTasks(uint16_t nodeID);

      void addNewDentry(std::string parentDirID, std::string entryName, std::string entryID,
         bool hasEntryIDHardlink, uint16_t mirrorToNodeID);
      void addNewInode(std::string entryID, uint16_t mirrorToNodeID);
      void removeDentry(std::string parentDirID, std::string entryName, std::string entryID,
         uint16_t mirrorNodeID);
      void removeInode(std::string entryID, bool hasContentsDir, uint16_t mirrorToNodeID);
      void renameDentryInSameDir(std::string parentDirID, std::string oldEntryName,
         std::string newEntryName, uint16_t mirrorToNodeID);

      void hardlinkInSameDir(std::string parentDirID, std::string fromName,
         FileInode* fromInode, std::string toName, uint16_t mirrorToNodeID);

      static void addNewDentryStatic(std::string parentDirID, std::string entryName,
         std::string entryID, bool hasEntryIDHardlink, uint16_t mirrorToNodeID);
      static void addNewInodeStatic(std::string entryID, uint16_t mirrorToNodeID);
      static void removeDentryStatic(std::string parentDirID, std::string entryName,
         std::string entryID, uint16_t mirrorNodeID);
      static void removeInodeStatic(std::string entryID, bool hasContentsDir,
         uint16_t mirrorToNodeID);
      static void renameDentryInSameDirStatic(std::string parentDirID, std::string oldEntryName,
         std::string newEntryName, uint16_t mirrorToNodeID);
      static void hardlinkInSameDirStatic(std::string parentDirID, std::string fromName,
         FileInode* fromInode, std::string toName, uint16_t mirrorToNodeID);


   private:
      LogContext log;

      Mutex mutex;
      Condition notifyFlusherCondition; // wake up mirrorer to start flushing
      Condition notifyAddersCondition; // wake up waiters if task queue was too full

      MirrorIDsMap nodesMap; // maps target nodeIDs to list of tasks for the given nodeID
      unsigned numTasksQueued; // aggreate number of tasks for all nodeIDs in map
      unsigned numTasksInProgress; // aggreate number of tasks for all nodeIDs in progress
      unsigned numTaskMsgsInProgress; // aggregate number of task submission msgs in progress

      uint16_t lastSubmissionNodeID; // continue after this node to avoid starvation of high nodeIDs

      virtual void run();
      void flushLoop();

      void addTaskToMap(uint16_t mirrorToNodeID, MirrorerTask* task);
      void processNodeTasks(uint16_t nodeID, NodeTasks& tasks);


   public:
      // getters & setters

      void getStats(size_t* outNumNodes, size_t* outNumTasksQueued, size_t* outNumTasksInProgress,
         size_t* outNumTaskMsgsInProgress)
      {
         SafeMutexLock safeLock(&mutex);

         *outNumNodes = nodesMap.size();
         *outNumTasksQueued = numTasksQueued;
         *outNumTasksInProgress = numTasksInProgress;
         *outNumTaskMsgsInProgress = numTaskMsgsInProgress;

         safeLock.unlock();
      }



};


#endif /* METADATAMIRRORER_H_ */

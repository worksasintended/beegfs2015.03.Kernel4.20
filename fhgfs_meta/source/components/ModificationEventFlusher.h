#ifndef MODIFICATIONEVENTFLUSHER_H
#define MODIFICATIONEVENTFLUSHER_H

#include <app/App.h>
#include <common/components/worker/Worker.h>
#include <common/components/worker/DecAtomicWork.h>
#include <common/components/worker/IncAtomicWork.h>
#include <common/components/worker/IncSyncedCounterWork.h>
#include <components/worker/BarrierWork.h>
#include <common/threading/Condition.h>
#include <common/threading/Barrier.h>
#include <common/toolkit/MetadataTk.h>
#include <components/DatagramListener.h>
#include <program/Program.h>


#define MODFLUSHER_MAXSIZE_EVENTLIST        10000
#define MODFLUSHER_SEND_AT_ONCE             10    // only very few events, because msg is UDP
#define MODFLUSHER_FLUSH_MAX_INTERVAL_MS    5000
#define MODFLUSHER_WAIT_FOR_ACK_MS          1000
#define MODFLUSHER_WAIT_FOR_ACK_RETRIES     100

/*
 * Note: this class is only used by fsck at the moment; therefore it is designed for fsck
 */
class ModificationEventFlusher: public PThread
{
   public:
      ModificationEventFlusher();
      virtual ~ModificationEventFlusher();

      virtual void run();

      bool add(ModificationEventType eventType, const std::string& entryID);

   private:
      LogContext log;

      DatagramListener* dGramLis;
      std::list<Worker*>* workerList;

      UInt8List eventTypeBufferList;
      StringList entryIDBufferList;

      // general mutex used to lock the buffer and the notification enabling and disabling
      Mutex mutex;

      Mutex eventsFlushedMutex;
      Condition eventsFlushedCond;

      Mutex eventsAddedMutex;
      Condition eventsAddedCond;

      AtomicSizeT loggingEnabled; // 1 if enabled

      AtomicSizeT numLoggingWorkers;

      Mutex fsckMutex;

      Node* fsckNode;
      bool fsckMissedEvent;

      uint64_t sentCounter;

      void sendToFsck();

   public:
      // inliners

      bool enableLogging(unsigned fsckPortUDP, NicAddressList& fsckNicList, bool forceRestart)
      {
         App* app = Program::getApp();
         WorkerList* workers = app->getWorkers();

         SafeMutexLock mutexLock(&mutex);

         if (!forceRestart && this->loggingEnabled.read() > 0)
         {
            mutexLock.unlock();
            return false;
         }

         eventTypeBufferList.clear();
         entryIDBufferList.clear();
         this->loggingEnabled.set(1);

         // set fsckParameters
         setFsckParametersUnlocked(fsckPortUDP, fsckNicList);

         mutexLock.unlock();

         stallAllWorkers(true, false);
         this->numLoggingWorkers.set(workers->size());

         return true;
      }

      bool disableLogging()
      {
         return disableLoggingLocally(true);
      }

      bool isLoggingEnabled()
      {
         return (this->loggingEnabled.read() != 0);
      }

      bool getFsckMissedEvent()
      {
         SafeMutexLock mutexLock(&fsckMutex);

         bool retVal = this->fsckMissedEvent;

         mutexLock.unlock();

         return retVal;
      }

      size_t getNumLoggingWorkers()
      {
         return this->numLoggingWorkers.read();
      }

   private:
      /*
       * Note: if logging is already disabled, this function basically does nothing, but returns
       * if the buffer is empty or not
       * @param fromWorker set to true if this is called from a worker thread. Otherwise, the worker
       *                   calling this will deadlock
       * @return true if buffer is empty, false otherwise
       */
      bool disableLoggingLocally(bool fromWorker)
      {
         if ( this->loggingEnabled.read() != 0 )
            this->loggingEnabled.setZero();

         stallAllWorkers(fromWorker, true);
         this->numLoggingWorkers.setZero();

         SafeMutexLock mutexLock(&mutex);

         // make sure list is empty and no worker is logging anymore
         bool res = this->eventTypeBufferList.empty();

         mutexLock.unlock();

         return res;
      }

      void setFsckParametersUnlocked(unsigned portUDP, NicAddressList& nicList)
      {
         this->fsckMissedEvent = false;

         this->fsckNode->updateInterfaces(portUDP, 0, nicList);
      }

      /**
       * @param fromWorker This is called from a worker thread. In that case, this function blocks
       *                   only until n-1 workers have reached the counter work item - because one
       *                   of the workers is already blocked inside this function.
       * @param flush Flush the modification event queue. Do this when stopping the modification
       *              event logger, because otherwise, workers might lock up trying to enqueue items
       *              which will never be sent to the Fsck.
       */
      void stallAllWorkers(bool fromWorker, bool flush)
      {
         App* app = Program::getApp();
         MultiWorkQueue* workQueue = app->getWorkQueue();
         pthread_t threadID = PThread::getCurrentThreadID();

         SynchronizedCounter notified;

         for (WorkerListIter workerIt = workerList->begin(); workerIt != workerList->end();
              ++workerIt)
         {
            // don't enqueue it in the worker that processes this message (this would deadlock)
            if (!PThread::threadIDEquals((*workerIt)->getID(), threadID) || !fromWorker)
            {
               PersonalWorkQueue* personalQ = (*workerIt)->getPersonalWorkQueue();
               workQueue->addPersonalWork(new IncSyncedCounterWork(&notified), personalQ);
            }
         }

         while (true)
         {
            const bool done = notified.timedWaitForCount(workerList->size() - (fromWorker ? 1 : 0),
                  1000);

            if (done)
            {
               break;
            }
            else if (flush)
            {
               SafeMutexLock safeMutexLock(&mutex);
               this->eventTypeBufferList.clear();
               this->entryIDBufferList.clear();
               safeMutexLock.unlock();

               SafeMutexLock eventsFlushedSafeLock(&eventsFlushedMutex);
               eventsFlushedCond.broadcast();
               eventsFlushedSafeLock.unlock();
            }
         }
      }
};

#endif /* MODIFICATIONEVENTFLUSHER_H */

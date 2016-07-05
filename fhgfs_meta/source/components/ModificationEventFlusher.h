#ifndef MODIFICATIONEVENTFLUSHER_H
#define MODIFICATIONEVENTFLUSHER_H

#include <app/App.h>
#include <common/components/worker/Worker.h>
#include <common/components/worker/DecAtomicWork.h>
#include <common/components/worker/IncAtomicWork.h>
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

         if (forceRestart)
         {
            const bool listEmpty = disableLoggingUnlocked();
            if (!listEmpty)
               eventTypeBufferList.clear();
         }

         this->loggingEnabled.set(1);

         // set fsckParameters
         setFsckParametersUnlocked(fsckPortUDP, fsckNicList);

         this->numLoggingWorkers.set(workers->size());
         stallAllWorkers();

         mutexLock.unlock();

         return true;
      }

      bool disableLogging()
      {
         SafeMutexLock mutexLock(&mutex);
         const bool res = disableLoggingUnlocked();
         mutexLock.unlock();
         return res;
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
      void setFsckParametersUnlocked(unsigned portUDP, NicAddressList& nicList)
      {
         this->fsckMissedEvent = false;

         this->fsckNode->updateInterfaces(portUDP, 0, nicList);
      }

      /*
       * Note: if logging is already disabled, this function basically does nothing, but returns
       * if the buffer is empty or not
       * Caller must hold mutex lock.
       *
       * @return true if buffer is empty, false otherwise
       *
       */
      bool disableLoggingUnlocked()
      {
         if ( this->loggingEnabled.read() != 0 )
            this->loggingEnabled.setZero();

         this->numLoggingWorkers.setZero();
         stallAllWorkers();

         // make sure list is empty and no worker is logging anymore
         bool listEmpty = this->eventTypeBufferList.empty();

         return listEmpty;
      }

      void stallAllWorkers()
      {
         App* app = Program::getApp();
         WorkerList* workers = app->getWorkers();
         MultiWorkQueue* workQueue = app->getWorkQueue();
         pthread_t threadID = PThread::getCurrentThreadID();

         Barrier workerBarrier(workers->size());
         for (WorkerListIter workerIt = workerList->begin(); workerIt != workerList->end();
              ++workerIt)
         {
            if (!PThread::threadIDEquals((*workerIt)->getID(), threadID))
            {
               PersonalWorkQueue* personalQ = (*workerIt)->getPersonalWorkQueue();
               workQueue->addPersonalWork(new BarrierWork(&workerBarrier), personalQ);
            }
         }

         workerBarrier.wait();
         workerBarrier.wait();
      }
};

#endif /* MODIFICATIONEVENTFLUSHER_H */

#ifndef MODIFICATIONEVENTFLUSHER_H
#define MODIFICATIONEVENTFLUSHER_H

#include <app/App.h>
#include <common/components/worker/Worker.h>
#include <common/components/worker/DecAtomicWork.h>
#include <common/components/worker/IncAtomicWork.h>
#include <common/threading/Condition.h>
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

      bool add(ModificationEventType eventType, std::string& entryID);

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

      void enableLogging(unsigned fsckPortUDP, NicAddressList& fsckNicList)
      {
         App* app = Program::getApp();
         MultiWorkQueue* workQ = app->getWorkQueue();

         SafeMutexLock mutexLock(&mutex);

         if (this->loggingEnabled.read() > 0)
         {
            mutexLock.unlock();
            return;
         }

         this->loggingEnabled.set(1);

         // set fsckParameters
         setFsckParametersUnlocked(fsckPortUDP, fsckNicList);

         this->numLoggingWorkers.setZero();

         for (std::list<Worker*>::iterator iter = workerList->begin(); iter != workerList->end();
            iter++)
         {
            Worker* worker = *iter;
            PersonalWorkQueue* personalQ = worker->getPersonalWorkQueue();
            IncAtomicWork<size_t>* incCounterWork =
               new IncAtomicWork<size_t>(&(this->numLoggingWorkers));

            workQ->addPersonalWork(incCounterWork, personalQ);
         }

         mutexLock.unlock();
      }

      /*
       * Note: if logging is already disabled, this function basically does nothing, but returns
       * if the buffer is empty or not
       *
       * @return true if buffer is empty, false otherwise
       *
       */
      bool disableLogging()
      {
         App* app = Program::getApp();
         MultiWorkQueue* workQ = app->getWorkQueue();

         SafeMutexLock mutexLock(&mutex);

         if ( this->loggingEnabled.read() != 0 )
         {
            this->loggingEnabled.setZero();

            for ( std::list<Worker*>::iterator iter = workerList->begin();
               iter != workerList->end(); iter++ )
            {
               Worker* worker = *iter;
               PersonalWorkQueue* personalQ = worker->getPersonalWorkQueue();
               DecAtomicWork<size_t>* decCounterWork = new DecAtomicWork<size_t>(
                  &(this->numLoggingWorkers));

               workQ->addPersonalWork(decCounterWork, personalQ);
            }
         }

         // make sure list is empty and no worker is logging anymore
         bool listEmpty = ( (this->eventTypeBufferList.empty())
            && (this->numLoggingWorkers.read() == 0) );

         mutexLock.unlock();

         return listEmpty;
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
};

#endif /* MODIFICATIONEVENTFLUSHER_H */

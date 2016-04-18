#ifndef LOCALCONNWORKER_H_
#define LOCALCONNWORKER_H_

#include <common/components/ComponentInitException.h>
#include <common/components/worker/Worker.h>
#include <common/threading/PThread.h>
#include <common/net/sock/StandardSocket.h>


/**
 * Handles connections from the local machine to itself (to avoid deadlocks by using
 * the standard work-queue mechanism).
 */
class LocalConnWorker : public PThread
{
   public:
      LocalConnWorker(std::string workerID)
         throw(ComponentInitException);
      
      virtual ~LocalConnWorker();
      
      
   private:
      LogContext log;
      AbstractNetMessageFactory* netMessageFactory;

      char* bufIn;
      char* bufOut;
      
      StandardSocket* workerEndpoint;
      StandardSocket* clientEndpoint;

      bool available; // == !acquired
      

      virtual void run();
      void workLoop();
      
      bool processIncomingData(char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen);
      void invalidateConnection();     
      
      void applySocketOptions(StandardSocket* sock);
      void initBuffers();

            
   public:
      // getters & setters
      Socket* getClientEndpoint()
      {
         return clientEndpoint;
      }

      bool isAvailable()
      {
         return available;
      }

      void setAvailable(bool available)
      {
         this->available = available;
      }
};

#endif /*LOCALCONNWORKER_H_*/

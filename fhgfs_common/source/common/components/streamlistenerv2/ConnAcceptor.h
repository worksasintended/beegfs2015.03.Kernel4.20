#ifndef CONNACCEPTOR_H_
#define CONNACCEPTOR_H_

#include <common/app/log/LogContext.h>
#include <common/components/worker/queue/MultiWorkQueue.h>
#include <common/components/ComponentInitException.h>
#include <common/net/sock/StandardSocket.h>
#include <common/net/sock/RDMASocket.h>
#include <common/net/message/NetMessage.h>
#include <common/nodes/Node.h>
#include <common/threading/PThread.h>
#include <common/toolkit/poll/PollList.h>
#include <common/toolkit/Pipe.h>
#include <common/Common.h>


class AbstractApp; // forward declaration


class ConnAcceptor : public PThread
{
   public:
      ConnAcceptor(AbstractApp* app, NicAddressList& localNicList, unsigned short listenPort)
         throw(ComponentInitException);
      virtual ~ConnAcceptor();
      
      
   private:
      AbstractApp*      app;
      LogContext        log;

      StandardSocket*   tcpListenSock;
      StandardSocket*   sdpListenSock;
      RDMASocket*       rdmaListenSock;
      
      int               epollFD;
      
      bool initSocks(unsigned short listenPort, NicListCapabilities* localNicCaps);

      virtual void run();
      void listenLoop();
      
      void onIncomingStandardConnection(StandardSocket* sock);
      void onIncomingRDMAConnection(RDMASocket* sock);
      
      void applySocketOptions(StandardSocket* sock);

      
   public:
      // getters & setters
      
};

#endif /*CONNACCEPTOR_H_*/

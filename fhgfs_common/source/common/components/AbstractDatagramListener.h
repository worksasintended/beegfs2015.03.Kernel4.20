#ifndef ABSTRACTDATAGRAMLISTENER_H_
#define ABSTRACTDATAGRAMLISTENER_H_

#include <common/app/log/LogContext.h>
#include <common/threading/Atomics.h>
#include <common/threading/PThread.h>
#include <common/net/sock/StandardSocket.h>
#include <common/net/message/AcknowledgeableMsg.h>
#include <common/net/message/NetMessage.h>
#include <common/nodes/AbstractNodeStore.h>
#include <common/toolkit/AcknowledgmentStore.h>
#include <common/toolkit/NetFilter.h>
#include <common/Common.h>
#include "ComponentInitException.h"


#define DGRAMMGR_RECVBUF_SIZE    65536
#define DGRAMMGR_SENDBUF_SIZE    DGRAMMGR_RECVBUF_SIZE


// forward declaration
class NetFilter;


class AbstractDatagramListener : public PThread
{
   // for efficient individual multi-notifications (needs access to sendMutex)
   friend class LockEntryNotificationWork;
   friend class LockRangeNotificationWork;


   public:
      virtual ~AbstractDatagramListener();

      void sendToNodesUDP(AbstractNodeStore* nodes, NetMessage* msg, int numRetries,
         int timeoutMS=0);
      void sendToNodesUDP(NodeList* nodes, NetMessage* msg, int numRetries, int timeoutMS=0);
      bool sendToNodesUDPwithAck(AbstractNodeStore* nodes, AcknowledgeableMsg* msg,
         int ackWaitSleepMS = 1000, int numRetries=2);
      bool sendToNodesUDPwithAck(NodeList* nodes, AcknowledgeableMsg* msg,
         int ackWaitSleepMS = 1000, int numRetries=2);
      bool sendToNodeUDPwithAck(Node* node, AcknowledgeableMsg* msg, int ackWaitSleepMS = 1000,
         int numRetries=2);
      void sendBufToNode(Node* node, const char* buf, size_t bufLen);
      void sendMsgToNode(Node* node, NetMessage* msg);

      void sendDummyToSelfUDP();


   protected:
      AbstractDatagramListener(std::string threadName, NetFilter* netFilter,
         NicAddressList& localNicList, AcknowledgmentStore* ackStore, unsigned short udpPort)
         throw(ComponentInitException);

      LogContext log;

      NetFilter* netFilter;
      AcknowledgmentStore* ackStore;
      NicAddressList localNicList;

      StandardSocket* udpSock;
      unsigned short udpPortNetByteOrder;
      in_addr_t loopbackAddrNetByteOrder; // (because INADDR_... constants are in host byte order)
      char* sendBuf;
      Mutex sendMutex;

      void handleInvalidMsg(struct sockaddr_in* fromAddr);

      virtual void handleIncomingMsg(struct sockaddr_in* fromAddr, NetMessage* msg) = 0;


   private:
      char* recvBuf;
      int recvTimeoutMS;

      AtomicUInt32 ackCounter; // used to generate ackIDs

      bool initSock(unsigned short udpPort);
      void initBuffers();

      void run();
      void listenLoop();

      bool isDGramFromSelf(struct sockaddr_in* fromAddr);
      unsigned incAckCounter();


   public:
      // inliners

      ssize_t sendto(const void* buf, size_t len, int flags,
         const struct sockaddr* to, socklen_t tolen)
      {
         SafeMutexLock mutexLock(&sendMutex); // L O C K

         try
         {
            ssize_t sendRes = udpSock->sendto(buf, len, flags, to, tolen);

            mutexLock.unlock(); // U N L O C K

            return sendRes;
         }
         catch(...)
         {
            mutexLock.unlock(); // U N L O C K
            throw;
         }
      }

      ssize_t sendto(const void *buf, size_t len, int flags,
         struct in_addr ipAddr, unsigned short port)
      {
         SafeMutexLock mutexLock(&sendMutex); // L O C K

         try
         {
            ssize_t sendRes = udpSock->sendto(buf, len, flags, ipAddr, port);

            mutexLock.unlock(); // U N L O C K

            return sendRes;
         }
         catch(...)
         {
            mutexLock.unlock(); // U N L O C K
            throw;
         }
      }

      // getters & setters
      void setRecvTimeoutMS(int recvTimeoutMS)
      {
         this->recvTimeoutMS = recvTimeoutMS;
      }

      unsigned short getUDPPort()
      {
         return Serialization::ntohsTrans(udpPortNetByteOrder);
      }
};

#endif /*ABSTRACTDATAGRAMLISTENER_H_*/

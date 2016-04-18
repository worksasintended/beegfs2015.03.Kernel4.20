#ifndef DATAGRAMLISTENER_H_
#define DATAGRAMLISTENER_H_

#include <app/log/Logger.h>
#include <common/threading/Thread.h>
#include <common/net/sock/StandardSocket.h>
#include <common/net/message/NetMessage.h>
#include <nodes/NodeStoreEx.h>
#include <common/components/worker/Worker.h>
#include <common/toolkit/NetFilter.h>
#include <common/Common.h>


#define DGRAMMGR_RECVBUF_SIZE    65536
#define DGRAMMGR_SENDBUF_SIZE    DGRAMMGR_RECVBUF_SIZE


struct DatagramListener;
typedef struct DatagramListener DatagramListener;

static inline void DatagramListener_init(DatagramListener* this, App* app, Node* localNode,
   unsigned short udpPort);
static inline DatagramListener* DatagramListener_construct(App* app, Node* localNode,
   unsigned short udpPort);
static inline void DatagramListener_uninit(DatagramListener* this);
static inline void DatagramListener_destruct(DatagramListener* this);

//extern ssize_t DatagramListener_broadcast(DatagramListener* this, void* buf, size_t len,
//   int flags, unsigned short port); // no longer needed
extern void DatagramListener_sendBufToNode(DatagramListener* this, Node* node,
   char* buf, size_t bufLen);
extern void DatagramListener_sendMsgToNode(DatagramListener* this, Node* node, NetMessage* msg);

extern void __DatagramListener_run(Thread* this);
extern void __DatagramListener_listenLoop(DatagramListener* this);

extern void _DatagramListener_handleInvalidMsg(DatagramListener* this,
   fhgfs_sockaddr_in* fromAddr);
extern void _DatagramListener_handleIncomingMsg(DatagramListener* this,
   fhgfs_sockaddr_in* fromAddr, NetMessage* msg);

extern fhgfs_bool __DatagramListener_initSock(DatagramListener* this, unsigned short udpPort);
extern void __DatagramListener_initBuffers(DatagramListener* this);
extern fhgfs_bool __DatagramListener_isDGramFromLocalhost(DatagramListener* this,
   fhgfs_sockaddr_in* fromAddr);

// inliners
static inline ssize_t DatagramListener_sendto(DatagramListener* this, void* buf, size_t len,
   int flags, fhgfs_sockaddr_in* to);
static inline ssize_t DatagramListener_sendtoIP(DatagramListener* this, void *buf, size_t len,
   int flags, fhgfs_in_addr ipAddr, unsigned short port);

// getters & setters
static inline fhgfs_bool DatagramListener_getValid(DatagramListener* this);


struct DatagramListener
{
   Thread thread;

   fhgfs_bool componentValid;

   App* app;

   char* recvBuf;

   Node* localNode;
   NetFilter* netFilter;

   StandardSocket* udpSock;
   unsigned short udpPortNetByteOrder;
   char* sendBuf;
   Mutex* sendMutex;
};


void DatagramListener_init(DatagramListener* this, App* app, Node* localNode,
   unsigned short udpPort)
{
   Thread_init( (Thread*)this, BEEGFS_THREAD_NAME_PREFIX_STR "DGramLis", __DatagramListener_run);

   this->componentValid = fhgfs_true;
   this->app = app;
   this->udpSock = NULL;

   this->localNode = localNode;
   this->netFilter = App_getNetFilter(app);

   this->recvBuf = NULL;
   this->sendBuf = NULL;
   
   this->sendMutex = Mutex_construct();

   if(!__DatagramListener_initSock(this, udpPort) )
   {
      Logger* log = App_getLogger(app);
      const char* logContext = "DatagramListener_init";

      Logger_logErr(log, logContext, "Unable to initialize the socket");

      this->componentValid = fhgfs_false;

      return;
   }

}

struct DatagramListener* DatagramListener_construct(App* app, Node* localNode,
   unsigned short udpPort)
{
   struct DatagramListener* this = (DatagramListener*)os_kmalloc(sizeof(DatagramListener) );

   DatagramListener_init(this, app, localNode, udpPort);

   return this;
}

void DatagramListener_uninit(DatagramListener* this)
{
   Socket* udpSockBase = (Socket*)this->udpSock;

   if(udpSockBase)
      Socket_virtualDestruct(udpSockBase);

   SAFE_DESTRUCT(this->sendMutex, Mutex_destruct);
   
   SAFE_VFREE(this->sendBuf);
   SAFE_VFREE(this->recvBuf);

   Thread_uninit( (Thread*)this);
}

void DatagramListener_destruct(DatagramListener* this)
{
   DatagramListener_uninit(this);

   os_kfree(this);
}


ssize_t DatagramListener_sendto(DatagramListener* this, void* buf, size_t len, int flags,
   fhgfs_sockaddr_in* to)
{
   Socket* udpSockBase = (Socket*)this->udpSock;
   ssize_t sendRes;
   
   Mutex_lock(this->sendMutex);

   sendRes = udpSockBase->sendto(udpSockBase, buf, len, flags, to);
   
   Mutex_unlock(this->sendMutex);
   
   return sendRes;
}

ssize_t DatagramListener_sendtoIP(DatagramListener* this, void *buf, size_t len, int flags,
   fhgfs_in_addr ipAddr, unsigned short port)
{
   ssize_t sendRes;

   Mutex_lock(this->sendMutex);

   sendRes = StandardSocket_sendtoIP(this->udpSock, buf, len, flags, ipAddr, port);
   
   Mutex_unlock(this->sendMutex);

   return sendRes;
}

fhgfs_bool DatagramListener_getValid(DatagramListener* this)
{
   return this->componentValid;
}

#endif /*DATAGRAMLISTENER_H_*/

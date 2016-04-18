#include "DatagramListener.h"
#include <common/toolkit/SocketTk.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/NetFilter.h>
#include <common/toolkit/Serialization.h>
#include <net/message/NetMessageFactory.h>


void __DatagramListener_run(Thread* this)
{
   DatagramListener* thisCast = (DatagramListener*)this;

   Logger* log = App_getLogger(thisCast->app);
   const char* logContext = "DatagramListener (run)";

   __DatagramListener_initBuffers(thisCast);

   __DatagramListener_listenLoop(thisCast);

   Logger_log(log, 4, logContext, "Component stopped.");
}

void __DatagramListener_listenLoop(DatagramListener* this)
{
   Logger* log = App_getLogger(this->app);
   const char* logContext = "DatagramListener (listen loop)";

   Thread* thisThread = (Thread*)this;

   fhgfs_sockaddr_in fromAddr;
   const int recvTimeoutMS = 2000;

   while(!Thread_getSelfTerminate(thisThread) )
   {
      NetMessage* msg;

      ssize_t recvRes = StandardSocket_recvfromT(this->udpSock,
         this->recvBuf, DGRAMMGR_RECVBUF_SIZE, 0, &fromAddr, recvTimeoutMS);

      if(recvRes == -ETIMEDOUT)
      { // timeout: nothing to worry about, just idle
         continue;
      }
      else
      if(unlikely(recvRes <= 0) )
      { // error

         //if(errno == -EINTR) // ignore iterruption, because the debugger makes this happen
         //   continue;

         Logger_logErrFormatted(log, logContext,
            "Encountered an unrecoverable socket error. ErrCode: %d", recvRes);

         break;
      }

      if(__DatagramListener_isDGramFromLocalhost(this, &fromAddr) )
      {
         //log.log(5, "Discarding DGram from localhost");
         continue;
      }

      msg = NetMessageFactory_createFromBuf(this->app, this->recvBuf, recvRes);

      _DatagramListener_handleIncomingMsg(this, &fromAddr, msg);

      NetMessage_virtualDestruct(msg);

   } // end of while loop
}


void _DatagramListener_handleInvalidMsg(DatagramListener* this, fhgfs_sockaddr_in* fromAddr)
{
   Logger* log = App_getLogger(this->app);
   const char* logContext = "DatagramListener (invalid msg)";

   char* ipStr = SocketTk_ipaddrToStr(&fromAddr->addr);

   Logger_logFormatted(log, 3, logContext, "Received invalid data from: %s", ipStr);

   os_kfree(ipStr);
}

void _DatagramListener_handleIncomingMsg(DatagramListener* this,
   fhgfs_sockaddr_in* fromAddr, NetMessage* msg)
{
   Logger* log = App_getLogger(this->app);
   const char* logContext = "DatagramListener (incoming msg)";

   switch(NetMessage_getMsgType(msg) )
   {
      // valid messages within this context
      case NETMSGTYPE_Ack:
      case NETMSGTYPE_HeartbeatRequest:
      case NETMSGTYPE_Heartbeat:
      case NETMSGTYPE_MapTargets:
      case NETMSGTYPE_RemoveNode:
      case NETMSGTYPE_LockGranted:
      case NETMSGTYPE_RefreshTargetStates:
      case NETMSGTYPE_SetMirrorBuddyGroup:
      {
         if(!msg->processIncoming(msg, this->app, fromAddr, (Socket*)this->udpSock,
            this->sendBuf, DGRAMMGR_SENDBUF_SIZE) )
         {
            Logger_log(log, 2, logContext,
               "Problem encountered during handling of incoming message");
         }
      } break;

      case NETMSGTYPE_Invalid:
      {
         _DatagramListener_handleInvalidMsg(this, fromAddr);
      } break;

      default:
      { // valid fhgfs message, but not allowed within this context
         char* ipStr = SocketTk_ipaddrToStr(&fromAddr->addr);
         Logger_logErrFormatted(log, logContext, "Received a message of type %d "
            "that is invalid within the current context from: %s",
            NetMessage_getMsgType(msg), ipStr);
         os_kfree(ipStr);
      } break;
   };
}

fhgfs_bool __DatagramListener_initSock(DatagramListener* this, unsigned short udpPort)
{
   Config* cfg = App_getConfig(this->app);
   Logger* log = App_getLogger(this->app);
   const char* logContext = "DatagramListener (init sock)";

   fhgfs_bool broadcastRes;
   fhgfs_bool bindRes;
   Socket* udpSockBase;

   this->udpPortNetByteOrder = Serialization_htonsTrans(udpPort);

   this->udpSock = StandardSocket_constructUDP();
   udpSockBase = (Socket*)this->udpSock;

   if(!Socket_getSockValid(udpSockBase) )
   {
      Logger_logErr(log, logContext, "Initialization of UDP socket failed");
      return fhgfs_false;
   }

   // set some socket options

   broadcastRes = StandardSocket_setSoBroadcast(this->udpSock, fhgfs_true);
   if(!broadcastRes)
   {
      Logger_logErr(log, logContext, "Enabling broadcast for UDP socket failed.");
      return fhgfs_false;
   }

   StandardSocket_setSoRcvBuf(this->udpSock,
      Config_getConnRDMABufNum(cfg) * Config_getConnRDMABufSize(cfg) );

   // bind the socket

   bindRes = Socket_bind(udpSockBase, udpPort);
   if(!bindRes)
   {
      Logger_logErrFormatted(log, logContext, "Binding UDP socket to port %d failed.", udpPort);
      return fhgfs_false;
   }

   Logger_logFormatted(log, 3, logContext, "Listening for UDP datagrams: Port %d", udpPort);

   return fhgfs_true;
}

/**
 * Note: Delayed init of buffers (better for NUMA).
 */
void __DatagramListener_initBuffers(DatagramListener* this)
{
   this->recvBuf = (char*)os_vmalloc(DGRAMMGR_RECVBUF_SIZE);
   this->sendBuf = (char*)os_vmalloc(DGRAMMGR_SENDBUF_SIZE);
}


// no longer needed
///**
// * Broadcasts to each broadcast address of the (allowed) local standard interfaces.
// */
//ssize_t DatagramListener_broadcast(DatagramListener* this, void* buf, size_t len, int flags,
//   unsigned short port)
//{
//   ssize_t sendRes = 0;
//   NetFilter* netFilter = App_getNetFilter(this->app);
//   NicAddressList* nicList = Node_getNicList(this->localNode);
//   NicAddressListIter iter;
//
//   NicAddressListIter_init(&iter, nicList);
//
//   for( ; !NicAddressListIter_end(&iter); NicAddressListIter_next(&iter) )
//   {
//      NicAddress* nicAddr = NicAddressListIter_value(&iter);
//
//      if(nicAddr->nicType != NICADDRTYPE_STANDARD)
//         continue;
//
//      if(!NetFilter_isAllowed(netFilter, nicAddr->ipAddr) )
//         continue;
//
//      Mutex_lock(this->sendMutex);
//
//      sendRes = StandardSocket_broadcast(
//         this->udpSock, buf, len, flags, &(nicAddr->broadcastAddr), port);
//
//      Mutex_unlock(this->sendMutex);
//
//      if(sendRes <= 0)
//         break;
//   }
//
//   NicAddressListIter_uninit(&iter);
//
//   return sendRes;
//}

/**
 * Sends the buffer to all available node interfaces.
 */
void DatagramListener_sendBufToNode(DatagramListener* this, Node* node,
   char* buf, size_t bufLen)
{
   NodeConnPool* connPool = Node_getConnPool(node);
   NicAddressList* nicList = NodeConnPool_getNicList(connPool);
   unsigned short port = Node_getPortUDP(node);
   NicAddressListIter iter;

   NicAddressListIter_init(&iter, nicList);

   for( ; !NicAddressListIter_end(&iter); NicAddressListIter_next(&iter) )
   {
      NicAddress* nicAddr = NicAddressListIter_value(&iter);

      if(nicAddr->nicType != NICADDRTYPE_STANDARD)
         continue;

      if(!NetFilter_isAllowed(this->netFilter, nicAddr->ipAddr) )
         continue;

      DatagramListener_sendtoIP(this, buf, bufLen, 0, nicAddr->ipAddr, port);
   }

   NicAddressListIter_uninit(&iter);
}

/**
 * Sends the message to all available node interfaces.
 */
void DatagramListener_sendMsgToNode(DatagramListener* this, Node* node, NetMessage* msg)
{
   char* msgBuf = MessagingTk_createMsgBuf(msg);
   unsigned msgLen = NetMessage_getMsgLength(msg);

   DatagramListener_sendBufToNode(this, node, msgBuf, msgLen);

   os_kfree(msgBuf);
}

fhgfs_bool __DatagramListener_isDGramFromLocalhost(DatagramListener* this,
   fhgfs_sockaddr_in* fromAddr)
{
   NicAddressList* nicList;
   NicAddressListIter iter;
   int nicListSize;
   int i;

   if(fromAddr->port != this->udpPortNetByteOrder)
      return fhgfs_false;

   nicList = Node_getNicList(this->localNode);

   NicAddressListIter_init(&iter, nicList);
   nicListSize = NicAddressList_length(nicList);

   for(i = 0; i < nicListSize; i++, NicAddressListIter_next(&iter) )
   {
      NicAddress* nicAddr = NicAddressListIter_value(&iter);

      if(nicAddr->ipAddr == fromAddr->addr)
      {
         NicAddressListIter_uninit(&iter);
         return fhgfs_true;
      }
   }

   NicAddressListIter_uninit(&iter);

   if(Serialization_ntohlTrans(BEEGFS_INADDR_LOOPBACK) == fromAddr->addr)
      return fhgfs_true;

   // (inaddr_loopback is in host byte order)

   return fhgfs_false;
}

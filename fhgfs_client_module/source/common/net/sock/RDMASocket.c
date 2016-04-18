#include <common/net/sock/RDMASocket.h>
#include <common/Common.h>

#include <linux/in.h>
#include <linux/poll.h>


// Note: Good tradeoff between throughput and mem usage (for SDR IB):
//    buf_num=64; buf_size=4*1024 (=> 512kB per socket for send and recv)

#define RDMASOCKET_DEFAULT_BUF_NUM                    (128) // moved to config
#define RDMASOCKET_DEFAULT_BUF_SIZE                   (4*1024) // moved to config


void RDMASocket_init(RDMASocket* this)
{
   Socket* thisBase = (Socket*)this;


   // init super class
   _PooledSocket_init( (PooledSocket*)this, NICADDRTYPE_RDMA);

   // assign virtual functions
   thisBase->uninit = _RDMASocket_uninit;

   thisBase->connectByIP = _RDMASocket_connectByIP;
   thisBase->bindToAddr = _RDMASocket_bindToAddr;
   thisBase->listen = _RDMASocket_listen;
   thisBase->shutdown = _RDMASocket_shutdown;
   thisBase->shutdownAndRecvDisconnect = _RDMASocket_shutdownAndRecvDisconnect;

   thisBase->send = _RDMASocket_send;
   thisBase->sendto = _RDMASocket_sendto;

   thisBase->recv = _RDMASocket_recv;
   thisBase->recvT = _RDMASocket_recvT;

   // normal init part

   thisBase->sockValid = fhgfs_false;

   thisBase->sockType = NICADDRTYPE_RDMA;

   this->commCfg.bufNum = RDMASOCKET_DEFAULT_BUF_NUM;
   this->commCfg.bufSize = RDMASOCKET_DEFAULT_BUF_SIZE;

   this->ibvsock = IBVSocket_construct();

   if(!this->ibvsock)
   {
      //printk_fhgfs(KERN_WARNING, "RDMASocket allocation failed\n");
      return;
   }

   if(!IBVSocket_getSockValid(this->ibvsock) )
   {
      //printk_fhgfs(KERN_WARNING, "RDMASocket initialization failed.\n");
      SAFE_DESTRUCT(this->ibvsock, IBVSocket_destruct);

      return;
   }

   thisBase->sockValid = fhgfs_true;
}

RDMASocket* RDMASocket_construct(void)
{
   RDMASocket* this = (RDMASocket*)os_kmalloc(sizeof(*this) );

   RDMASocket_init(this);

   return this;
}

void _RDMASocket_uninit(Socket* this)
{
   RDMASocket* thisCast = (RDMASocket*)this;

   _PooledSocket_uninit(this);

   SAFE_DESTRUCT(thisCast->ibvsock, IBVSocket_destruct);
}

void _RDMASocket_destruct(Socket* this)
{
   _RDMASocket_uninit(this);

   kfree(this);
}

fhgfs_bool RDMASocket_rdmaDevicesExist(void)
{
   return IBVSocket_rdmaDevicesExist();
}

fhgfs_bool _RDMASocket_connectByIP(Socket* this, fhgfs_in_addr* ipaddress, unsigned short port)
{
   // note: does not set the family type to the one of this socket.

   RDMASocket* thisCast = (RDMASocket*)this;

   struct in_addr osIPAddress;
   fhgfs_bool connRes;

   osIPAddress.s_addr = *ipaddress;

   connRes = IBVSocket_connectByIP(thisCast->ibvsock, &osIPAddress, port, &thisCast->commCfg);

   if(!connRes)
   {
      // note: this message would flood the log if hosts are unreachable on the primary interface

      //char* ipStr = SocketTk_ipaddrToStr(ipaddress);
      //printk_fhgfs(KERN_WARNING, "RDMASocket failed to connect to %s.\n", ipStr);
      //kfree(ipStr);

      return fhgfs_false;
   }

   // connected

   // set peername if not done so already (e.g. by connect(hostname) )
   if(!this->peername)
   {
      this->peername = SocketTk_endpointAddrToString(ipaddress, port);
      this->peerIP = *ipaddress;
   }

   return fhgfs_true;
}

fhgfs_bool _RDMASocket_bindToAddr(Socket* this, fhgfs_in_addr* ipaddress, unsigned short port)
{
   RDMASocket* thisCast = (RDMASocket*)this;

   struct in_addr osIPAddress;
   fhgfs_bool bindRes;
   size_t peernameLen;

   osIPAddress.s_addr = *ipaddress;

   bindRes = IBVSocket_bindToAddr(thisCast->ibvsock, &osIPAddress, port);
   if(!bindRes)
   {
      //printk_fhgfs_debug(KERN_INFO, "Failed to bind RDMASocket.\n"); // debug in
      return fhgfs_false;
   }


   // 16 chars text + 8 (include max port len + colon + terminating zero)
   peernameLen = 16 + 8;
   this->peername = os_kmalloc(peernameLen);
   snprintf(this->peername, peernameLen, "Listen(Port: %u)", (unsigned)port);

   return fhgfs_true;
}

fhgfs_bool _RDMASocket_listen(Socket* this)
{
   RDMASocket* thisCast = (RDMASocket*)this;

   fhgfs_bool listenRes;

   listenRes = IBVSocket_listen(thisCast->ibvsock);
   if(!listenRes)
   {
      printk_fhgfs(KERN_WARNING, "Failed to set RDMASocket to listening mode.\n");
      return fhgfs_false;
   }

   return fhgfs_true;
}

fhgfs_bool _RDMASocket_shutdown(Socket* this)
{
   RDMASocket* thisCast = (RDMASocket*)this;

   fhgfs_bool shutRes = IBVSocket_shutdown(thisCast->ibvsock);
   if(!shutRes)
   {
      printk_fhgfs_debug(KERN_INFO, "RDMASocket failed to send shutdown.\n");
      return fhgfs_false;
   }

   return fhgfs_true;
}

/**
 * Note: The RecvDisconnect-part is currently not implemented, so this is equal to the
 * normal shutdown() method.
 */
fhgfs_bool _RDMASocket_shutdownAndRecvDisconnect(Socket* this, int timeoutMS)
{
   return this->shutdown(this);
}


ssize_t _RDMASocket_recv(Socket* this, void *buf, size_t len, int flags)
{
   RDMASocket* thisCast = (RDMASocket*)this;

   ssize_t retVal;
   mm_segment_t oldfs;

   //printk_fhgfs(KERN_INFO, "%s:%d: len: %d; flags: %d\n", __func__, __LINE__,
   //   (int)len, flags); // remove me

   ACQUIRE_PROCESS_CONTEXT(oldfs);

   retVal = IBVSocket_recv(thisCast->ibvsock, (char*)buf, len, flags);

   RELEASE_PROCESS_CONTEXT(oldfs);

   //printk_fhgfs(KERN_INFO, "%s:%d: return %d\n", __func__, __LINE__, (int)retVal); // remove me

   return retVal;
}

/**
 * @return -ETIMEDOUT on timeout
 */
ssize_t _RDMASocket_recvT(Socket* this, void *buf, size_t len, int flags, int timeoutMS)
{
   RDMASocket* thisCast = (RDMASocket*)this;

   ssize_t retVal;
   mm_segment_t oldfs;

   //printk_fhgfs(KERN_INFO, "%s:%d: len: %d; flags: %d; timeoutMS: %d\n", __func__, __LINE__,
   //   (int)len, flags, timeoutMS); // remove me

   ACQUIRE_PROCESS_CONTEXT(oldfs);

   retVal = IBVSocket_recvT(thisCast->ibvsock, (char*)buf, len, flags, timeoutMS);

   RELEASE_PROCESS_CONTEXT(oldfs);

   //printk_fhgfs(KERN_INFO, "%s:%d: return %d\n", __func__, __LINE__, (int)retVal); // remove me

   return retVal;
}

/**
 * Note: This is a connection-based socket type, so to and tolen are ignored.
 *
 * @param flags ignored
 */
ssize_t _RDMASocket_sendto(Socket* this, const void *buf, size_t len, int flags,
   fhgfs_sockaddr_in *to)
{
   RDMASocket* thisCast = (RDMASocket*)this;

   ssize_t retVal;
   mm_segment_t oldfs;

   ACQUIRE_PROCESS_CONTEXT(oldfs);

   retVal = IBVSocket_send(thisCast->ibvsock, (const char*)buf, len, flags);

   RELEASE_PROCESS_CONTEXT(oldfs);

   return retVal;
}

ssize_t _RDMASocket_send(Socket* this, const void *buf, size_t len, int flags)
{
   RDMASocket* thisCast = (RDMASocket*)this;

   ssize_t retVal;
   mm_segment_t oldfs;

   //printk_fhgfs(KERN_INFO, "%s:%d: len: %d; flags: %d\n", __func__, __LINE__,
   //   (int)len, flags); // remove me

   ACQUIRE_PROCESS_CONTEXT(oldfs);

   retVal = IBVSocket_send(thisCast->ibvsock, (const char*)buf, len, flags);

   RELEASE_PROCESS_CONTEXT(oldfs);

   //printk_fhgfs(KERN_INFO, "%s:%d: return %d\n", __func__, __LINE__, (int)retVal); // remove me

   return retVal;
}

/**
 * Note: Don't call this for sockets that have never been connected!
 *
 * @return fhgfs_false on connection error
 */
fhgfs_bool RDMASocket_checkConnection(RDMASocket* this)
{
   int checkRes;

   checkRes = IBVSocket_checkConnection(this->ibvsock);
   if(checkRes)
      return fhgfs_false;

   return fhgfs_true;
}

/**
 * Register for polling (=> this method does not call schedule() !).
 *
 * Note: Call this only once with finishPoll==fhgfs_true (=> non-blocking) or multiple times with
 *    finishPoll==fhgfs_true in the last call from the current thread (for cleanup).
 * Note: It's safe to call this multiple times with finishPoll==fhgfs_true.
 *
 * @param events the event flags you are interested in (POLL...)
 * @param finishPoll true for cleanup if you don't call poll again from this thread; (it's also ok
 *    to set this to true if you call poll only once and want to avoid blocking)
 * @return mask revents mask (like poll() => POLL... flags), but only the events you requested or
 *    error events
 */
unsigned long RDMASocket_poll(RDMASocket* this, short events, fhgfs_bool finishPoll)
{
   unsigned long retVal;

   //printk_fhgfs(KERN_INFO, "%s:%d: events: (in: %d; out: %d); finish: %d\n", __func__, __LINE__,
   //   events & POLLIN, events & POLLOUT, (int)finishPoll); // remove me

   retVal = IBVSocket_poll(this->ibvsock, events, finishPoll);

   //printk_fhgfs(KERN_INFO, "%s:%d: return events: (in: %d; out: %d)\n", __func__, __LINE__,
   //   retVal & POLLIN, retVal & POLLOUT); // remove me

   return retVal;
}


#include <common/net/sock/StandardSocket.h>
#include <common/toolkit/Serialization.h>
#include <common/toolkit/SocketTk.h>
#include <common/Common.h>

#include <linux/in.h>
#include <linux/tcp.h>


#define SOCKET_LISTEN_BACKLOG                32
#define SOCKET_SHUTDOWN_RECV_BUF_LEN         32
#define STANDARDSOCKET_CONNECT_TIMEOUT_MS    5000


void StandardSocket_init(StandardSocket* this, int domain, int type, int protocol)
{
   Socket* thisBase = (Socket*)this;

   NicAddrType_t nicType = NICADDRTYPE_STANDARD;

   // init super class
   if(domain == PF_SDP)
      nicType = NICADDRTYPE_SDP;

   _PooledSocket_init( (PooledSocket*)this, nicType);

   // assign virtual functions
   thisBase->uninit = _StandardSocket_uninit;

   thisBase->connectByIP = _StandardSocket_connectByIP;
   thisBase->bindToAddr = _StandardSocket_bindToAddr;
   thisBase->listen = _StandardSocket_listen;
   thisBase->shutdown = _StandardSocket_shutdown;
   thisBase->shutdownAndRecvDisconnect = _StandardSocket_shutdownAndRecvDisconnect;

   thisBase->send = _StandardSocket_send;
   thisBase->sendto = _StandardSocket_sendto;

   thisBase->recv = _StandardSocket_recv;
   thisBase->recvT = _StandardSocket_recvT;

   // normal init part

   this->sock = NULL;

   this->sockDomain = domain;

   if(domain == PF_SDP)
      thisBase->sockType = NICADDRTYPE_SDP;

   thisBase->sockValid = _StandardSocket_initSock(this, domain, type, protocol);
   if(!thisBase->sockValid)
      return;
}

StandardSocket* StandardSocket_construct(int domain, int type, int protocol)
{
   StandardSocket* this = (StandardSocket*)os_kmalloc(sizeof(*this) );

   StandardSocket_init(this, domain, type, protocol);

   return this;
}

StandardSocket* StandardSocket_constructUDP(void)
{
   StandardSocket* this = (StandardSocket*)os_kmalloc(sizeof(*this) );

   StandardSocket_init(this, PF_INET, SOCK_DGRAM, 0);

   return this;
}

StandardSocket* StandardSocket_constructTCP(void)
{
   StandardSocket* this = (StandardSocket*)os_kmalloc(sizeof(*this) );

   StandardSocket_init(this, PF_INET, SOCK_STREAM, 0);

   return this;
}

StandardSocket* StandardSocket_constructSDP(void)
{
   StandardSocket* this = (StandardSocket*)os_kmalloc(sizeof(*this) );

   StandardSocket_init(this, PF_SDP, SOCK_STREAM, 0);

   return this;
}

void _StandardSocket_uninit(Socket* this)
{
   StandardSocket* thisCast = (StandardSocket*)this;

   _PooledSocket_uninit(this);

   if(thisCast->sock)
      sock_release(thisCast->sock);
}

void _StandardSocket_destruct(Socket* this)
{
   _StandardSocket_uninit(this);

   kfree(this);
}

/**
 * @param outEndpointA only valid if return is success; has to be destructed by the caller
 * @param outEndpointB only valid if return is success; has to be destructed by the caller
 * @return fhgfs_false on error
 */
fhgfs_bool StandardSocket_createSocketPair(int domain, int type, int protocol,
   StandardSocket** outEndpointA, StandardSocket** outEndpointB)
{
   long pairErr;

   *outEndpointA = StandardSocket_construct(domain, type, protocol);
   if(!StandardSocket_getSockValid(*outEndpointA) )
      goto destruct_first_endpoint;

   *outEndpointB = StandardSocket_construct(domain, type, protocol);
   if(!StandardSocket_getSockValid(*outEndpointB) )
      goto destruct_both_endpoints;

   // pair the endoints
   pairErr = (*outEndpointA)->sock->ops->socketpair( (*outEndpointA)->sock, (*outEndpointB)->sock);
   if(pairErr < 0)
      goto destruct_both_endpoints;

   // todo: peernames need to be set here

   return fhgfs_true;


destruct_both_endpoints:
   _StandardSocket_destruct( (Socket*)*outEndpointB);

destruct_first_endpoint:
   _StandardSocket_destruct( (Socket*)*outEndpointA);

   return fhgfs_false;
}

fhgfs_bool _StandardSocket_initSock(StandardSocket* this, int domain, int type, int protocol)
{
   int createRes;

   // prepare/create socket
#ifndef KERNEL_HAS_SOCK_CREATE_KERN_NS
   createRes = sock_create_kern(domain, type, protocol, &this->sock);
#else
   createRes = sock_create_kern(&init_net, domain, type, protocol, &this->sock);
#endif
   if(createRes < 0)
   {
      //printk_fhgfs(KERN_WARNING, "Failed to create socket\n");
      return fhgfs_false;
   }

   __StandardSocket_setAllocMode(this, GFP_NOFS);

   return fhgfs_true;
}

void __StandardSocket_setAllocMode(StandardSocket* this, gfp_t flags)
{
   this->sock->sk->sk_allocation = flags;
}


/**
 * Use this to change socket options.
 * Note: Behaves (almost) like user-space setsockopt.
 *
 * @return 0 on success, error code otherwise (=> different from userspace version)
 */
int _StandardSocket_setsockopt(StandardSocket* this, int level,
   int optname, char* optval, int optlen)
{
   int retVal = -EINVAL;
   mm_segment_t oldfs;

   if(optlen < 0)
      return retVal;

   ACQUIRE_PROCESS_CONTEXT(oldfs);

   if (level == SOL_SOCKET)
      retVal = sock_setsockopt(this->sock, level, optname, optval, optlen);
   else
      retVal = this->sock->ops->setsockopt(this->sock, level, optname, optval, optlen);

   RELEASE_PROCESS_CONTEXT(oldfs);

   return retVal;
}

/**
 * Use this to query socket options.
 * Note: Behaves (almost) like user-space setsockopt.
 * Note: Only limited set of optnames supported.
 *
 * @param optname currently, only
 * @param optlen is an in and out argument
 * @return 0 on success, negative error code otherwise (=> different from userspace version)
 */
int _StandardSocket_getsockopt(StandardSocket* this, int level, int optname,
   char* optval, int* optlen)
{
   int retVal = -EINVAL;
   mm_segment_t oldfs;

   if(*optlen < 0)
      return retVal;

   ACQUIRE_PROCESS_CONTEXT(oldfs);

   // note: sock_getsockopt() is not exported to modules

   if(level == SOL_SOCKET)
   {
      switch(optname)
      {
         case SO_RCVBUF:
         {
            if(*optlen == sizeof(int) )
            {
               struct socket* sock = StandardSocket_getRawSock(this);
               *(int*)optval = sock->sk->sk_rcvbuf;
               retVal = 0;
            }
         } break;

         default:
         {
            retVal = -ENOTSUPP;
         } break;
      }
   }
   else
      retVal = this->sock->ops->getsockopt(this->sock, level, optname, optval, optlen);

   RELEASE_PROCESS_CONTEXT(oldfs);

   return retVal;
}

fhgfs_bool StandardSocket_setSoKeepAlive(StandardSocket* this, fhgfs_bool enable)
{
   int keepAliveVal = (enable ? 1 : 0);

   int setRes = _StandardSocket_setsockopt(this,
      SOL_SOCKET,
      SO_KEEPALIVE,
      (char*)&keepAliveVal,
      sizeof(keepAliveVal) );

   if(setRes != 0)
      return fhgfs_false;

   return fhgfs_true;
}

fhgfs_bool StandardSocket_setSoBroadcast(StandardSocket* this, fhgfs_bool enable)
{
   int broadcastVal = (enable ? 1 : 0);

   int setRes = _StandardSocket_setsockopt(this,
      SOL_SOCKET,
      SO_BROADCAST,
      (char*)&broadcastVal,
      sizeof(broadcastVal) );

   if(setRes != 0)
      return fhgfs_false;

   return fhgfs_true;
}

fhgfs_bool StandardSocket_setSoReuseAddr(StandardSocket* this, fhgfs_bool enable)
{
   int reuseVal = (enable ? 1 : 0);

   int setRes = _StandardSocket_setsockopt(this,
      SOL_SOCKET,
      SO_REUSEADDR,
      (char*)&reuseVal,
      sizeof(reuseVal) );

   if(setRes != 0)
      return fhgfs_false;

   return fhgfs_true;
}

fhgfs_bool StandardSocket_getSoRcvBuf(StandardSocket* this, int* outSize)
{
   int optlen = sizeof(*outSize);

   int getRes = _StandardSocket_getsockopt(this,
      SOL_SOCKET,
      SO_RCVBUF,
      (char*)outSize,
      &optlen);

   if(getRes)
   {
      printk_fhgfs_debug(KERN_INFO, "getSoRcvBuf: error code: %d\n", getRes);
      return fhgfs_false;
   }

   return fhgfs_true;
}

/**
 * Note: Increase only (buffer will not be set to a smaller value).
 *
 * @return fhgfs_false on error, fhgfs_true otherwise (decrease skipping is not an error)
 */
fhgfs_bool StandardSocket_setSoRcvBuf(StandardSocket* this, int size)
{
   /* note: according to socket(7) man page, the value given to setsockopt() is doubled and the
      doubled value is returned by getsockopt() */

   int halfSize = size/2;
   fhgfs_bool getOrigRes;
   int origBufLen;
   int newBufLen;
   int setRes;

   getOrigRes = StandardSocket_getSoRcvBuf(this, &origBufLen);
   if(!getOrigRes)
      return fhgfs_false;

   if(origBufLen >= (size) )
   { // we don't decrease buf sizes (but this is not an error)
      return fhgfs_true;
   }

   setRes = _StandardSocket_setsockopt(this,
      SOL_SOCKET,
      SO_RCVBUF,
      (char*)&halfSize,
      sizeof(halfSize) );

   if(setRes)
      printk_fhgfs_debug(KERN_INFO, "%s: setSoRcvBuf error: %d;\n", __func__, setRes);

   StandardSocket_getSoRcvBuf(this, &newBufLen);

   return fhgfs_true;
}


fhgfs_bool StandardSocket_setTcpNoDelay(StandardSocket* this, fhgfs_bool enable)
{
   int noDelayVal = (enable ? 1 : 0);

   int noDelayRes = _StandardSocket_setsockopt(this,
      IPPROTO_TCP,
      TCP_NODELAY,
      (char*)&noDelayVal,
      sizeof(noDelayVal) );

   if(noDelayRes != 0)
      return fhgfs_false;

   return fhgfs_true;
}

fhgfs_bool StandardSocket_setTcpCork(StandardSocket* this, fhgfs_bool enable)
{
   int corkVal = (enable ? 1 : 0);

   int setRes = _StandardSocket_setsockopt(this,
      SOL_TCP,
      TCP_CORK,
      (char*)&corkVal,
      sizeof(corkVal) );

   if(setRes != 0)
      return fhgfs_false;

   return fhgfs_true;
}

/*
fhgfs_bool StandardSocket_connectByName(StandardSocket* this, const char* hostname,
   unsigned short port, ExternalHelperd* helperd)
{
   size_t peernameLen;

   struct in_addr hostAddr;

   SocketTk_getHostByName(helperd, hostname, &hostAddr);
   //hostAddr.s_addr = in_aton(hostname);

   // set peername
   peernameLen = strlen(hostname) + 8; // 8 include max port len + colon + terminating zero
   this->peername = os_kmalloc(peernameLen);
   snprintf(this->peername, peernameLen, "%s:%u", hostname, (unsigned)port);

   // connect
   return StandardSocket_connectByIP(this, &hostAddr, port);
}
*/

// old version that didn't use a timeout
//fhgfs_bool _StandardSocket_connectByIP(Socket* this, fhgfs_in_addr* ipaddress, unsigned short port)
//{
//   // note: does not set the family type to the one of this socket.
//
//   StandardSocket* thisCast = (StandardSocket*)this;
//
//   mm_segment_t oldfs;
//   int connRes;
//
//   struct sockaddr_in serveraddr =
//   {
//      .sin_family = AF_INET, //sockDomain;
//      .sin_addr.s_addr = *ipaddress, // apply server ip address
//      .sin_port = Serialization_htonsTrans(port), // apply server port
//   };
//
//
//   ACQUIRE_PROCESS_CONTEXT(oldfs);
//
//   connRes = thisCast->sock->ops->connect(
//      thisCast->sock,
//      (struct sockaddr*) &serveraddr,
//      sizeof(serveraddr),
//      0);
//
//   RELEASE_PROCESS_CONTEXT(oldfs);
//
//   if(connRes)
//   {
//      // note: this message would flood the log if hosts are unreachable on the primary interface
//
//      //char* ipStr = SocketTk_ipaddrToStr(ipaddress);
//      //printk_fhgfs(KERN_WARNING, "Failed to connect to %s. ErrCode: %d\n", ipStr, connRes);
//      //kfree(ipStr);
//
//      return fhgfs_false;
//   }
//
//   // connected
//
//   // set peername if not done so already (e.g. by connect(hostname) )
//   if(!this->peername)
//      this->peername = SocketTk_endpointAddrToString(ipaddress, port);
//
//   return fhgfs_true;
//}

fhgfs_bool _StandardSocket_connectByIP(Socket* this, fhgfs_in_addr* ipaddress, unsigned short port)
{
   // note: does not set the family type to the one of this socket (would lead to problems with SDP)

   // note: this might look a bit strange (it's kept similar to the c++ version)

   // note: error messages here would flood the log if hosts are unreachable on primary interface


   const int timeoutMS = STANDARDSOCKET_CONNECT_TIMEOUT_MS;

   StandardSocket* thisCast = (StandardSocket*)this;

   mm_segment_t oldfs;
   int connRes;

   struct sockaddr_in serveraddr =
   {
      .sin_family = AF_INET, //sockDomain;
      .sin_addr.s_addr = *ipaddress, // apply server ip address
      .sin_port = Serialization_htonsTrans(port), // apply server port
   };


   ACQUIRE_PROCESS_CONTEXT(oldfs);

   connRes = thisCast->sock->ops->connect(
      thisCast->sock,
      (struct sockaddr*) &serveraddr,
      sizeof(serveraddr),
      O_NONBLOCK); // non-blocking connect

   RELEASE_PROCESS_CONTEXT(oldfs);

   if(connRes)
   {
      if(connRes == -EINPROGRESS)
      { // wait for "ready to send data"
         struct PollSocket pollStruct = {thisCast, BEEGFS_POLLOUT, 0};
         int pollRes = SocketTk_pollSingle(&pollStruct, timeoutMS);

         if(pollRes > 0)
         { // we got something (could also be an error)

            /* note: it's important to test ERR/HUP/NVAL here instead of POLLOUT only, because
               POLLOUT and POLLERR can be returned together. */

            if(pollStruct.revents & (POLLERR | POLLHUP | POLLNVAL) )
               return fhgfs_false;

            // connection successfully established

            if(!this->peername)
            {
               this->peername = SocketTk_endpointAddrToString(ipaddress, port);
               this->peerIP = *ipaddress;
            }

            return fhgfs_true;
         }
         else
         if(!pollRes)
            return fhgfs_false; // timeout
         else
            return fhgfs_false; // connection error

      } // end of "EINPROGRESS"
   }
   else
   { // connected immediately

      // set peername if not done so already (e.g. by connect(hostname) )
      if(!this->peername)
      {
         this->peername = SocketTk_endpointAddrToString(ipaddress, port);
         this->peerIP = *ipaddress;
      }

      return fhgfs_true;
   }

   return fhgfs_false;
}


fhgfs_bool _StandardSocket_bindToAddr(Socket* this, fhgfs_in_addr* ipaddress, unsigned short port)
{
   StandardSocket* thisCast = (StandardSocket*)this;

   struct sockaddr_in bindAddr;
   int bindRes;

   size_t peernameLen;

   bindAddr.sin_family = thisCast->sockDomain;
   bindAddr.sin_addr.s_addr = *ipaddress;
   bindAddr.sin_port = Serialization_htonsTrans(port); // apply server port

   bindRes = thisCast->sock->ops->bind(
      thisCast->sock,
      (struct sockaddr*)&bindAddr,
      sizeof(bindAddr) );

   if(bindRes)
   {
      printk_fhgfs(KERN_WARNING, "Failed to bind socket. ErrCode: %d\n", bindRes);
      return fhgfs_false;
   }


   // 16 chars text + 8 (include max port len + colon + terminating zero)
   peernameLen = 16 + 8;
   this->peername = os_kmalloc(peernameLen);
   snprintf(this->peername, peernameLen, "Listen(Port: %u)", (unsigned)port);

   return fhgfs_true;
}

fhgfs_bool _StandardSocket_listen(Socket* this)
{
   StandardSocket* thisCast = (StandardSocket*)this;

   int listenRes;

   listenRes = thisCast->sock->ops->listen(thisCast->sock, SOCKET_LISTEN_BACKLOG);
   if(listenRes)
   {
      printk_fhgfs(KERN_WARNING, "Failed to set socket to listening mode. ErrCode: %d\n",
         listenRes);
      return fhgfs_false;
   }

   return fhgfs_true;
}

fhgfs_bool _StandardSocket_shutdown(Socket* this)
{
   StandardSocket* thisCast = (StandardSocket*)this;

   int sendshutRes;
   mm_segment_t oldfs;

   ACQUIRE_PROCESS_CONTEXT(oldfs);

   sendshutRes = thisCast->sock->ops->shutdown(thisCast->sock, SEND_SHUTDOWN);

   RELEASE_PROCESS_CONTEXT(oldfs);

   if( (sendshutRes < 0) && (sendshutRes != -ENOTCONN) )
   {
      printk_fhgfs(KERN_WARNING, "Failed to send shutdown. ErrCode: %d\n", sendshutRes);
      return fhgfs_false;
   }

   return fhgfs_true;
}

fhgfs_bool _StandardSocket_shutdownAndRecvDisconnect(Socket* this, int timeoutMS)
{
   fhgfs_bool shutRes;
   char buf[SOCKET_SHUTDOWN_RECV_BUF_LEN];
   int recvRes;
   mm_segment_t oldfs;


   shutRes = this->shutdown(this);;
   if(!shutRes)
      return fhgfs_false;

   ACQUIRE_PROCESS_CONTEXT(oldfs);

   // receive until shutdown arrives
   do
   {
      recvRes = this->recvT(this, buf, SOCKET_SHUTDOWN_RECV_BUF_LEN, 0, timeoutMS);
   } while(recvRes > 0);

   RELEASE_PROCESS_CONTEXT(oldfs);

   if(recvRes &&
      (recvRes != -ECONNRESET) )
   { // error occurred (but we're disconnecting, so we don't really care about errors)
      //printk_fhgfs_debug(KERN_WARNING, "Error during connection shutdown "
      //   "(problaby irrelevant). ErrCode: %d\n", recvRes);
      return fhgfs_false;
   }

   return fhgfs_true;
}


ssize_t _StandardSocket_recv(Socket* this, void *buf, size_t len, int flags)
{
   StandardSocket* thisCast = (StandardSocket*)this;

   return StandardSocket_recvfrom(thisCast, buf, len, flags, NULL);
}

/**
 * @return -ETIMEDOUT on timeout
 */
ssize_t _StandardSocket_recvT(Socket* this, void *buf, size_t len, int flags, int timeoutMS)
{
   StandardSocket* thisCast = (StandardSocket*)this;

   return StandardSocket_recvfromT(thisCast, buf, len, flags, NULL, timeoutMS);
}

ssize_t _StandardSocket_sendto(Socket* this, const void *buf, size_t len, int flags,
   fhgfs_sockaddr_in *to)
{
   StandardSocket* thisCast = (StandardSocket*)this;

   int sendRes;
   mm_segment_t oldfs;

   struct sockaddr_in toSockAddr;

   struct iovec iov =
   {
      .iov_base = (void __user *)buf,
      .iov_len  = len,
   };

   struct msghdr msg =
   {
      .msg_control      = NULL,
      .msg_controllen   = 0,
      .msg_flags        = flags | MSG_NOSIGNAL,
      .msg_name         = (struct sockaddr*)(to ? &toSockAddr : NULL),
      .msg_namelen      = sizeof(toSockAddr),
   };

   #ifndef KERNEL_HAS_MSGHDR_ITER
      msg.msg_iov       = &iov;
      msg.msg_iovlen    = 1;
   #else
      iov_iter_init(&msg.msg_iter, WRITE, &iov, 1, len);
   #endif // LINUX_VERSION_CODE

   if(to)
   {
      toSockAddr.sin_family       = thisCast->sockDomain;
      toSockAddr.sin_addr.s_addr  = to->addr;
      toSockAddr.sin_port         = to->port;
   }

   ACQUIRE_PROCESS_CONTEXT(oldfs);

#ifndef KERNEL_HAS_SOCK_SENDMSG_NOLEN
   sendRes = sock_sendmsg(thisCast->sock, &msg, len);
#else
   sendRes = sock_sendmsg(thisCast->sock, &msg);
#endif

   RELEASE_PROCESS_CONTEXT(oldfs);

   return sendRes;
}

ssize_t _StandardSocket_send(Socket* this, const void *buf, size_t len, int flags)
{
   return _StandardSocket_sendto(this, buf, len, flags, NULL);
}

ssize_t StandardSocket_sendtoIP(StandardSocket* this, const void *buf, size_t len, int flags,
   fhgfs_in_addr ipAddr, unsigned short port)
{
   Socket* thisBase = (Socket*)this;

   fhgfs_sockaddr_in peerAddr =
   {
      .addr = ipAddr,
      .port = Serialization_htonsTrans(port),
   };

   return thisBase->sendto(thisBase, buf, len, flags, &peerAddr);
}

ssize_t StandardSocket_recvfrom(StandardSocket* this, void *buf, size_t len, int flags,
   fhgfs_sockaddr_in *from)
{
   int recvRes;
   mm_segment_t oldfs;

   struct sockaddr_in fromSockAddr;

   struct iovec iov =
   {
      .iov_base = buf,
      .iov_len  = len,
   };

   struct msghdr msg =
   {
      .msg_control      = NULL,
      .msg_controllen   = 0,
      .msg_flags        = flags,
      .msg_name         = (struct sockaddr*)&fromSockAddr,
      .msg_namelen      = sizeof(fromSockAddr),
   };

   #ifndef KERNEL_HAS_MSGHDR_ITER
      msg.msg_iov       = &iov;
      msg.msg_iovlen    = 1;
   #else
      iov_iter_init(&msg.msg_iter, READ, &iov, 1, len);
   #endif // LINUX_VERSION_CODE

   ACQUIRE_PROCESS_CONTEXT(oldfs);

#ifdef KERNEL_HAS_RECVMSG_SIZE
   recvRes = sock_recvmsg(this->sock, &msg, len, flags);
#else
   recvRes = sock_recvmsg(this->sock, &msg, flags);
#endif

   RELEASE_PROCESS_CONTEXT(oldfs);


   if(from)
   {
      from->addr = fromSockAddr.sin_addr.s_addr;
      from->port = fromSockAddr.sin_port;
   }

   return recvRes;
}

/**
 * @return -ETIMEDOUT on timeout
 */
ssize_t StandardSocket_recvfromT(StandardSocket* this, void *buf, size_t len, int flags,
   fhgfs_sockaddr_in *from, int timeoutMS)
{
   Socket* thisBase = (Socket*)this;

   int pollRes;
   PollSocket pollStruct = {this, POLLIN, 0};

   pollRes = SocketTk_pollSingle(&pollStruct, timeoutMS);

   if( (pollRes > 0) && (pollStruct.revents & POLLIN) )
   {
      int recvRes = StandardSocket_recvfrom(this, buf, len, flags, from);
      return recvRes;
   }
   else
   if(!pollRes)
   { // timeout
      //printk_fhgfs(KERN_INFO, "Receive from %s timed out\n", this->peername);
      return -ETIMEDOUT;
   }
   else
   if(pollStruct.revents & POLLERR)
   {
      printk_fhgfs_debug(KERN_DEBUG, "StandardSocket_recvfromT: poll(): %s: Error condition\n",
         thisBase->peername);
   }
   else
   if(pollStruct.revents & POLLHUP)
   {
      printk_fhgfs_debug(KERN_DEBUG, "StandardSocket_recvfromT: poll(): %s: Hung up\n",
         thisBase->peername);
   }
   else
   if(pollStruct.revents & POLLNVAL)
   {
      printk_fhgfs(KERN_DEBUG, "StandardSocket_recvfromT: poll(): %s: Invalid request\n",
         thisBase->peername);
   }
   else
   {
      printk_fhgfs(KERN_DEBUG, "StandardSocket_recvfromT: poll(): %s: ErrCode: %d\n",
         thisBase->peername, pollRes);
   }


   return -ECOMM;
}

ssize_t StandardSocket_broadcast(StandardSocket* this, void *buf, size_t len, int flags,
   fhgfs_in_addr* broadcastIP, unsigned short port)
{
   Socket* thisBase = (Socket*)this;

   struct fhgfs_sockaddr_in broadcastAddr =
   {
      .addr = *broadcastIP,
      //broadcastAddr.sin_addr = in_aton("255.255.255.255");
      //broadcastAddr.sin_addr = Serialization_htonsTrans(INADDR_BROADCAST);
      .port = Serialization_htonsTrans(port),
   };

   return thisBase->sendto(thisBase, buf, len, flags, &broadcastAddr);
}

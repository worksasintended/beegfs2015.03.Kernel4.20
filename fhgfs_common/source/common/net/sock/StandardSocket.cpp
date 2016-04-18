#include <common/app/AbstractApp.h>
#include <common/app/log/LogContext.h>
#include <common/system/System.h>
#include <common/threading/PThread.h>
#include <common/toolkit/StringTk.h>
#include "StandardSocket.h"

#include <sys/epoll.h>


#define STANDARDSOCKET_CONNECT_TIMEOUT_MS    5000


/**
 * @throw SocketException
 */
StandardSocket::StandardSocket(int domain, int type, int protocol)
{
   this->sockDomain = domain;

   if(domain == PF_SDP)
      this->sockType = NICADDRTYPE_SDP;

   this->sock = ::socket(domain, type, protocol);
   if(sock == -1)
   {
      throw SocketException(std::string("Error during socket creation: ") +
         System::getErrString() );
   }

   this->epollFD = epoll_create(1); // "1" is just a hint (and is actually ignored)
   if(epollFD == -1)
   {
      int sysErr = errno;
      close(sock);

      throw SocketException(std::string("Error during epoll_create(): ") +
         System::getErrString(sysErr) );
   }

   struct epoll_event epollEvent;
   epollEvent.events = EPOLLIN;
   epollEvent.data.ptr = NULL;

   if(epoll_ctl(epollFD, EPOLL_CTL_ADD, sock, &epollEvent) == -1)
   {
      int sysErr = errno;
      close(sock);
      close(epollFD);

      throw SocketException(std::string("Unable to add sock to epoll set: ") +
         System::getErrString(sysErr) );
   }

}

/**
 * Note: To be used by accept() or createSocketPair() only.
 *
 * @param fd will be closed by the destructor of this object
 * @throw SocketException in case epoll_create fails, the caller will need to close the
 * corresponding socket file descriptor (fd)
 */
StandardSocket::StandardSocket(int fd, unsigned short sockDomain, struct in_addr peerIP,
   std::string peername)
{
   this->sock = fd;
   this->sockDomain = sockDomain;
   this->peerIP = peerIP;
   this->peername = peername;

   if(sockDomain == PF_SDP)
      this->sockType = NICADDRTYPE_SDP;

   this->epollFD = epoll_create(1); // "1" is just a hint (and is actually ignored)
   if(epollFD == -1)
   {
      throw SocketException(std::string("Error during epoll_create(): ") +
         System::getErrString() );
   }

   struct epoll_event epollEvent;
   epollEvent.events = EPOLLIN;
   epollEvent.data.ptr = NULL;

   if(epoll_ctl(epollFD, EPOLL_CTL_ADD, sock, &epollEvent) == -1)
   {
      close(epollFD);

      throw SocketException(std::string("Unable to add sock to epoll set: ") +
         System::getErrString() );
   }

}


StandardSocket::~StandardSocket()
{
   if(this->epollFD != -1)
      close(this->epollFD);

   close(this->sock);
}

/**
 * @throw SocketException
 */
void StandardSocket::createSocketPair(int domain, int type, int protocol,
   StandardSocket** outEndpointA, StandardSocket** outEndpointB)
{
   int socket_vector[2];
   struct in_addr loopbackIP = {INADDR_LOOPBACK};

   int pairRes = socketpair(domain, type, protocol, socket_vector);

   if(pairRes == -1)
   {
      throw SocketConnectException(
         std::string("Unable to create local SocketPair. SysErr: ") + System::getErrString() );
   }

   *outEndpointA = NULL;
   *outEndpointB = NULL;

   try
   {
      *outEndpointA = new StandardSocket(socket_vector[0], domain, loopbackIP,
         std::string("Localhost:PeerFD#") + StringTk::intToStr(socket_vector[0]) );
      *outEndpointB = new StandardSocket(socket_vector[1], domain, loopbackIP,
         std::string("Localhost:PeerFD#") + StringTk::intToStr(socket_vector[1]) );
   }
   catch(SocketException& e)
   {
      if(*outEndpointA)
         delete(*outEndpointA);
      else
         close(socket_vector[0]);

      if(*outEndpointB)
         delete(*outEndpointB);
      else
         close(socket_vector[1]);

      throw;
   }
}

/**
 * @throw SocketException
 */
void StandardSocket::connect(const char* hostname, unsigned short port)
{
   Socket::connect(hostname, port, sockDomain, SOCK_STREAM);
}

// old method that didn't have a timeout
//void StandardSocket::connect(const struct sockaddr* serv_addr, socklen_t addrlen)
//{
//   // set peername if not done so already (e.g. by connect(hostname) )
//   if(!peername.length() )
//   {
//      peername = Socket::ipaddrToStr(&( (struct sockaddr_in*)serv_addr)->sin_addr );
//      peername += std::string(":") +
//         StringTk::intToStr(ntohs(( (struct sockaddr_in*)serv_addr)->sin_port) );
//   }
//
//   int connRes = ::connect(sock, serv_addr, addrlen);
//   if(connRes)
//      throw SocketConnectException(
//         std::string("Unable to connect to: ") + std::string(peername) +
//         std::string(". SysErr: ") + System::getErrString() );
//}

/**
 * @throw SocketException
 */
void StandardSocket::connect(const struct sockaddr* serv_addr, socklen_t addrlen)
{
   const int timeoutMS = STANDARDSOCKET_CONNECT_TIMEOUT_MS;

   unsigned short peerPort = ntohs(( (struct sockaddr_in*)serv_addr)->sin_port);

   this->peerIP = ( (struct sockaddr_in*)serv_addr)->sin_addr;

   // set peername if not done so already (e.g. by connect(hostname) )

   if(peername.empty() )
      peername = Socket::ipaddrToStr(&peerIP) + ":" + StringTk::intToStr(peerPort);


   int flagsOrig = fcntl(sock, F_GETFL, 0);
   fcntl(sock, F_SETFL, flagsOrig | O_NONBLOCK); // make the socket nonblocking

   int connRes = ::connect(sock, serv_addr, addrlen);
   if(connRes)
   {
      if(errno == EINPROGRESS)
      { // wait for "ready to send data"
         struct pollfd pollStruct = {sock, POLLOUT, 0};
         int pollRes = poll(&pollStruct, 1, timeoutMS);

         if(pollRes > 0)
         { // we got something (could also be an error)

            /* note: it's important to test ERR/HUP/NVAL here instead of POLLOUT only, because
               POLLOUT and POLLERR can be returned together. */

            if(pollStruct.revents & (POLLERR | POLLHUP | POLLNVAL) )
               throw SocketConnectException(
                  std::string("Unable to establish connection: ") + std::string(peername) );

            // connection successfully established

            fcntl(sock, F_SETFL, flagsOrig);  // set socket back to original mode
            return;
         }
         else
         if(!pollRes)
            throw SocketConnectException(
               std::string("Timeout connecting to: ") + std::string(peername) );
         else
            throw SocketConnectException(
               std::string("Error connecting to: ") + std::string(peername) + ". "
               "SysErr: " + System::getErrString() );

      }
   }
   else
   { // immediate connect => strange, but okay...
      fcntl(sock, F_SETFL, flagsOrig);  // set socket back to original mode
      return;
   }

   throw SocketConnectException(
      std::string("Unable to connect to: ") + std::string(peername) +
      std::string(". SysErr: ") + System::getErrString() );
}


/**
 * @throw SocketException
 */
void StandardSocket::bindToAddr(in_addr_t ipAddr, unsigned short port)
{
   struct sockaddr_in localaddr_in;
   memset(&localaddr_in, 0, sizeof(localaddr_in) );

   localaddr_in.sin_family = sockDomain;
   localaddr_in.sin_addr.s_addr = ipAddr;
   localaddr_in.sin_port = htons(port);

   int bindRes = ::bind(sock, (struct sockaddr *)&localaddr_in, sizeof(localaddr_in) );
   if (bindRes == -1)
      throw SocketException("Unable to bind to port: " + StringTk::uintToStr(port) +
         ". SysErr: " + System::getErrString() );

   peername = std::string("Listen(Port: ") + StringTk::uintToStr(port) + std::string(")");
}

/**
 * @throw SocketException
 */
void StandardSocket::listen()
{
   unsigned backlog = 16;

   if(PThread::getCurrentThreadApp() )
      backlog = PThread::getCurrentThreadApp()->getCommonConfig()->getConnBacklogTCP();

   int listenRes = ::listen(sock, backlog);
   if(listenRes == -1)
      throw SocketException(std::string("listen: ") + System::getErrString() );

   if(this->epollFD != -1)
   { // we won't need epoll for listening sockets (we use it only for recvT/recvfromT)
      close(this->epollFD);
      this->epollFD = -1;
   }

}

/**
 * @throw SocketException
 */
Socket* StandardSocket::accept(struct sockaddr *addr, socklen_t *addrlen)
{
   int acceptRes = ::accept(sock, addr, addrlen);
   if(acceptRes == -1)
   {
      throw SocketException(std::string("Error during socket accept(): ") +
         System::getErrString() );
   }

   // prepare new socket object
   struct in_addr acceptIP = ( (struct sockaddr_in*)addr)->sin_addr;
   unsigned short acceptPort = ntohs( ( (struct sockaddr_in*)addr)->sin_port);

   std::string acceptPeername = endpointAddrToString(&acceptIP, acceptPort);

   try
   {
      Socket* acceptedSock = new StandardSocket(acceptRes, sockDomain, acceptIP, acceptPeername);

      return acceptedSock;
   }
   catch(SocketException& e)
   {
      close(acceptRes);
      throw;
   }
}

/**
 * @throw SocketException
 */
void StandardSocket::shutdown()
{
   int shutRes = ::shutdown(sock, SHUT_WR);
   if(shutRes == -1)
   {
      throw SocketException(std::string("Error during socket shutdown(): ") +
         System::getErrString() );
   }
}

/**
 * @throw SocketException
 */
void StandardSocket::shutdownAndRecvDisconnect(int timeoutMS)
{
   this->shutdown();

   try
   {
      // receive until shutdown arrives
      char buf[128];
      int recvRes;
      do
      {
         recvRes = recvT(buf, sizeof(buf), 0, timeoutMS);
      } while(recvRes > 0);
   }
   catch(SocketDisconnectException& e)
   {
      // just a normal thing to happen when we shutdown :)
   }

}

/**
 * Note: This is a synchronous (blocking) version
 *
 * @throw SocketException
 */
ssize_t StandardSocket::send(const void *buf, size_t len, int flags)
{
   ssize_t sendRes = ::send(sock, buf, len, flags | MSG_NOSIGNAL);
   if(sendRes == (ssize_t)len)
   {
      stats->incVals.netSendBytes += len;
      return sendRes;
   }
   else
   if(sendRes != -1)
   {
      throw SocketException(
         std::string("send(): Sent only ") + StringTk::int64ToStr(sendRes) +
         std::string(" bytes of the requested ") + StringTk::int64ToStr(len) +
         std::string(" bytes of data") );
   }

   throw SocketDisconnectException(
      "Disconnect during send() to: " + peername + "; "
      "SysErr: " + System::getErrString() );
}

// asynchronous version (with synchronous behavior)
//StandardSocket::ssize_t send(const void *buf, size_t len, int flags)
//      {
//         ssize_t bufPos = 0;
//
//         do
//         {
//            struct pollfd pollStruct = {sock, POLLOUT, 0};
//            int pollRes = poll(&pollStruct, 1, -1);
//
//            if( (pollRes > 0) && (pollStruct.revents & POLLOUT) )
//            {
//               ssize_t toBeSent = len - bufPos;
//
//               ssize_t sendRes = ::send(
//                  sock, &((char*)buf)[bufPos], toBeSent, flags | MSG_DONTWAIT | MSG_NOSIGNAL);
//
//               if(sendRes == toBeSent)
//               {
//                  stats->netSendBytes += sendRes;
//                  return len;
//               }
//               else
//               if(sendRes > 0)
//               {
//                  bufPos += sendRes;
//                  stats->netSendBytes += sendRes;
//                  continue;
//               }
//               else
//               if(errno == EAGAIN)
//               {
//                  continue;
//               }
//
//               throw SocketDisconnectException(std::string("send(): Hard Disconnect from ") +
//                  peername + ". SysErr: " + System::getErrString() );
//            }
//            else
//            if(!pollRes)
//               throw SocketTimeoutException(std::string("Send to ") + peername + " timed out");
//            else
//            if(pollStruct.revents & POLLERR)
//               throw SocketException(
//                  std::string("send(): poll(): ") + peername + ": Error condition");
//            else
//            if(pollStruct.revents & POLLHUP)
//               throw SocketException(
//                  std::string("send(): poll(): ") + peername + ": Hung up");
//            else
//            if(pollStruct.revents & POLLNVAL)
//               throw SocketException(
//                  std::string("send(): poll(): ") + peername + ": Invalid request/fd");
//            else
//            if(errno == EINTR)
//               throw SocketInterruptedPollException(
//                  std::string("send(): poll(): ") + peername + ": " + System::getErrString() );
//            else
//               throw SocketException(
//                  std::string("send(): poll(): ") + peername + ": " + System::getErrString() );
//
//         } while(true);
//
//      }

/**
 * Note: ENETUNREACH (unreachable network) errors will be silenty discarded and not be returned to
 * the caller.
 *
 * @throw SocketException
 */
ssize_t StandardSocket::sendto(const void *buf, size_t len, int flags,
   const struct sockaddr *to, socklen_t tolen)
{
   const char* logContext = "StandardSocket::sendto";

   ssize_t sendRes = ::sendto(sock, buf, len, flags | MSG_NOSIGNAL, to, tolen);
   if(sendRes == (ssize_t)len)
   {
      stats->incVals.netSendBytes += len;
      return sendRes;
   }
   else
   if(sendRes != -1)
   {
      throw SocketException(
         std::string("send(): Sent only ") + StringTk::int64ToStr(sendRes) +
         std::string(" bytes of the requested ") + StringTk::int64ToStr(len) +
         std::string(" bytes of data") );
   }

   int errCode = errno;

   std::string toStr;
   if(to)
   {
      struct sockaddr_in* toInet = (struct sockaddr_in*)to;
      toStr = Socket::ipaddrToStr(&toInet->sin_addr) + ":" +
         StringTk::uintToStr(ntohs(toInet->sin_port) );
   }

   if(errCode == ENETUNREACH)
   {
      static bool netUnreachLogged = false; // to avoid log spamming

      if(!netUnreachLogged)
      { // log unreachable net error once
         netUnreachLogged = true;

         LogContext(logContext).log(Log_WARNING,
            "Attempted to send message to unreachable network: " + toStr + "; " +
            "peername: " + peername + "; " +
            "(This error message will be logged only once.)");
      }

      return len;
   }

   throw SocketDisconnectException("sendto(" + toStr + "): "
      "Hard Disconnect from " + peername + ": " + System::getErrString(errCode) );
}

/**
 * @throw SocketException
 */
ssize_t StandardSocket::recv(void *buf, size_t len, int flags)
{
   ssize_t recvRes = ::recv(sock, buf, len, flags);
   if(recvRes > 0)
   {
      stats->incVals.netRecvBytes += recvRes;
      return recvRes;
   }

   if(recvRes == 0)
      throw SocketDisconnectException(std::string("Soft disconnect from ") + peername);
   else
      throw SocketDisconnectException(std::string("Recv(): Hard disconnect from ") +
         peername + ". SysErr: " + System::getErrString() );
}


// version with MSG_DONTWAIT flag (without poll)
//ssize_t StandardSocket::recvT(void *buf, size_t len, int flags, int timeoutMS)
//       {
//         Time startTime;
//
//         do
//         {
//            ssize_t recvRes = ::recv(sock, buf, len, flags | MSG_DONTWAIT);
//            if(recvRes > 0)
//               return recvRes;
//
//            if(recvRes == 0)
//               throw SocketDisconnectException(std::string("Soft Disconnect from ") + peername);
//
//            if(errno == EAGAIN)
//            {
//               if(startTime.elapsedMS() < timeoutMS)
//               {
//                  sched_yield();
//                  continue;
//               }
//
//               throw SocketTimeoutException(std::string("Receive from ") + peername + " timed out");
//            }
//
//            throw SocketDisconnectException(std::string("Recv(): Hard Disconnect from ") +
//               peername + ". SysErr: " + System::getErrString() );
//
//         } while(true);
//
//       }

// version with manual non-blocking mode
//ssize_t StandardSocket::recvT(void *buf, size_t len, int flags, int timeoutMS)
//       {
//         int flagsOrig = fcntl(sock, F_GETFL);
//
//         int setNonBlockRes = fcntl(sock, F_SETFL, flagsOrig | O_NONBLOCK);
//         if(setNonBlockRes == -1)
//         {
//            throw SocketException("Failed to switch to non-blocking mode: " + peername +
//               std::string(". SysErr: ") + System::getErrString() );
//         }
//
//         Time startTime;
//
//         do
//         {
//            ssize_t recvRes = ::recv(sock, buf, len, flags);
//            if(recvRes > 0)
//            {
//               fcntl(sock, F_SETFL, flagsOrig); // set back to original flags
//               return recvRes;
//            }
//
//            if(recvRes == 0)
//            {
//               fcntl(sock, F_SETFL, flagsOrig); // set back to original flags
//               throw SocketDisconnectException(std::string("Soft Disconnect from ") + peername);
//            }
//
//            if(errno == EAGAIN)
//            {
//               if(startTime.elapsedMS() < timeoutMS)
//               {
//                  sched_yield();
//                  continue;
//               }
//
//               fcntl(sock, F_SETFL, flagsOrig); // set back to original flags
//               throw SocketTimeoutException(std::string("Receive from ") + peername + " timed out");
//            }
//
//            fcntl(sock, F_SETFL, flagsOrig); // set back to original flags
//            throw SocketDisconnectException(std::string("Recv(): Hard Disconnect from ") +
//               peername + ". SysErr: " + System::getErrString() );
//
//         } while(true);
//
//       }

///**
// * Note: This is the default version, using poll only => see man pages of select(2) bugs section
// */
//ssize_t StandardSocket::recvT(void *buf, size_t len, int flags, int timeoutMS)
//{
//   struct pollfd pollStruct = {sock, POLLIN, 0};
//   int pollRes = poll(&pollStruct, 1, timeoutMS);
//
//   if( (pollRes > 0) && (pollStruct.revents & POLLIN) )
//   {
//      int recvRes = this->recv(buf, len, flags);
//      return recvRes;
//   }
//   else
//   if(!pollRes)
//      throw SocketTimeoutException(std::string("Receive from ") + peername + " timed out");
//   else
//   if(pollStruct.revents & POLLERR)
//      throw SocketException(
//         std::string("recvT(): poll(): ") + peername + ": Error condition");
//   else
//   if(pollStruct.revents & POLLHUP)
//      throw SocketException(
//         std::string("recvT(): poll(): ") + peername + ": Hung up");
//   else
//   if(pollStruct.revents & POLLNVAL)
//      throw SocketException(
//         std::string("recvT(): poll(): ") + peername + ": Invalid request/fd");
//   else
//   if(errno == EINTR)
//      throw SocketInterruptedPollException(
//         std::string("recvT(): poll(): ") + peername + ": " + System::getErrString() );
//   else
//      throw SocketException(
//         std::string("recvT(): poll(): ") + peername + ": " + System::getErrString() );
//}

/**
 * Note: This is the default version, using epoll only => see man pages of select(2) bugs section
 *
 * @throw SocketException
 */
ssize_t StandardSocket::recvT(void *buf, size_t len, int flags, int timeoutMS)
{
   struct epoll_event epollEvent;

   while (1)
   {
      int epollRes = epoll_wait(epollFD, &epollEvent, 1, timeoutMS);

      if(likely( (epollRes > 0) && (epollEvent.events & EPOLLIN) ) )
      {
         int recvRes = this->recv(buf, len, flags);
         return recvRes;
      }
      else
      if(!epollRes)
      {
         throw SocketTimeoutException("Receive timed out from: " + peername);
         break;
      }
      else
      if(errno == EINTR)
         continue; // retry, probably interrupted by gdb
      else
      if(epollEvent.events & EPOLLHUP)
      {
         throw SocketException(
            "recvT(" + peername + "); Error: Hung up");
         break;
      }
      else
      if(epollEvent.events & EPOLLERR)
      {
         throw SocketException(
            "recvT(" + peername + "); Error condition flag set");
         break;
      }
      else
      {
         throw SocketException(
            "recvT(" + peername + "); SysErr: " + System::getErrString() );
         break;
      }
   }
}


// version combining poll and MSG_DONTWAIT
//ssize_t StandardSocket::recvT(void *buf, size_t len, int flags, int timeoutMS)
//      {
//         do
//         {
//            struct pollfd pollStruct = {sock, POLLIN, 0};
//            int pollRes = poll(&pollStruct, 1, timeoutMS);
//
//            if( (pollRes > 0) && (pollStruct.revents & POLLIN) )
//            {
//               ssize_t recvRes = ::recv(sock, buf, len, flags | MSG_DONTWAIT);
//
//               if(recvRes > 0)
//               {
//                  stats->netRecvBytes += recvRes;
//                  return recvRes;
//               }
//               else
//               if(recvRes == 0)
//               {
//                  throw SocketDisconnectException(std::string("recvT(): Soft Disconnect from ") +
//                     peername);
//               }
//               else
//               if(errno == EAGAIN)
//               {
//                  continue;
//               }
//
//               throw SocketDisconnectException(std::string("recvT(): Hard Disconnect from ") +
//                  peername + ". SysErr: " + System::getErrString() );
//            }
//            else
//            if(!pollRes)
//               throw SocketTimeoutException(std::string("Receive from ") + peername + " timed out");
//            else
//            if(pollStruct.revents & POLLERR)
//               throw SocketException(
//                  std::string("recvT(): poll(): ") + peername + ": Error condition");
//            else
//            if(pollStruct.revents & POLLHUP)
//               throw SocketException(
//                  std::string("recvT(): poll(): ") + peername + ": Hung up");
//            else
//            if(pollStruct.revents & POLLNVAL)
//               throw SocketException(
//                  std::string("recvT(): poll(): ") + peername + ": Invalid request/fd");
//            else
//            if(errno == EINTR)
//               throw SocketInterruptedPollException(
//                  std::string("recvT(): poll(): ") + peername + ": " + System::getErrString() );
//            else
//               throw SocketException(
//                  std::string("recvT(): poll(): ") + peername + ": " + System::getErrString() );
//
//         } while(true);
//      }

// version with MSG_DONTWAIT-recv() before poll
//ssize_t StandardSocket::recvT(void *buf, size_t len, int flags, int timeoutMS)
//      {
//         ssize_t recvRes = ::recv(sock, buf, len, flags | MSG_DONTWAIT);
//
//         if(recvRes > 0)
//         {
//            return recvRes;
//         }
//         else
//         if(recvRes == 0)
//         {
//            throw SocketDisconnectException(std::string("recvT(): Soft Disconnect from ") +
//               peername);
//         }
//         else
//         if(errno != EAGAIN)
//         {
//            throw SocketDisconnectException(std::string("recvT(): Hard Disconnect from ") +
//               peername + ". SysErr: " + System::getErrString() );
//         }
//
//         // recv would block => use poll
//
//         do
//         {
//            struct pollfd pollStruct = {sock, POLLIN, 0};
//            int pollRes = poll(&pollStruct, 1, timeoutMS);
//
//            if( (pollRes > 0) && (pollStruct.revents & POLLIN) )
//            {
//               ssize_t recvRes = ::recv(sock, buf, len, flags | MSG_DONTWAIT);
//
//               if(recvRes > 0)
//               {
//                  return recvRes;
//               }
//               else
//               if(recvRes == 0)
//               {
//                  throw SocketDisconnectException(std::string("recvT(): Soft Disconnect from ") +
//                     peername);
//               }
//               else
//               if(errno == EAGAIN)
//               {
//                  continue;
//               }
//
//               throw SocketDisconnectException(std::string("recvT(): Hard Disconnect from ") +
//                  peername + ". SysErr: " + System::getErrString() );
//            }
//            else
//            if(!pollRes)
//               throw SocketTimeoutException(std::string("Receive from ") + peername + " timed out");
//            else
//            if(pollStruct.revents & POLLERR)
//               throw SocketException(
//                  std::string("recvT(): poll(): ") + peername + ": Error condition");
//            else
//            if(pollStruct.revents & POLLHUP)
//               throw SocketException(
//                  std::string("recvT(): poll(): ") + peername + ": Hung up");
//            else
//            if(pollStruct.revents & POLLNVAL)
//               throw SocketException(
//                  std::string("recvT(): poll(): ") + peername + ": Invalid request/fd");
//            else
//            if(errno == EINTR)
//               throw SocketInterruptedPollException(
//                  std::string("recvT(): poll(): ") + peername + ": " + System::getErrString() );
//            else
//               throw SocketException(
//                  std::string("recvT(): poll(): ") + peername + ": " + System::getErrString() );
//
//         } while(true);
//      }

/**
 * @throw SocketException
 */
ssize_t StandardSocket::recvfrom(void  *buf, size_t len, int flags,
   struct sockaddr *from, socklen_t *fromlen)
{
   int recvRes = ::recvfrom(sock, buf, len, flags, from, fromlen);
   if(recvRes > 0)
   {
      stats->incVals.netRecvBytes += recvRes;
      return recvRes;
   }

   if(recvRes == 0)
      throw SocketDisconnectException(std::string("Soft disconnect from ") + peername);
   else
      throw SocketDisconnectException(
         std::string("Recvfrom(): Hard disconnect from ") + peername + ": " +
         System::getErrString() );
}

/**
 * This is the epoll-based version.
 *
 * @throw SocketException
 */
ssize_t StandardSocket::recvfromT(void  *buf, size_t len, int flags,
   struct sockaddr *from, socklen_t *fromlen, int timeoutMS)
{
   struct epoll_event epollEvent;

   while (1)
   {
      int epollRes = epoll_wait(epollFD, &epollEvent, 1, timeoutMS);

      if(likely( (epollRes > 0) && (epollEvent.events & EPOLLIN) ) )
      {
         int recvRes = this->recvfrom(buf, len, flags, from, fromlen);
         return recvRes;
      }
      else
      if(!epollRes)
      {
         throw SocketTimeoutException(std::string("Receive from ") + peername + " timed out");
         break;
      }
      else
      if(errno == EINTR)
      {
         continue; // retry, probably interrupted by gdb
      }
      else
      if(epollEvent.events & EPOLLHUP)
      {
         throw SocketException(
            std::string("recvfromT(): epoll(): ") + peername + ": Hung up");
         break;
      }
      else
      if(epollEvent.events & EPOLLERR)
      {
         throw SocketException(
            std::string("recvfromT(): epoll(): ") + peername + ": Error condition");
         break;
      }
      else
      {
         throw SocketException(
            std::string("recvfromT(): epoll(): ") + peername + ": " + System::getErrString() );
         break;
      }
   }
}

// poll-based version
//ssize_t StandardSocket::recvfromT(void  *buf, size_t len, int flags,
//   struct sockaddr *from, socklen_t *fromlen, int timeoutMS)
//{
//   struct pollfd pollStruct = {sock, POLLIN, 0};
//   int pollRes = poll(&pollStruct, 1, timeoutMS);
//
//   if( (pollRes > 0) && (pollStruct.revents & POLLIN) )
//   {
//      int recvRes = this->recvfrom(buf, len, flags, from, fromlen);
//      return recvRes;
//   }
//   else
//   if(!pollRes)
//      throw SocketTimeoutException(std::string("Receive from ") + peername + " timed out");
//   else
//   if(pollStruct.revents & POLLERR)
//      throw SocketException(
//         std::string("recvfromT(): poll(): ") + peername + ": Error condition");
//   else
//   if(pollStruct.revents & POLLHUP)
//      throw SocketException(
//         std::string("recvfromT(): poll(): ") + peername + ": Hung up");
//   else
//   if(pollStruct.revents & POLLNVAL)
//      throw SocketException(
//         std::string("recvfromT(): poll(): ") + peername + ": Invalid request/fd");
//   else
//   if(errno == EINTR)
//      throw SocketInterruptedPollException(
//         std::string("recvT(): poll(): ") + peername + ": " + System::getErrString() );
//   else
//      throw SocketException(
//         std::string("recvfromT(): poll(): ") + peername + ": " + System::getErrString() );
//}


/**
 * @throw SocketException
 */
ssize_t StandardSocket::broadcast(const void *buf, size_t len, int flags,
   struct in_addr* broadcastIP, unsigned short port)
{
   struct sockaddr_in broadcastAddr;

   memset(&broadcastAddr, 0, sizeof(broadcastAddr) );

   broadcastAddr.sin_family         = sockDomain;
   broadcastAddr.sin_addr           = *broadcastIP;
   //broadcastAddr.sin_addr.s_addr    = inet_addr("255.255.255.255");//htonl(INADDR_BROADCAST);
   broadcastAddr.sin_port           = htons(port);

   return this->sendto(buf, len, flags,
      (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr) );
}

/**
 * @throw SocketException
 */
void StandardSocket::setSoKeepAlive(bool enable)
{
   int keepAliveVal = (enable ? 1 : 0);

   int setRes = setsockopt(sock,
      SOL_SOCKET,
      SO_KEEPALIVE,
      (char*)&keepAliveVal,
      sizeof(keepAliveVal) );

   if(setRes == -1)
      throw SocketException(std::string("setSoKeepAlive: ") + System::getErrString() );
}

/**
 * @throw SocketException
 */
void StandardSocket::setSoBroadcast(bool enable)
{
   int broadcastVal = (enable ? 1 : 0);

   int setRes = setsockopt(sock,
      SOL_SOCKET,
      SO_BROADCAST,
      &broadcastVal,
      sizeof(broadcastVal) );

   if(setRes == -1)
      throw SocketException(std::string("setSoBroadcast: ") + System::getErrString() );
}

/**
 * @throw SocketException
 */
void StandardSocket::setSoReuseAddr(bool enable)
{
   int reuseVal = (enable ? 1 : 0);

   int setRes = setsockopt(sock,
      SOL_SOCKET,
      SO_REUSEADDR,
      &reuseVal,
      sizeof(reuseVal) );

   if(setRes == -1)
      throw SocketException(std::string("setSoReuseAddr: ") + System::getErrString() );
}

/**
 * @throw SocketException
 */
int StandardSocket::getSoRcvBuf()
{
   int rcvBufLen;
   socklen_t optlen = sizeof(rcvBufLen);

   int getRes = getsockopt(sock,
      SOL_SOCKET,
      SO_RCVBUF,
      &rcvBufLen,
      &optlen);

   if(getRes == -1)
      throw SocketException(std::string("getSoRcvBuf: ") + System::getErrString() );

   return rcvBufLen;
}

/**
 * Note: Increase only (buffer will not be set to a smaller value).
 * Note: Currently, this method never throws an exception and hence doesn't return an error. (You
 *       could use getSoRcvBuf() if you're interested in the resulting buffer size.)
 *
 * @throw SocketException
 */
void StandardSocket::setSoRcvBuf(int size)
{
   /* note: according to socket(7) man page, the value given to setsockopt() is doubled and the
      doubled value is returned by getsockopt() */

   int halfSize = size/2;
   int origBufLen = getSoRcvBuf();

   if(origBufLen >= (size) )
   { // we don't decrease buf sizes (but this is not an error)
      return;
   }

   // try without force-flag (will internally reduce to global max setting without errrors)

   int setRes = setsockopt(sock,
      SOL_SOCKET,
      SO_RCVBUF,
      &halfSize,
      sizeof(halfSize) );

   if(setRes)
      LOG_DEBUG(__func__, Log_DEBUG, std::string("setSoRcvBuf error: ") + System::getErrString() );

   // now try with force-flag (will allow root to exceed the global max setting)
   // BUT: unforunately, our suse 10 build system doesn't support the SO_RCVBUFFORCE sock opt

   /*
   setsockopt(sock,
      SOL_SOCKET,
      SO_RCVBUFFORCE,
      &halfSize,
      sizeof(halfSize) );
   */
}

/**
 * @throw SocketException
 */
void StandardSocket::setTcpNoDelay(bool enable)
{
   int noDelayVal = (enable ? 1 : 0);

   int noDelayRes = setsockopt(sock,
      IPPROTO_TCP,
      TCP_NODELAY,
      (char*)&noDelayVal,
      sizeof(noDelayVal) );

   if(noDelayRes == -1)
      throw SocketException(std::string("setTcpNoDelay: ") + System::getErrString() );
}

/**
 * @throw SocketException
 */
void StandardSocket::setTcpCork(bool enable)
{
   int corkVal = (enable ? 1 : 0);

   int setRes = setsockopt(sock,
      SOL_TCP,
      TCP_CORK,
      &corkVal,
      sizeof(corkVal) );

   if(setRes == -1)
      throw SocketException(std::string("setTcpCork: ") + System::getErrString() );
}



#ifndef SOCKET_H_
#define SOCKET_H_

#include <common/external/sdp_inet.h>
#include <common/toolkit/SocketTk.h>
#include <common/toolkit/StringTk.h>
#include <common/toolkit/Time.h>
#include <common/Common.h>
#include <common/net/sock/NicAddress.h>


#define PF_SDP                AF_INET_SDP // the Sockets Direct Protocol (Family)

#define BEEGFS_MSG_DONTWAIT    0x40  // Nonblocking IO


/*
 * This is an abstract class.
 */


struct Socket;
typedef struct Socket Socket;


extern void _Socket_init(Socket* this);
extern Socket* _Socket_construct(void);
extern void _Socket_uninit(Socket* this);
extern void _Socket_destruct(Socket* this);

extern fhgfs_bool Socket_bind(Socket* this, unsigned short port);
extern fhgfs_bool Socket_bindToAddr(Socket* this, fhgfs_in_addr* ipAddr, unsigned short port);

// getters & setters
static inline fhgfs_bool Socket_getSockValid(Socket* this);
static inline NicAddrType_t Socket_getSockType(Socket* this);
static inline char* Socket_getPeername(Socket* this);
static inline uint32_t Socket_getPeerIP(Socket* this);

// inliners
static inline void Socket_virtualDestruct(Socket* this);
static inline ssize_t Socket_recvExact(Socket* this, void *buf, size_t len, int flags);
static inline ssize_t Socket_recvExactT(Socket* this, void *buf, size_t len, int flags,
   int timeoutMS);
static inline ssize_t Socket_recvExactTEx(Socket* this, void *buf, size_t len, int flags,
   int timeoutMS, size_t* outNumReceivedBeforeError);




struct Socket
{
   NicAddrType_t sockType;
   char* peername;
   fhgfs_in_addr peerIP;

   fhgfs_bool sockValid;

   // virtual functions
   void (*uninit)(Socket* this);
   //void (*destruct)(Socket* this);

   fhgfs_bool (*connectByIP)(Socket* this, fhgfs_in_addr* ipaddress, unsigned short port);
   fhgfs_bool (*bindToAddr)(Socket* this, fhgfs_in_addr* ipaddress, unsigned short port);
   fhgfs_bool (*listen)(Socket* this);
   fhgfs_bool (*shutdown)(Socket* this);
   fhgfs_bool (*shutdownAndRecvDisconnect)(Socket* this, int timeoutMS);

   ssize_t (*send)(Socket* this, const void *buf, size_t len, int flags);
   ssize_t (*sendto)(Socket* this, const void *buf, size_t len, int flags, fhgfs_sockaddr_in *to);

   ssize_t (*recv)(Socket* this, void *buf, size_t len, int flags);
   ssize_t (*recvT)(Socket* this, void *buf, size_t len, int flags, int timeoutMS);
};


fhgfs_bool Socket_getSockValid(Socket* this)
{
   return this->sockValid;
}

NicAddrType_t Socket_getSockType(Socket* this)
{
   return this->sockType;
}

char* Socket_getPeername(Socket* this)
{
   return this->peername;
}

uint32_t Socket_getPeerIP(Socket* this)
{
   return this->peerIP;
}

/**
 * Calls the virtual uninit method and kfrees the object.
 */
void Socket_virtualDestruct(Socket* this)
{
   this->uninit(this);
   os_kfree(this);
}

ssize_t Socket_recvExact(Socket* this, void *buf, size_t len, int flags)
{
   ssize_t missing = len;
   ssize_t recvRes;

   do
   {
      recvRes = this->recv(this, &((char*)buf)[len-missing], missing, flags);
      missing -= recvRes;
   } while(missing && likely(recvRes > 0) );

   return (recvRes > 0) ? (ssize_t)len : recvRes;
}

/**
 * Receive with timeout.
 *
 * @return -ETIMEDOUT on timeout.
 */
ssize_t Socket_recvExactT(Socket* this, void *buf, size_t len, int flags, int timeoutMS)
{
   size_t numReceivedBeforeError;

   return Socket_recvExactTEx(this, buf, len, flags, timeoutMS, &numReceivedBeforeError);
}

/**
 * Receive with timeout, extended version with numReceivedBeforeError.
 *
 * note: this uses a soft timeout that is being reset after each received data packet.
 *
 * @param outNumReceivedBeforeError number of bytes received before returning (also set in case of
 * an error, e.g. timeout); given value will only be increased and is intentionally not set to 0
 * initially.
 * @return -ETIMEDOUT on timeout.
 */
ssize_t Socket_recvExactTEx(Socket* this, void *buf, size_t len, int flags, int timeoutMS,
   size_t* outNumReceivedBeforeError)
{
   ssize_t missingLen = len;
   ssize_t recvRes;

   do
   {
      recvRes = this->recvT(this, &((char*)buf)[len-missingLen], missingLen, flags, timeoutMS);

      if(unlikely(recvRes <= 0) )
         return recvRes;

      missingLen -= recvRes;
      *outNumReceivedBeforeError += recvRes;

   } while(missingLen);

   // all received if we got here

   return len;
}



#endif /*SOCKET_H_*/

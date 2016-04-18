#ifndef OPEN_RDMASOCKET_H_
#define OPEN_RDMASOCKET_H_

#include <common/toolkit/SocketTk.h>
#include <common/toolkit/StringTk.h>
#include <common/toolkit/Time.h>
#include <common/Common.h>
#include <opentk/net/sock/ibvsocket/OpenTk_IBVSocket.h>
#include <common/net/sock/PooledSocket.h>


struct RDMASocket;
typedef struct RDMASocket RDMASocket;


extern void RDMASocket_init(RDMASocket* this);
extern RDMASocket* RDMASocket_construct(void);
extern void _RDMASocket_uninit(Socket* this);
extern void _RDMASocket_destruct(Socket* this);

extern fhgfs_bool RDMASocket_rdmaDevicesExist(void);

extern fhgfs_bool _RDMASocket_connectByIP(Socket* this, fhgfs_in_addr* ipaddress,
   unsigned short port);
extern fhgfs_bool _RDMASocket_bindToAddr(Socket* this, fhgfs_in_addr* ipaddress,
   unsigned short port);
extern fhgfs_bool _RDMASocket_listen(Socket* this);
extern fhgfs_bool _RDMASocket_shutdown(Socket* this);
extern fhgfs_bool _RDMASocket_shutdownAndRecvDisconnect(Socket* this, int timeoutMS);

extern ssize_t _RDMASocket_recv(Socket* this, void *buf, size_t len, int flags);
extern ssize_t _RDMASocket_recvT(Socket* this, void *buf, size_t len, int flags,
   int timeoutMS);

extern ssize_t _RDMASocket_send(Socket* this, const void *buf, size_t len, int flags);
extern ssize_t _RDMASocket_sendto(Socket* this, const void *buf, size_t len, int flags,
   fhgfs_sockaddr_in *to);

extern fhgfs_bool  RDMASocket_checkConnection(RDMASocket* this);
extern unsigned long RDMASocket_poll(RDMASocket* this, short events, fhgfs_bool finishPoll);

// inliners
static inline fhgfs_bool RDMASocket_getSockValid(RDMASocket* this);
static inline char* RDMASocket_getPeername(RDMASocket* this);
static inline void RDMASocket_setBuffers(RDMASocket* this, unsigned bufNum, unsigned bufSize);
static inline void RDMASocket_setTypeOfService(RDMASocket* this, int typeOfService);

struct RDMASocket
{
   PooledSocket pooledSocket;

   IBVSocket* ibvsock;

   IBVCommConfig commCfg;
};


fhgfs_bool RDMASocket_getSockValid(RDMASocket* this)
{
   Socket* thisBase = (Socket*)this;

   return thisBase->sockValid;
}

char* RDMASocket_getPeername(RDMASocket* this)
{
   Socket* thisBase = (Socket*)this;

   return thisBase->peername;
}

/**
 * Note: Only has an effect for unconnected sockets.
 */
void RDMASocket_setBuffers(RDMASocket* this, unsigned bufNum, unsigned bufSize)
{
   this->commCfg.bufNum = bufNum;
   this->commCfg.bufSize = bufSize;
}

/**
 * Note: Only has an effect for unconnected sockets.
 */
void RDMASocket_setTypeOfService(RDMASocket* this, int typeOfService)
{
   IBVSocket_setTypeOfService(this->ibvsock, typeOfService);
}

#endif /*OPEN_RDMASOCKET_H_*/

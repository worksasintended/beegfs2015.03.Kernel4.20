#ifndef OPENTK_IBVSOCKET_H_
#define OPENTK_IBVSOCKET_H_

#include <opentk/OpenTk_Common.h>



/*
 * This is the interface of the ibverbs socket abstraction.
 */

struct in_addr; // forward declaration

struct IBVSocket; // forward declaration
typedef struct IBVSocket IBVSocket;

struct IBVCommConfig;
typedef struct IBVCommConfig IBVCommConfig;


// construction/destruction
extern void IBVSocket_init(IBVSocket* _this);
extern IBVSocket* IBVSocket_construct(void);
extern void IBVSocket_uninit(IBVSocket* _this);
extern void IBVSocket_destruct(IBVSocket* _this);

// static
extern fhgfs_bool IBVSocket_rdmaDevicesExist(void);

// methods
extern fhgfs_bool IBVSocket_connectByIP(IBVSocket* _this, struct in_addr* ipaddress,
   unsigned short port, IBVCommConfig* commCfg);
extern fhgfs_bool IBVSocket_bind(IBVSocket* _this, unsigned short port);
extern fhgfs_bool IBVSocket_bindToAddr(IBVSocket* _this, struct in_addr* ipAddr,
   unsigned short port);
extern fhgfs_bool IBVSocket_listen(IBVSocket* _this);
extern fhgfs_bool IBVSocket_shutdown(IBVSocket* _this);

extern ssize_t IBVSocket_recv(IBVSocket* _this, char* buf, size_t bufLen, int flags);
extern ssize_t IBVSocket_recvT(IBVSocket* _this, char* buf, size_t bufLen, int flags,
   int timeoutMS);
extern ssize_t IBVSocket_send(IBVSocket* _this, const char* buf, size_t bufLen, int flags);

extern int IBVSocket_checkConnection(IBVSocket* _this);
extern int IBVSocket_nonblockingRecvCheck(IBVSocket* _this);
extern int IBVSocket_nonblockingSendCheck(IBVSocket* _this);

extern unsigned long IBVSocket_poll(IBVSocket* _this, short events, fhgfs_bool finishPoll);

// getters & setters
extern fhgfs_bool IBVSocket_getSockValid(IBVSocket* _this);

extern void IBVSocket_setTypeOfService(IBVSocket* _this, int typeOfService);

struct IBVCommConfig
{
   unsigned bufNum; // number of available buffers
   unsigned bufSize; // size of each buffer
};


#endif /*OPENTK_IBVSOCKET_H_*/

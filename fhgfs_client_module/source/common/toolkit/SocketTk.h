#ifndef OPEN_SOCKETTK_H_
#define OPEN_SOCKETTK_H_

#include <common/Common.h>
#include <common/toolkit/Time.h>

#define BEEGFS_POLLIN       0x0001
#define BEEGFS_POLLPRI      0x0002
#define BEEGFS_POLLOUT      0x0004
#define BEEGFS_POLLERR      0x0008
#define BEEGFS_POLLHUP      0x0010
#define BEEGFS_POLLNVAL     0x0020


#define SOCKETTK_ENDPOINTSTR_LEN       24 // size for _endpointAddrToStringNoAlloc()


// forward declarations
struct StandardSocket;

struct PollSocket;
typedef struct PollSocket PollSocket;

struct PollSocketEx;
typedef struct PollSocketEx PollSocketEx;

struct ExternalHelperd;



extern fhgfs_bool SocketTk_initOnce(void);
extern void SocketTk_uninitOnce(void);

extern int SocketTk_pollSingle(PollSocket* pollSock, int timeoutMS);
extern int SocketTk_poll(PollSocket* pollSocks, size_t numPollSocks, int timeoutMS);
extern int SocketTk_pollEx(PollSocketEx* pollSocks, size_t numPollSocks, int timeoutMS);

extern fhgfs_bool SocketTk_getHostByName(struct ExternalHelperd* helperd, const char* hostname,
   fhgfs_in_addr* outIPAddr);
extern fhgfs_bool SocketTk_getHostByAddrStr(const char* hostAddr, fhgfs_in_addr* outIPAddr);
extern fhgfs_in_addr SocketTk_in_aton(const char* hostAddr);

extern fhgfs_bool SocketTk_checkCompileTimeAssumptions(void);

extern fhgfs_bool __SocketTk_pollInterruptedBySignal(void);

extern char* SocketTk_ipaddrToStr(fhgfs_in_addr* ipaddress);
extern char* SocketTk_endpointAddrToString(fhgfs_in_addr* ipaddress, unsigned short port);
extern void SocketTk_endpointAddrToStringNoAlloc(char* buf, unsigned bufLen,
   fhgfs_in_addr* ipaddress, unsigned short port);


struct PollSocket
{
   struct StandardSocket* sock;
   short events;  // requested events
   short revents; // returned events
};

struct PollSocketEx
{
   struct Socket* sock;
   short events;  // requested events
   short revents; // returned events
};


#endif /*OPEN_SOCKETTK_H_*/

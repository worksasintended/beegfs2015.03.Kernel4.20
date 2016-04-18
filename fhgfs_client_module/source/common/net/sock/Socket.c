#include <common/toolkit/SocketTk.h>
#include <common/net/sock/Socket.h>
#include <common/threading/Thread.h>

void _Socket_init(Socket* this)
{
   // clear virtual function pointers
   os_memset(this, 0, sizeof(*this) );

   this->peername = NULL;
   this->peerIP = 0;
   this->sockType = NICADDRTYPE_STANDARD;

   this->sockValid = fhgfs_false;
}

Socket* _Socket_construct(void)
{
   Socket* this = (Socket*)os_kmalloc(sizeof(*this) );

   _Socket_init(this);

   return this;
}

void _Socket_uninit(Socket* this)
{
   // free
   SAFE_KFREE(this->peername);
}

void _Socket_destruct(Socket* this)
{
   _Socket_uninit(this);

   os_kfree(this);
}


/*
fhgfs_bool Socket_connectByName(Socket* this, const char* hostname, unsigned short port,
   ExternalHelperd* helperd)
{
   size_t peernameLen;

   fhgfs_in_addr hostAddr;

   SocketTk_getHostByName(helperd, hostname, &hostAddr);
   //hostAddr.s_addr = in_aton(hostname);

   // set peername
   peernameLen = os_strlen(hostname) + 8; // 8 include max port len + colon + terminating zero
   this->peername = os_kmalloc(peernameLen);
   snprintf(this->peername, peernameLen, "%s:%u", hostname, (unsigned)port);

   // connect
   return Socket_connectByIP(this, &hostAddr, port);
}
*/

fhgfs_bool Socket_bind(Socket* this, unsigned short port)
{
   fhgfs_in_addr ipAddr;

   ipAddr = BEEGFS_INADDR_ANY;

   return this->bindToAddr(this, &ipAddr, port);
}

fhgfs_bool Socket_bindToAddr(Socket* this, fhgfs_in_addr* ipAddr, unsigned short port)
{
   return this->bindToAddr(this, ipAddr, port);
}

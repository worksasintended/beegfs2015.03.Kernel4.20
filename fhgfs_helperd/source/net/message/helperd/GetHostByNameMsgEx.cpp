#include <program/Program.h>
#include <common/net/message/helperd/GetHostByNameRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/SocketTk.h>
#include "GetHostByNameMsgEx.h"


bool GetHostByNameMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("GetHostByNameMsgEx incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername(); 
   LOG_DEBUG_CONTEXT(log, 4, std::string("Received a GetHostByNameMsg from: ") + peer);

   // note: this will send an empty string in the response when the system was unable to
   //    resolve the hostname
   
   const char* hostname = getHostname();
   struct in_addr hostIP;
   std::string hostAddrStr;
   
   bool resolveRes = SocketTk::getHostByName(hostname, &hostIP);
   if(resolveRes)
      hostAddrStr = Socket::ipaddrToStr(&hostIP);

   GetHostByNameRespMsg respMsg(hostAddrStr.c_str() );
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   return true;      
}


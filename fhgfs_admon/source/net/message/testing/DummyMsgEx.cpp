#include <program/Program.h>
#include "DummyMsgEx.h"

bool DummyMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("DummyMsg incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG_CONTEXT(log, 4, std::string("Received a DummyMsg from: ") + peer);

   DummyMsg respMsg;
   respMsg.serialize(respBuf, bufLen);

   if(fromAddr)
   { // datagram => sync via dgramLis send method
      Program::getApp()->getDatagramListener()->sendto(respBuf, respMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(*fromAddr) );
   }
   else
      sock->sendto(respBuf, respMsg.getMsgLength(), 0, NULL, 0);

   return true;
}

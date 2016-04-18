#include <program/Program.h>
#include "SetChannelDirectMsgEx.h"

bool SetChannelDirectMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("SetChannelDirect incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername(); 
   LOG_DEBUG_CONTEXT(log, 4, std::string("Received a SetChannelDirectMsg from: ") + peer);
   
   LOG_DEBUG_CONTEXT(log, 5, std::string("Value: ") + StringTk::intToStr(getValue() ) );
   
   sock->setIsDirect(getValue() );
   
   return true;
}



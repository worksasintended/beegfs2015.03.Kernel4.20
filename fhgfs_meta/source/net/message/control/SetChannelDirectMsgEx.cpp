#include <program/Program.h>
#include "SetChannelDirectMsgEx.h"

bool SetChannelDirectMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   #ifdef BEEGFS_DEBUG
      const char* logContext = "SetChannelDirect incoming";

      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, 4, std::string("Received a SetChannelDirectMsg from: ") + peer);

      LOG_DEBUG(logContext, 5, std::string("Value: ") + StringTk::intToStr(getValue() ) );
   #endif // BEEGFS_DEBUG
   
   
   sock->setIsDirect(getValue() );

   App* app = Program::getApp();
   app->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_SETCHANNELDIRECT,
      getMsgHeaderUserID() );

   // note: this message does not require a response
   
   return true;
}



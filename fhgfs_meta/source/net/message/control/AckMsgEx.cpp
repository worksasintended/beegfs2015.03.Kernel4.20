#include <program/Program.h>
#include "AckMsgEx.h"

bool AckMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   #ifdef BEEGFS_DEBUG
      const char* logContext = "Ack incoming";

      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, 4, std::string("Received a AckMsg from: ") + peer);
   #endif //BEEGFS_DEBUG
   
   LOG_DEBUG(logContext, 5, std::string("Value: ") + getValue() );
   
   AcknowledgmentStore* ackStore = Program::getApp()->getAckStore();
   ackStore->receivedAck(getValue() );

   App* app = Program::getApp();
   app->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_ACK,
      getMsgHeaderUserID() );
   
   // note: this message does not require a response

   return true;
}



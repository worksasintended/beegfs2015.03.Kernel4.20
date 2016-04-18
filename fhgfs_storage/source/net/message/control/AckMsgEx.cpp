#include <program/Program.h>
#include "AckMsgEx.h"

bool AckMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("Ack incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername(); 
   LOG_DEBUG_CONTEXT(log, 4, std::string("Received a AckMsg from: ") + peer);
   
   LOG_DEBUG_CONTEXT(log, 5, std::string("Value: ") + getValue() );
   
   AcknowledgmentStore* ackStore = Program::getApp()->getAckStore();
   ackStore->receivedAck(getValue() );
   
   // note: this message does not require a response

   App* app = Program::getApp();
   app->getNodeOpStats()->updateNodeOp(
      sock->getPeerIP(), StorageOpCounter_ACK, getMsgHeaderUserID() );

   return true;
}



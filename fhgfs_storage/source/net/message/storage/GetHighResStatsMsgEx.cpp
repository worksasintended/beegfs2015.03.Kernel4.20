#include <program/Program.h>
#include <common/net/message/storage/GetHighResStatsRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include "GetHighResStatsMsgEx.h"


bool GetHighResStatsMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   const char* logContext = "GetHighResStatsMsgEx incoming";

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG(logContext, Log_DEBUG, std::string("Received a GetHighResStatsMsg from: ") + peer);
   IGNORE_UNUSED_VARIABLE(logContext);

   HighResStatsList statsHistory;
   uint64_t lastStatsMS = getValue();

   // get stats history
   StatsCollector* statsCollector = Program::getApp()->getStatsCollector();
   statsCollector->getStatsSince(lastStatsMS, statsHistory);


   GetHighResStatsRespMsg respMsg(&statsHistory);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   return true;
}


#include <program/Program.h>
#include <common/net/message/storage/GetHighResStatsRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <common/nodes/OpCounter.h>
#include "GetClientStatsMsgEx.h"
#include <nodes/MetaNodeOpStats.h>
#include <common/net/message/nodes/GetClientStatsRespMsg.h>


/**
 * Server side gets a GetClientStatsMsgEx request
 */
bool GetClientStatsMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("GetClientStatsMsgEx incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG_CONTEXT(log, Log_DEBUG, std::string("Received a GetClientStatsMsg from: ") + peer);

   uint64_t cookieIP = getCookieIP(); // requested is cookie+1

   // get stats
   MetaNodeOpStats* opStats = Program::getApp()->getNodeOpStats();

   bool wantPerUserStats = isMsgHeaderFeatureFlagSet(GETCLIENTSTATSMSG_FLAG_PERUSERSTATS);
   UInt64Vector opStatsVec;

   opStats->mapToUInt64Vec(
      cookieIP, GETCLIENTSTATSRESP_MAX_PAYLOAD_LEN, wantPerUserStats, &opStatsVec);


   GetClientStatsRespMsg respMsg(&opStatsVec);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   return true;
}


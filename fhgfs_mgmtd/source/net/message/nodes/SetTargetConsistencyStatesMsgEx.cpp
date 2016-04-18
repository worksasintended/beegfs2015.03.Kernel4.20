#include <common/net/message/nodes/SetTargetConsistencyStatesRespMsg.h>
#include <nodes/MgmtdTargetStateStore.h>
#include <program/Program.h>

#include "SetTargetConsistencyStatesMsgEx.h"

bool SetTargetConsistencyStatesMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
      char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG("Set target states incoming", Log_DEBUG,
      "Received a SetTargetConsistencyStatesMsg from: " + peer);

   App* app = Program::getApp();
   MgmtdTargetStateStore* targetStateStore = app->getTargetStateStore();

   UInt16List targetIDs;
   UInt8List consistencyStates;

   parseTargetIDs(&targetIDs);
   parseStates(&consistencyStates);

   bool setOnline = getSetOnline();

   targetStateStore->setConsistencyStatesFromLists(targetIDs, consistencyStates, setOnline);

   FhgfsOpsErr result = FhgfsOpsErr_SUCCESS;

   SetTargetConsistencyStatesRespMsg respMsg(result);
   respMsg.serialize(respBuf, bufLen);

   // send response
   sock->sendto(respBuf, respMsg.getMsgLength(), 0, NULL, 0);

   return true;
}

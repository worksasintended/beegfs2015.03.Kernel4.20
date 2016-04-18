#include <common/net/message/nodes/SetTargetConsistencyStatesRespMsg.h>
#include <common/nodes/TargetStateStore.h>
#include <program/Program.h>

#include "SetTargetConsistencyStatesMsgEx.h"

bool SetTargetConsistencyStatesMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG("Set target states incoming", Log_DEBUG,
      "Received a SetTargetConsistencyStatesMsg from: " + peer);

   App* app = Program::getApp();
   StorageTargets* storageTargets = app->getStorageTargets();
   FhgfsOpsErr result = FhgfsOpsErr_SUCCESS;

   UInt16List targetIDs;
   UInt8List states;

   parseTargetIDs(&targetIDs);
   parseStates(&states);

   UInt16ListIter targetIDIter = targetIDs.begin();
   UInt8ListIter statesIter = states.begin();

   if (targetIDs.size() != states.size())
   {
      LogContext(__func__).logErr("Different list size of targetIDs and states");
      result = FhgfsOpsErr_INTERNAL;
      goto send_response;
   }

   for ( ; targetIDIter != targetIDs.end(); targetIDIter++, statesIter++)
   {
      bool setResp = storageTargets->setState(*targetIDIter, (TargetConsistencyState)*statesIter);
      if (!setResp)
      {
         LogContext(__func__).logErr("Unknown targetID: " + StringTk::uintToStr(*targetIDIter));
         result = FhgfsOpsErr_UNKNOWNTARGET;
      }
   }

send_response:
   SetTargetConsistencyStatesRespMsg respMsg(result);
   respMsg.serialize(respBuf, bufLen);

   // send response
   sock->sendto(respBuf, respMsg.getMsgLength(), 0, NULL, 0);

   return true;
}

#include <common/net/message/storage/GetStorageTargetInfoRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <program/Program.h>
#include "GetStorageTargetInfoMsgEx.h"


bool GetStorageTargetInfoMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   const char* logContext = "GetStorageTargetInfoMsg incoming";

   App* app = Program::getApp();
   StorageTargets* storageTargets = app->getStorageTargets();

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG(logContext, Log_DEBUG, "Received a GetStorageTargetInfoMsg from: " + peer);
   IGNORE_UNUSED_VARIABLE(logContext);

   UInt16List targetIDs;
   StorageTargetInfoList targetInfoList;

   parseTargetIDs(&targetIDs);

   if (targetIDs.empty() ) // use all known targets
      storageTargets->getAllTargetIDs(&targetIDs);

   storageTargets->generateTargetInfoList(targetIDs, targetInfoList);

   // send response

   GetStorageTargetInfoRespMsg respMsg(&targetInfoList);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );


   // update stats

   app->getNodeOpStats()->updateNodeOp(
      sock->getPeerIP(), StorageOpCounter_STATSTORAGEPATH, getMsgHeaderUserID() );

   return true;
}


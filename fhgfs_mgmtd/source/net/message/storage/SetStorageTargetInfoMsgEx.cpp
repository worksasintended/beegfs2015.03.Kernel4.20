#include <common/net/message/storage/SetStorageTargetInfoRespMsg.h>
#include <program/Program.h>

#include "SetStorageTargetInfoMsgEx.h"

bool SetStorageTargetInfoMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG("Set target info incoming", Log_DEBUG,
      "Received a SetStorageTargetInfoMsg from: " + peer);

   FhgfsOpsErr result;

   App* app = Program::getApp();
   InternodeSyncer* internodeSyncer = app->getInternodeSyncer();

   StorageTargetInfoList targetInfoList;
   parseStorageTargetInfos(&targetInfoList);

   LOG_DEBUG("Set target info incoming", Log_DEBUG,
      "Number of reports: " + StringTk::uintToStr(targetInfoList.size() ) );

   NodeType nodeType = getNodeType();
   if (nodeType != NODETYPE_Storage && nodeType != NODETYPE_Meta)
      result = FhgfsOpsErr_INVAL;
   else
   {
      internodeSyncer->storeCapacityReports(targetInfoList, nodeType);
      result = FhgfsOpsErr_SUCCESS;
   }

   SetStorageTargetInfoRespMsg respMsg(result);
   respMsg.serialize(respBuf, bufLen);

   // Send response.
   sock->sendto(respBuf, respMsg.getMsgLength(), 0, NULL, 0);

   return true;
}

#include <net/message/storage/GetStorageTargetInfoMsgEx.h>
#include "RequestStorageDataMsgEx.h"

bool RequestStorageDataMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats *stats)
{
   const char* logContext = "RequestStorageDataMsg incoming";
   
   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG(logContext, Log_DEBUG, "Received a RequestDataMsg from: " + peer);
   IGNORE_UNUSED_VARIABLE(logContext);

   App* app = Program::getApp();
   Node* node = app->getLocalNode();
   MultiWorkQueueMap* workQueueMap = app->getWorkQueueMap();
   StorageTargets* storageTargets = app->getStorageTargets();

   // get disk space of each target

   UInt16List targetIDs;
   StorageTargetInfoList storageTargetInfoList;

   storageTargets->getAllTargetIDs(&targetIDs);

   storageTargets->generateTargetInfoList(targetIDs, storageTargetInfoList);

   // compute total disk space and total free space

   int64_t diskSpaceTotal = 0; // sum of all targets
   int64_t diskSpaceFree = 0; // sum of all targets

   for(StorageTargetInfoListIter iter = storageTargetInfoList.begin();
       iter != storageTargetInfoList.end();
       iter++)
   {
      if(diskSpaceTotal == -1)
         continue; // statfs() failed on this target

      diskSpaceTotal += iter->getDiskSpaceTotal();
      diskSpaceFree += iter->getDiskSpaceFree();
   }


   unsigned sessionCount = app->getSessions()->getSize();

   NicAddressList nicList(node->getNicList());

   // highresStats
   HighResStatsList statsHistory;
   uint64_t lastStatsMS = getValue();

   // get stats history
   StatsCollector* statsCollector = app->getStatsCollector();
   statsCollector->getStatsSince(lastStatsMS, statsHistory);

   // get work queue stats
   unsigned indirectWorkListSize = 0;
   unsigned directWorkListSize = 0;

   for(MultiWorkQueueMapCIter iter = workQueueMap->begin(); iter != workQueueMap->end(); iter++)
   {
      indirectWorkListSize += iter->second->getIndirectWorkListSize();
      directWorkListSize += iter->second->getDirectWorkListSize() +
         iter->second->getMirrorWorkListSize();
   }

   RequestStorageDataRespMsg requestStorageDataRespMsg(node->getID().c_str(), node->getNumID(),
      &nicList, indirectWorkListSize, directWorkListSize, diskSpaceTotal, diskSpaceFree,
      sessionCount, &statsHistory, &storageTargetInfoList);
   requestStorageDataRespMsg.serialize(respBuf, bufLen);

   sock->sendto(respBuf, requestStorageDataRespMsg.getMsgLength(), 0, (struct sockaddr*) fromAddr,
      sizeof(struct sockaddr_in));

   LOG_DEBUG(logContext, Log_SPAM, std::string("Sent a message with type: " ) +
      StringTk::uintToStr(requestStorageDataRespMsg.getMsgType() ) + std::string(" to admon") );

   app->getNodeOpStats()->updateNodeOp(
      sock->getPeerIP(), StorageOpCounter_REQUESTSTORAGEDATA, getMsgHeaderUserID() );

   return true;
}

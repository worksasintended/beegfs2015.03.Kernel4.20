#include "RequestStorageDataWork.h"
#include <program/Program.h>



void RequestStorageDataWork::process(char* bufIn, unsigned bufInLen,
   char* bufOut, unsigned bufOutLen)
{
   App *app = Program::getApp();

   // let's see at which time we did the last update to the HighResStats and
   // save the time
   HighResStatsList highResStats = node->getHighResData();
   uint64_t lastStatsTime = 0;

   if(!highResStats.empty())
   {
      lastStatsTime = highResStats.back().rawVals.statsTimeMS;
   }

   // generate the RequestStorageDataMsg with the lastStatsTime
   RequestStorageDataMsg requestDataMsg(lastStatsTime);
   char *outBuf;
   NetMessage *rspMsg = NULL;
   bool respReceived = MessagingTk::requestResponse(node, &requestDataMsg,
      NETMSGTYPE_RequestStorageDataResp, &outBuf, &rspMsg);

   // if node does not respond set not-responding flag, otherwise process the
   // response
   if(!respReceived)
   {
      log.log(Log_SPAM, std::string("Received no response from node: ") +
         node->getNodeIDWithTypeStr());
      node->setNotResponding();
   }
   else
   {
      // get response and process it
      RequestStorageDataRespMsg *storageAdmonDataMsg =
         (RequestStorageDataRespMsg*) rspMsg;
      std::string nodeID = storageAdmonDataMsg->getNodeID();
      uint16_t nodeNumID = storageAdmonDataMsg->getNodeNumID();

      NicAddressList nicList;
      storageAdmonDataMsg->parseNicList(&nicList);

      // create a new StorageNodeEx object and fill it with the parsed content
      // from the response 0,0 are dummy ports; the object will be deleted
      // anyways
      StorageNodeEx *newNode = new StorageNodeEx(nodeID, nodeNumID, 0, 0, nicList);

      NodeStoreStorageEx *storageNodeStore = app->getStorageNodes();
      StorageNodeDataContent content = newNode->getContent();
      content.indirectWorkListSize = storageAdmonDataMsg->getIndirectWorkListSize();
      content.directWorkListSize = storageAdmonDataMsg->getDirectWorkListSize();
      content.diskSpaceTotal = storageAdmonDataMsg->getDiskSpaceTotalMiB();
      content.diskSpaceFree = storageAdmonDataMsg->getDiskSpaceFreeMiB();
      storageAdmonDataMsg->parseStorageTargets(&content.storageTargets);
      newNode->setContent(content);

      // same with highResStats
      HighResStatsList newHighResStats;
      storageAdmonDataMsg->parseStatsList(&newHighResStats);
      newNode->setHighResStatsList(newHighResStats);

      // call addOrUpdate in the node store (the newNode object will definitely
      // be deleted in there)
      storageNodeStore->addOrUpdateNode(&newNode);

      SAFE_FREE(outBuf);
      SAFE_DELETE(storageAdmonDataMsg);
   }

   storageNodes->releaseStorageNode(&node);
}


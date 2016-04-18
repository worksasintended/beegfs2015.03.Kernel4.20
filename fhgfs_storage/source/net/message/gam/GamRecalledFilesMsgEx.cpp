#include "GamRecalledFilesMsgEx.h"

#include <common/app/log/LogContext.h>
#include <program/Program.h>
#include <toolkit/StorageTkEx.h>

bool GamRecalledFilesMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("GamRecalledFilesMsg incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG_CONTEXT(log, 4, std::string("Received a GamRecalledFilesMsg from: ") + peer);

   App* app = Program::getApp();
   NodeStore* metaNodes = app->getMetaNodes();

   StringVector recalledChunksVec;
   parseChunkIDVec(&recalledChunksVec);

   std::map<uint16_t, EntryInfoList> recalledEntryInfoMap; // <ownerNodeID, entryInfoList>
   std::map<uint16_t, EntryInfoList>::iterator recalledEntryInfoMapIter;

   uint16_t targetID = this->getTargetID();

   // for each of the chunks read the backlinks and create an EntryInfo object
   for (StringVectorIter iter = recalledChunksVec.begin(); iter != recalledChunksVec.end(); iter++)
   {
      // we need to read entryIDs from the chunks on disk and send them to the MDS
      // for that, we need to group them by ownerNode, which is not nice, but necessary
      std::string chunkID = *iter;

      PathInfo pathInfo;
      EntryInfo entryInfo;
      if (StorageTkEx::readEntryInfoFromChunk(targetID, &pathInfo, chunkID, &entryInfo))
         recalledEntryInfoMap[entryInfo.getOwnerNodeID()].push_back(entryInfo);
      else
      {
         // backlink could not be read, write to log, but ignore
         log.log(3, "Notification of recalled chunk " + chunkID + ", but chunk has no backlink.");
      }
   }

   for (recalledEntryInfoMapIter = recalledEntryInfoMap.begin();
      recalledEntryInfoMapIter != recalledEntryInfoMap.end(); recalledEntryInfoMapIter++)
   {
      uint16_t nodeID = (*recalledEntryInfoMapIter).first;
      EntryInfoList entryInfoList = (*recalledEntryInfoMapIter).second;

      Node* metaNode = metaNodes->referenceNode(nodeID);

      if (!metaNode)
      {
         log.logErr("Recalled entries are supposed to be on metadata node " +
            StringTk::uintToStr(nodeID) + ", but " + StringTk::uintToStr(nodeID) +
            " does not exist in node store.");
         continue; // try the next node
      }

      GamRecalledFilesMetaMsg gamRecalledFilesMetaMsg(&entryInfoList);

      NodeConnPool* connPool = metaNode->getConnPool();

      // cleanup init
      Socket* metaNodeSock = NULL;
      char* sendBuf = NULL;

      // connect
      metaNodeSock = connPool->acquireStreamSocket();

      if ( metaNodeSock )
      {
         // prepare sendBuf
         size_t sendBufLen = gamRecalledFilesMetaMsg.getMsgLength();
         sendBuf = (char*) malloc(sendBufLen);

         gamRecalledFilesMetaMsg.serialize(sendBuf, sendBufLen);

         // send request to metadata node
         metaNodeSock->send(sendBuf, sendBufLen, 0);

         connPool->invalidateStreamSocket(metaNodeSock);

         SAFE_FREE(sendBuf);
      }
      else
      {
         log.log(3, "Could not aquire stream socket for metadata node " +
            StringTk::uintToStr(nodeID) + ".");
      }
      metaNodes->releaseNode(&metaNode);
   }

   return true;
}

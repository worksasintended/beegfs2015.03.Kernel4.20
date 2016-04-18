#include "GamReleaseFilesMsgEx.h"

#include <common/app/log/LogContext.h>
#include <common/toolkit/MessagingTk.h>
#include <program/Program.h>
#include <toolkit/StorageTkEx.h>

bool GamReleaseFilesMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("GamReleaseFilesMsg incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG_CONTEXT(log, 4, std::string("Received a GamReleaseFilesMsg from: ") + peer);

   App* app = Program::getApp();
   NodeStore* metaNodes = app->getMetaNodes();

   StringVector releaseChunksVec;
   StringVector keepChunksVec;
   parseChunkIDVec(&releaseChunksVec);

   std::map<uint16_t, EntryInfoList> releaseEntryInfoMap; // <ownerNodeID, entryInfoList>
   std::map<uint16_t, EntryInfoList>::iterator releaseEntryInfoMapIter;

   uint16_t targetID = this->getTargetID();

   // for each of the chunks read the backlinks and create an EntryInfo object
   for ( StringVectorIter iter = releaseChunksVec.begin(); iter != releaseChunksVec.end(); iter++ )
   {
      // we need to read entryIDs from the chunks on disk and send them to the MDS
      // for that, we need to group them by ownerNode, which is not nice, but necessary
      std::string chunkID = *iter;

      PathInfo pathInfo;
      EntryInfo entryInfo;
      if ( StorageTkEx::readEntryInfoFromChunk(targetID, &pathInfo, chunkID, &entryInfo) )
         releaseEntryInfoMap[entryInfo.getOwnerNodeID()].push_back(entryInfo);
      else
      {
         // backlink could not be read, directly deny release
         log.log(3, "Request to release chunk " + chunkID + ", but chunk has no backlink.");
         keepChunksVec.push_back(chunkID);
      }
   }

   for ( releaseEntryInfoMapIter = releaseEntryInfoMap.begin();
      releaseEntryInfoMapIter != releaseEntryInfoMap.end(); releaseEntryInfoMapIter++ )
   {
      // send a message to each of the involved MDSs and put their vetos to the outgoing list
      uint16_t nodeID = (*releaseEntryInfoMapIter).first;
      EntryInfoList entryInfoList = (*releaseEntryInfoMapIter).second;

      Node* metaNode = metaNodes->referenceNode(nodeID);

      if(!metaNode)
      {
         log.logErr(
            "Entries on metadata node " + StringTk::uintToStr(nodeID) + " should be released, but "
               + StringTk::uintToStr(nodeID) + " does not exist in node store.");
         continue; // try the next node
      }

      bool commRes;
      char* metaRespBuf = NULL;
      NetMessage* metaRespMsg = NULL;
      GamReleaseFilesMetaRespMsg* metaRespMsgCast = NULL;

      GamReleaseFilesMetaMsg gamReleaseFilesMetaMsg(&entryInfoList, getUrgency());
      commRes = MessagingTk::requestResponse(metaNode, &gamReleaseFilesMetaMsg,
         NETMSGTYPE_GamReleaseFilesMetaResp, &metaRespBuf, &metaRespMsg);

      if ( commRes )
      {
         metaRespMsgCast = (GamReleaseFilesMetaRespMsg*) metaRespMsg;

         EntryInfoList keepEntryInfoList;
         metaRespMsgCast->parseEntryInfoList(&keepEntryInfoList);

         for ( EntryInfoListIter iter = keepEntryInfoList.begin(); iter != keepEntryInfoList.end();
            iter++ )
         {
            keepChunksVec.push_back((*iter).getEntryID());
         }
      }
      else
      {
         log.logErr("Communication with metadata node " + StringTk::uintToStr(nodeID) + " failed.");
         // deny all
         for ( EntryInfoListIter iter = entryInfoList.begin(); iter != entryInfoList.end(); iter++ )
         {
            keepChunksVec.push_back((*iter).getEntryID());
         }
      }

      metaNodes->releaseNode(&metaNode);
      SAFE_DELETE(metaRespMsg);
      SAFE_FREE(metaRespBuf);
   }

   // send response
   GamReleaseFilesRespMsg respMsg(&keepChunksVec);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0, NULL, 0);

   return true;
}

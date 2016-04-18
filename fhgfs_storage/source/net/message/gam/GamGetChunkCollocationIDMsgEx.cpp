#include <common/toolkit/MessagingTk.h>
#include <toolkit/StorageTkEx.h>
#include <program/Program.h>

#include "GamGetChunkCollocationIDMsgEx.h"

bool GamGetChunkCollocationIDMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("GamGetChunkCollocationIDMsg incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG_CONTEXT(log, 4, std::string("Received a GamGetChunkCollocationIDMsg from: ") + peer);
   
   StringVector chunkIDs;
   StringVector respChunkIDs;
   UShortVector respCollocationIDs;

   this->parseChunkIDs(&chunkIDs);

   std::map<uint16_t, EntryInfoList> entryInfoMap; // <ownerNodeID, EntryInfoList>
   std::map<uint16_t, EntryInfoList>::iterator entryInfoMapIter;

   uint16_t targetID = this->getTargetID();

   // for each of the chunks read the backlinks and create an EntryInfo object
   for ( StringVectorIter iter = chunkIDs.begin(); iter != chunkIDs.end(); iter++ )
   {
      // we need to read entryIDs from the chunks on disk and send them to the MDS
      // for that, we need to group them by ownerNode, which is not nice, but necessary
      std::string chunkID = *iter;

      EntryInfo entryInfo;
      PathInfo pathInfo;
      bool readBacklinkRes = StorageTkEx::readEntryInfoFromChunk(targetID, &pathInfo, chunkID,
         &entryInfo);

      if ( readBacklinkRes )
         entryInfoMap[entryInfo.getOwnerNodeID()].push_back(entryInfo);
      else
      {
         // insert invalid collocation ID
         respChunkIDs.push_back(chunkID);
         respCollocationIDs.push_back(HSM_COLLOCATIONID_INVALID);

         log.log(3, "Request to read collocation ID from chunk " + chunkID + ", but chunk has no "
            "backlink.");
      }
   }

   for ( entryInfoMapIter = entryInfoMap.begin(); entryInfoMapIter != entryInfoMap.end();
      entryInfoMapIter++ )
   {
      // send a message to each of the involved MDSs and put their vetos to the outgoing list
      uint16_t nodeID = (*entryInfoMapIter).first;
      EntryInfoList entryInfoList = (*entryInfoMapIter).second;

      EntryInfoList tempOutEntries;
      UShortVector tempOutCollocationIDs;

      bool requestRes = requestCollocationIDs(nodeID, &entryInfoList, &tempOutEntries,
         &tempOutCollocationIDs);

      if (! requestRes ) // communication failed; error was already printed inside function
         continue; // proceed to next node
      else if ( tempOutEntries.size() != tempOutCollocationIDs.size() )
      {
         log.logErr("Size of returned collocation ID lists from metadata node "
            + StringTk::uintToStr(nodeID) +
            " does not match. Refusing to forward any information to GAM.");
      }
      else
      {
         EntryInfoListIter entryIter = tempOutEntries.begin();
         UShortVectorIter collocationIDIter = tempOutCollocationIDs.begin();
         for ( ; entryIter != tempOutEntries.end(); entryIter++, collocationIDIter++ )
         {
            respChunkIDs.push_back((*entryIter).getEntryID());
            respCollocationIDs.push_back(*collocationIDIter);
         }
      }
   }

   // send response
   GamGetChunkCollocationIDRespMsg respMsg(&respChunkIDs, &respCollocationIDs);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   return true;
}

bool GamGetChunkCollocationIDMsgEx::requestCollocationIDs(uint16_t nodeID,
   EntryInfoList* entryInfoList, EntryInfoList* outEntryInfos,
   GamCollocationIDVector* outCollocationIDs)
{
   LogContext log("GamGetChunkCollocationIDMsg incoming");

   App* app = Program::getApp();
   NodeStore* metaNodes = app->getMetaNodes();

   bool commRes;
   char* respBuf = NULL;
   NetMessage* respMsg = NULL;
   GamGetCollocationIDMetaRespMsg* respMsgCast = NULL;

   Node* metaNode = metaNodes->referenceNode(nodeID);

   if(!metaNode)
   {
      log.logErr(
         "Collocation IDs on metadata node " + StringTk::uintToStr(nodeID) + " should be read, but "
            + StringTk::uintToStr(nodeID) + " does not exist in node store.");
      return false;
   }

   GamGetCollocationIDMetaMsg gamGetCollocationIDMetaMsg(entryInfoList);
   commRes = MessagingTk::requestResponse(metaNode, &gamGetCollocationIDMetaMsg,
      NETMSGTYPE_GamGetCollocationIDMetaResp, &respBuf, &respMsg);

   if ( !commRes )
   {
      log.logErr("Communication with metadata node " + StringTk::uintToStr(nodeID) + " failed.");
      return false;
   }

   metaNodes->releaseNode(&metaNode);

   respMsgCast = (GamGetCollocationIDMetaRespMsg*) respMsg;

   respMsgCast->parseEntryInfoList(outEntryInfos);
   respMsgCast->parseCollocationIDs(outCollocationIDs);

   SAFE_DELETE(respMsg);
   SAFE_FREE(respBuf);

   return true;
}


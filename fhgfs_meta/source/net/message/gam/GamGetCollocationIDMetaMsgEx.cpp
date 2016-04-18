#include <program/Program.h>

#include "GamGetCollocationIDMetaMsgEx.h"

bool GamGetCollocationIDMetaMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("GamGetCollocationIDMetaMsg incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG_CONTEXT(log, 4, std::string("Received a GamGetCollocationIDMetaMsg from: ") + peer);
   
   MetaStore* metaStore = Program::getApp()->getMetaStore();

   EntryInfoList entryInfoList;

   EntryInfoList respEntryInfoList;
   UShortVector respCollocationIDList;

   this->parseEntryInfoList(&entryInfoList);

   for ( EntryInfoListIter entryInfoIter = entryInfoList.begin();
      entryInfoIter != entryInfoList.end(); entryInfoIter++ )
   {
      EntryInfo entryInfo = *entryInfoIter;
      short collocationID;

      std::string entryID = entryInfo.getEntryID();

      if ( DirEntryType_ISDIR(entryInfo.getEntryType()) )
      {
         DirInode* dirInode = metaStore->referenceDir(entryInfo.getEntryID(), true);

         collocationID = dirInode->getHsmCollocationID();

         metaStore->releaseDir(entryInfo.getEntryID());
      }
      else if ( DirEntryType_ISREGULARFILE(entryInfo.getEntryType()) )
      {
         FileInode* fileInode = metaStore->referenceFile(&entryInfo);
         collocationID = fileInode->getHsmCollocationID();
         metaStore->releaseFile(entryInfo.getParentEntryID(), fileInode);
      }
      else
      {
         collocationID = HSM_COLLOCATIONID_INVALID;
         log.logErr(
            "Request to get GAM collocation ID for entryID " + entryInfo.getEntryID()
               + ", but entryID does not exist.");
      }

      respEntryInfoList.push_back(entryInfo);
      respCollocationIDList.push_back(collocationID);
   }

   // send response
   GamGetCollocationIDMetaRespMsg respMsg(&respEntryInfoList, &respCollocationIDList);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0, (struct sockaddr*) fromAddr,
      sizeof(struct sockaddr_in));

   return true;
}


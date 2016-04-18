#include <program/Program.h>

#include "GamReleaseFilesMetaMsgEx.h"

bool GamReleaseFilesMetaMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("GamReleaseFilesMetaMsg incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername(); 
   LOG_DEBUG_CONTEXT(log, 4, std::string("Received a GamReleaseFilesMetaMsg from: ") + peer);
   
   MetaStore* metaStore = Program::getApp()->getMetaStore();

   uint8_t urgency = getUrgency();

   EntryInfoList entryInfoList;
   EntryInfoList keepEntryInfoList;

   this->parseEntryInfoList(&entryInfoList);

   for ( EntryInfoListIter iter = entryInfoList.begin(); iter != entryInfoList.end(); iter++ )
   {
      // tell the MDS to mark the file as released;
      // if this returns false, an error occured or MDS overrides decision => add to "keep list"
      EntryInfo entryInfo = *iter;
      FileInode *fileInode = metaStore->referenceFile(&entryInfo);

      bool releaseRes = fileInode->hsmRelease(&entryInfo, urgency);

      if (!releaseRes)
         keepEntryInfoList.push_back(entryInfo);

      metaStore->releaseFile(entryInfo.getParentEntryID(), fileInode);
   }

   // send response
   GamReleaseFilesMetaRespMsg respMsg(&keepEntryInfoList);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   return true;
}


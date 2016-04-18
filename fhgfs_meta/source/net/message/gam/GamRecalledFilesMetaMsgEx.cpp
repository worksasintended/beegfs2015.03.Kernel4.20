#include <program/Program.h>

#include "GamRecalledFilesMetaMsgEx.h"

bool GamRecalledFilesMetaMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("GamRecalledFilesMetaMsg incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername(); 
   LOG_DEBUG_CONTEXT(log, 4, std::string("Received a GamRecalledFilesMetaMsg from: ") + peer);
   
   MetaStore* metaStore = Program::getApp()->getMetaStore();

   EntryInfoList entryInfoList;

   this->parseEntryInfoList(&entryInfoList);

   for ( EntryInfoListIter iter = entryInfoList.begin(); iter != entryInfoList.end(); iter++ )
   {
      // tell the MDS to mark a chunk as recalled
      EntryInfo entryInfo = *iter;
      FileInode *fileInode = metaStore->referenceFile(&entryInfo);

      fileInode->hsmRecalled(&entryInfo);

      metaStore->releaseFile(entryInfo.getParentEntryID(), fileInode);
   }

   return true;
}


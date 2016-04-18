#include <program/Program.h>
#include <common/net/message/fsck/RemoveInodesRespMsg.h>
#include "RemoveInodesMsgEx.h"


bool RemoveInodesMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats) throw(SocketException)
{
   const char* logContext = "RemoveInodesMsg incoming";

   #ifdef BEEGFS_DEBUG
      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, 4, std::string("Received a RemoveInodesMsg from: ") + peer);
   #endif // BEEGFS_DEBUG
   

   MetaStore* metaStore = Program::getApp()->getMetaStore();

   StringList entryIDList;
   DirEntryTypeList entryTypeList;

   StringList failedIDList;
   DirEntryTypeList failedEntryTypeList;

   this->parseEntryIDList(&entryIDList);
   this->parseEntryTypeList(&entryTypeList);

   StringListIter entryIDIter = entryIDList.begin();
   DirEntryTypeListIter entryTypeIter = entryTypeList.begin();

   if (entryIDList.size() != entryTypeList.size())
   {
      LogContext(logContext).log(Log_WARNING, "Incoming lists do not match in size.");
      failedIDList = entryIDList;
      failedEntryTypeList = entryTypeList;
      goto sendResponse;
   }

   for ( ; entryIDIter != entryIDList.end(); entryIDIter++, entryTypeIter++ )
   {
      std::string entryID = *entryIDIter;
      DirEntryType entryType = *entryTypeIter;
      FhgfsOpsErr rmRes;

      if ( entryType == DirEntryType_DIRECTORY )
      {
         rmRes = metaStore->removeDirInode(entryID);
      }
      else
      {
         rmRes = metaStore->fsckUnlinkFileInode(entryID);
      }

      if (rmRes != FhgfsOpsErr_SUCCESS)
      {
         failedIDList.push_back(entryID);
         failedEntryTypeList.push_back(entryType);
      }
   }
   
   sendResponse:
   RemoveInodesRespMsg respMsg(&failedIDList, &failedEntryTypeList);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   return true;      
}

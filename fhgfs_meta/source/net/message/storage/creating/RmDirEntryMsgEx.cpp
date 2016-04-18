#include <program/Program.h>
#include <common/net/message/storage/creating/RmDirEntryRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include "RmDirEntryMsgEx.h"

bool RmDirEntryMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("RmDirEntryMsg incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG_CONTEXT(log, 4, std::string("Received a RmDirEntryMsg from: ") + peer);


   EntryInfo* parentInfo = getParentInfo();
   std::string entryName = getEntryName();

   FhgfsOpsErr rmRes = rmDirEntry(parentInfo, entryName);

   RmDirEntryRespMsg respMsg(rmRes);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   App* app = Program::getApp();
   app->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_RMLINK,
      getMsgHeaderUserID() );

   return true;
}

FhgfsOpsErr RmDirEntryMsgEx::rmDirEntry(EntryInfo* parentInfo, std::string& entryName)
{
   const char* logContext = "RmDirEntryMsg (rm entry)";

   MetaStore* metaStore = Program::getApp()->getMetaStore();

   FhgfsOpsErr retVal;


   // reference parent
   DirInode* parentDir = metaStore->referenceDir(parentInfo->getEntryID(), true);
   if(!parentDir)
      return FhgfsOpsErr_PATHNOTEXISTS;

   DirEntry removeDentry(entryName);
   bool getRes = parentDir->getDentry(entryName, removeDentry);
   if(!getRes)
      retVal = FhgfsOpsErr_PATHNOTEXISTS;
   else
   {
      if(DirEntryType_ISDIR(removeDentry.getEntryType() ) )
      { // remove dir link
         retVal = parentDir->removeDir(entryName, NULL);
      }
      else
      if(DirEntryType_ISFILE(removeDentry.getEntryType() ) )
      {
         retVal = parentDir->unlinkDirEntry(entryName, &removeDentry,
            DirEntry_UNLINK_ID_AND_FILENAME);
      }
      else
      { // unknown entry type
         LogContext(logContext).logErr(std::string("Unknown entry type: ") +
            StringTk::intToStr(removeDentry.getEntryType() ) );

         retVal = FhgfsOpsErr_INTERNAL;
      }
   }

   // clean-up
   metaStore->releaseDir(parentInfo->getEntryID() );

   return retVal;
}


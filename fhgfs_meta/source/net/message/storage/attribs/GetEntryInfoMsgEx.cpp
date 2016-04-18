#include <common/net/message/storage/attribs/GetEntryInfoRespMsg.h>
#include <common/storage/striping/Raid0Pattern.h>
#include <common/toolkit/MessagingTk.h>
#include <program/Program.h>
#include <storage/MetaStore.h>
#include "GetEntryInfoMsgEx.h"


bool GetEntryInfoMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   #ifdef BEEGFS_DEBUG
      const char* logContext = "GetEntryInfoMsg incoming";

      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, 4, std::string("Received a GetEntryInfoMsg from: ") + peer);
   #endif // BEEGFS_DEBUG

   EntryInfo* entryInfo = this->getEntryInfo();

   LOG_DEBUG(logContext, Log_SPAM,
      "ParentEntryID: " + entryInfo->getParentEntryID() + "; "
      "entryID: " + entryInfo->getParentEntryID() );
   
   StripePattern* pattern = NULL;
   uint16_t mirrorNodeID = 0;
   FhgfsOpsErr getInfoRes;
   PathInfo pathInfo;

   
   if(entryInfo->getParentEntryID().empty() )
   { // special case: get info for root directory

      // no pathInfo here, as this is currently only used for fileInodes

      getInfoRes = getRootInfo(&pattern, &mirrorNodeID);
   }
   else
   {
      getInfoRes = getInfo(entryInfo, &pattern, &mirrorNodeID, &pathInfo);
   }
   
   
   if(getInfoRes != FhgfsOpsErr_SUCCESS)
   { // error occurred => create a dummy pattern
      UInt16Vector dummyStripeNodes;
      pattern = new Raid0Pattern(1, dummyStripeNodes);
   }

   GetEntryInfoRespMsg respMsg(getInfoRes, pattern, mirrorNodeID, &pathInfo);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   // cleanup
   delete(pattern);

   App* app = Program::getApp();
   app->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_GETENTRYINFO,
      getMsgHeaderUserID() );

   return true;      
}


/**
 * @param outPattern StripePattern clone (in case of success), that must be deleted by the caller
 */
FhgfsOpsErr GetEntryInfoMsgEx::getInfo(EntryInfo* entryInfo, StripePattern** outPattern,
   uint16_t* outMirrorNodeID, PathInfo* outPathInfo)
{
   MetaStore* metaStore = Program::getApp()->getMetaStore();
   
   if (entryInfo->getEntryType() == DirEntryType_DIRECTORY)
   { // entry is a directory
      DirInode* dir = metaStore->referenceDir(entryInfo->getEntryID(), true);
      if(dir)
      {
         *outPattern = dir->getStripePatternClone();
         *outMirrorNodeID = dir->getMirrorNodeID();

         metaStore->releaseDir(entryInfo->getEntryID() );
         return FhgfsOpsErr_SUCCESS;
      }
   }
   else
   { // entry is a file
      FileInode* fileInode = metaStore->referenceFile(entryInfo);
      if(fileInode)
      {
         *outPattern = fileInode->getStripePattern()->clone();
         *outMirrorNodeID = fileInode->getMirrorNodeID();
         fileInode->getPathInfo(outPathInfo);

         metaStore->releaseFile(entryInfo->getParentEntryID(), fileInode);
         return FhgfsOpsErr_SUCCESS;
      }
   }
   
   return FhgfsOpsErr_PATHNOTEXISTS;
}

/**
 * @param outPattern StripePattern clone (in case of success), that must be deleted by the caller
 */
FhgfsOpsErr GetEntryInfoMsgEx::getRootInfo(StripePattern** outPattern, uint16_t* outMirrorNodeID)
{
   App* app = Program::getApp();
   DirInode* rootDir = app->getRootDir();

   uint16_t localNodeID = app->getLocalNodeNumID();

   if(localNodeID != rootDir->getOwnerNodeID() )
   {
      return FhgfsOpsErr_NOTOWNER;
   }
   
   *outPattern = rootDir->getStripePatternClone();
   *outMirrorNodeID = rootDir->getMirrorNodeID();
   
   return FhgfsOpsErr_SUCCESS;
}

#include <common/net/message/storage/attribs/SetDirPatternRespMsg.h>
#include <common/storage/striping/Raid0Pattern.h>
#include <common/toolkit/MessagingTk.h>
#include <program/Program.h>
#include <storage/DirInode.h>
#include <storage/MetaStore.h>
#include "SetDirPatternMsgEx.h"


bool SetDirPatternMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("SetDirPatternMsgEx incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername(); 
   LOG_DEBUG_CONTEXT(log, Log_DEBUG, std::string("Received a SetDirPatternMsg from: ") + peer);

   EntryInfo* entryInfo = this->getEntryInfo();

   LOG_DEBUG("SetDirPatternMsgEx::processIncoming", Log_SPAM, std::string("parentEntryID: ") +
      entryInfo->getParentEntryID() + " entryID: " + entryInfo->getEntryID() );
   
   StripePattern* pattern = createPattern();
   FhgfsOpsErr setPatternRes;
   
   if(pattern->getPatternType() == STRIPEPATTERN_Invalid)
   { // pattern is invalid
      setPatternRes = FhgfsOpsErr_INTERNAL;
      log.logErr("Received an invalid pattern");
   }
   else
   if( (pattern->getChunkSize() < STRIPEPATTERN_MIN_CHUNKSIZE) ||
       !MathTk::isPowerOfTwo(pattern->getChunkSize() ) )
   { // check of stripe pattern details validity failed

      setPatternRes = FhgfsOpsErr_INTERNAL;
      log.logErr("Received an invalid pattern chunksize: " +
         StringTk::uintToStr(pattern->getChunkSize() ) );
   }
   else
   { // received pattern is valid => apply it
      
      setPatternRes = setDirPattern(entryInfo, pattern);
   }

   
   SetDirPatternRespMsg respMsg(setPatternRes);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );
   
   
   // cleanup
   delete(pattern);

   App* app = Program::getApp();
   app->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_SETDIRPATTERN,
      getMsgHeaderUserID() );

   return true;      
}


/**
 * @param pattern will be cloned and must be deleted by the caller 
 */
FhgfsOpsErr SetDirPatternMsgEx::setDirPattern(EntryInfo* entryInfo, StripePattern* pattern)
{
   const char* logContext = "SetDirPatternMsg (set dir pattern)";

   App* app = Program::getApp();
   MetaStore* metaStore = app->getMetaStore();

   FhgfsOpsErr retVal = FhgfsOpsErr_NOTADIR;

   uint32_t actorUID = isMsgHeaderFeatureFlagSet(Flags::HAS_UID)
      ? getUID()
      : 0;

   if (actorUID != 0 && !app->getConfig()->getSysAllowUserSetPattern())
      return FhgfsOpsErr_PERM;

   // verify owner of root dir
   if( (entryInfo->getEntryID() == META_ROOTDIR_ID_STR) &&
       (app->getLocalNodeNumID() != app->getRootDir()->getOwnerNodeID() ) )
   {
      LogContext(logContext).log(Log_DEBUG, "This node does not own the root directory.");
      return FhgfsOpsErr_NOTOWNER;
   }
   
   DirInode* dir = metaStore->referenceDir(entryInfo->getEntryID(), true);
   if(dir)
   { // entry is a directory
      retVal = dir->setStripePattern(*pattern, actorUID);
      if (retVal != FhgfsOpsErr_SUCCESS)
         LogContext(logContext).logErr("Update of stripe pattern failed. "
            "DirID: " + entryInfo->getEntryID() );

      metaStore->releaseDir(entryInfo->getEntryID() );
   }
   
   return retVal;
}


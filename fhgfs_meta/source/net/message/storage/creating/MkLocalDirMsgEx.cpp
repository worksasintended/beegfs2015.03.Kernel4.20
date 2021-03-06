#include <program/Program.h>
#include <common/net/message/storage/creating/MkLocalDirRespMsg.h>
#include "MkLocalDirMsgEx.h"


bool MkLocalDirMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   const char* logContext = "MkLocalDirMsg incoming";

   EntryInfo* entryInfo = getEntryInfo();

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG(logContext, 4, std::string("Received a MkLocalDirMsg from: ") + peer +
      std::string(" entryID: ") + entryInfo->getEntryID() +
      std::string(" entryName: ") + entryInfo->getFileName() );

   IGNORE_UNUSED_VARIABLE(logContext);
   IGNORE_UNUSED_VARIABLE(entryInfo);

   FhgfsOpsErr mkRes = FhgfsOpsErr_INTERNAL;
   
   StripePattern* pattern = createPattern();
   if(unlikely(pattern->getPatternType() == STRIPEPATTERN_Invalid) )
      LogContext(logContext).logErr("Received an invalid stripe pattern from: " + peer);
   else
      mkRes = mkLocalDir(pattern);

   delete(pattern);

   MkLocalDirRespMsg respMsg(mkRes);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   return true;
}


/**
 * Creates a specific directory and adds it to the store.
 */
FhgfsOpsErr MkLocalDirMsgEx::mkLocalDir(StripePattern* pattern)
{
   MetaStore* metaStore = Program::getApp()->getMetaStore();
   uint16_t localNodeID = Program::getApp()->getLocalNode()->getNumID();

   EntryInfo *entryInfo = this->getEntryInfo();

   DirInode* newDir = new DirInode(entryInfo->getEntryID(), this->getMode(), this->getUserID(),
      this->getGroupID(), localNodeID, *pattern);

   if(isMsgHeaderFeatureFlagSet(MKLOCALDIRMSG_FLAG_MIRRORED) )
      newDir->setMirrorNodeIDUnlocked(getMirrorNodeID() );

#ifdef BEEGFS_HSM_DEPRECATED
   newDir->setHsmCollocationID(this->getHsmCollocationID());
#endif

   uint16_t parentNodeID = this->getParentNodeID();

   newDir->setParentInfoInitial(entryInfo->getParentEntryID(), parentNodeID);

   FhgfsOpsErr mkRes = metaStore->makeDirInode(newDir, getDefaultACLXAttr(), getAccessACLXAttr() );

   return mkRes;
}

#include <common/net/message/storage/creating/MkFileRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <common/storage/StatData.h>
#include <net/msghelpers/MsgHelperMkFile.h>
#include <program/Program.h>
#include "MkFileMsgEx.h"


bool MkFileMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   #ifdef BEEGFS_DEBUG
      const char* logContext = "MkFileMsg incoming";

      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, Log_DEBUG, std::string("Received a MkFileMsg from: ") + peer);
   #endif // BEEGFS_DEBUG

   LOG_DEBUG(logContext, Log_SPAM, "parentEntryID: " +
      getParentInfo()->getEntryID() + " newFileName: " + getNewName() );

   MkFileDetails mkDetails(this->getNewName(), getUserID(), getGroupID(), getMode() );

   UInt16List preferredNodes;
   parsePreferredNodes(&preferredNodes);

   EntryInfo entryInfo;
   FileInodeStoreData inodeData;

   FhgfsOpsErr mkRes = MsgHelperMkFile::mkFile(getParentInfo(), &mkDetails, &preferredNodes,
      getNumTargets(), getChunkSize(), &entryInfo, &inodeData);

   MkFileRespMsg respMsg(mkRes, &entryInfo );
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   App* app = Program::getApp();
   app->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_MKFILE,
      getMsgHeaderUserID() );

   return true;
}


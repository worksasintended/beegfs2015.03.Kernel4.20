#include <program/Program.h>
#include <common/net/message/storage/attribs/UpdateDirParentRespMsg.h>
#include <common/net/message/storage/attribs/SetLocalAttrMsg.h>
#include <common/net/message/storage/attribs/SetLocalAttrRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/VersionTk.h>
#include <components/worker/SetChunkFileAttribsWork.h>
#include "UpdateDirParentMsgEx.h"


bool UpdateDirParentMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   #ifdef BEEGFS_DEBUG
      const char* logContext = "UpdateDirParentMsgEx incoming";

      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, Log_DEBUG, std::string("Received a UpdateParentEntryIDMsg from: ") + peer);
   #endif // BEEGFS_DEBUG

   App* app = Program::getApp();


   EntryInfo* entryInfo = getEntryInfo();
   uint16_t parentNodeID = getParentNodeID();

   MetaStore* metaStore = app->getMetaStore();

   FhgfsOpsErr setRes = metaStore->setDirParent(entryInfo, parentNodeID);

   UpdateDirParentRespMsg respMsg(setRes);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   // update operation counters
   app->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_UPDATEDIRPARENT,
      getMsgHeaderUserID() );

   return true;
}



#include <common/components/streamlistenerv2/IncomingPreprocessedMsgWork.h>
#include <common/net/message/control/GenericResponseMsg.h>
#include <common/net/message/storage/creating/UnlinkFileRespMsg.h>
#include <common/net/message/storage/creating/UnlinkFileMsg.h>
#include <common/net/message/storage/creating/UnlinkLocalFileMsg.h>
#include <common/net/message/storage/creating/UnlinkLocalFileRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <net/msghelpers/MsgHelperUnlink.h>
#include <program/Program.h>
#include "UnlinkFileMsgEx.h"


bool UnlinkFileMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   #ifdef BEEGFS_DEBUG
      const char* logContext = "UnlinkFileMsg incoming";

      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, Log_DEBUG, std::string("Received a UnlinkFileMsg from: ") + peer);
   #endif // BEEGFS_DEBUG
   
   App* app = Program::getApp();
   Config* cfg = app->getConfig();
   EntryInfo* parentInfo = getParentInfo();
   std::string removeName = getDelFileName();

   LOG_DEBUG(logContext, Log_DEBUG, "ParentID: " + parentInfo->getEntryID() + "; "
      "deleteName: " + removeName);

   // update operation counters (here on top because we have an early sock release in this msg)
   app->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_UNLINK,
      getMsgHeaderUserID() );

   /* two alternatives:
         1) early response before chunk files unlink.
         2) normal response after chunk files unlink (incl. chunk files error). */
   
   if(cfg->getTuneEarlyUnlinkResponse() )
   { // alternative 1: response before chunk files unlink
      FileInode* unlinkedInode = NULL;

      FhgfsOpsErr unlinkMetaRes = MsgHelperUnlink::unlinkMetaFile(parentInfo->getEntryID(),
         removeName, &unlinkedInode);

      UnlinkFileRespMsg respMsg(unlinkMetaRes);
      respMsg.serialize(respBuf, bufLen);
      sock->sendto(respBuf, respMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

      IncomingPreprocessedMsgWork::releaseSocket(app, &sock, this);

      /* note: if the file is still opened or if there were hardlinks then unlinkedInode will be
         NULL even on FhgfsOpsErr_SUCCESS */
      if( (unlinkMetaRes == FhgfsOpsErr_SUCCESS) && unlinkedInode)
         MsgHelperUnlink::unlinkChunkFiles(unlinkedInode, getMsgHeaderUserID() );

      return true;
   }

   // alternative 2: response after chunk files unlink

   FhgfsOpsErr unlinkRes = MsgHelperUnlink::unlinkFile(
      parentInfo->getEntryID(), removeName, getMsgHeaderUserID() );

   if(unlikely(unlinkRes == FhgfsOpsErr_COMMUNICATION) )
   { // forward comm error as indirect communication error to client
      GenericResponseMsg respMsg(GenericRespMsgCode_INDIRECTCOMMERR,
         "Communication with storage target failed");
      respMsg.serialize(respBuf, bufLen);
      sock->sendto(respBuf, respMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );
   }
   else
   { // normal response
      UnlinkFileRespMsg respMsg(unlinkRes);
      respMsg.serialize(respBuf, bufLen);
      sock->sendto(respBuf, respMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );
   }

   return true;      
}

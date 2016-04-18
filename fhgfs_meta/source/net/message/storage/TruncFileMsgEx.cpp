#include <common/net/message/control/GenericResponseMsg.h>
#include <common/net/message/storage/TruncFileRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <net/msghelpers/MsgHelperTrunc.h>
#include "TruncFileMsgEx.h"
#include <program/Program.h>

bool TruncFileMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   #ifdef BEEGFS_DEBUG
      const char* logContext = "TruncFileMsgEx incoming";

      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, Log_DEBUG, std::string("Received a TruncFileMsg from: ") + peer);
   #endif // BEEGFS_DEBUG

   int64_t fileSize = getFilesize();
   EntryInfo* entryInfo = getEntryInfo();

   bool useQuota = isMsgHeaderFeatureFlagSet(TRUNCFILEMSG_FLAG_USE_QUOTA);

   FhgfsOpsErr truncRes = MsgHelperTrunc::truncFile(
      entryInfo, fileSize, useQuota, getMsgHeaderUserID() );

   // send response

   if(unlikely(truncRes == FhgfsOpsErr_COMMUNICATION) )
   { // forward comm error as indirect communication error to client
      GenericResponseMsg respMsg(GenericRespMsgCode_INDIRECTCOMMERR,
         "Communication with storage target failed");
      respMsg.serialize(respBuf, bufLen);
      sock->sendto(respBuf, respMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );
   }
   else
   { // normal response
      TruncFileRespMsg respMsg(truncRes);
      respMsg.serialize(respBuf, bufLen);
      sock->sendto(respBuf, respMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );
   }

   // update operation counters
   Program::getApp()->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_TRUNCATE,
      getMsgHeaderUserID() );

   return true;      
}


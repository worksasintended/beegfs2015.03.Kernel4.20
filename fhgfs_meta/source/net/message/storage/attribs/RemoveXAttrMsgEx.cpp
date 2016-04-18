#include <program/Program.h>
#include <common/net/message/storage/attribs/RemoveXAttrRespMsg.h>
#include <net/msghelpers/MsgHelperXAttr.h>
#include "RemoveXAttrMsgEx.h"


bool RemoveXAttrMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock, char* respBuf,
      size_t bufLen, HighResolutionStats* stats)
{
   App* app = Program::getApp();
   Config* config = app->getConfig();
   const char* logContext = "RemoveXAttrMsg incoming";
   std::string localNodeID = app->getLocalNode()->getID();
   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   EntryInfo* entryInfo = this->getEntryInfo();
   const std::string& name = this->getName();

   LOG_DEBUG(logContext, Log_DEBUG,
      std::string("Received a RemoveXAttrMsg from: ") + peer + "; name: " + name + ";");

   FhgfsOpsErr res;

   if (!config->getStoreClientXAttrs() )
   {
      LogContext(logContext).log(Log_ERR,
         "Received a RemoveXAttrMsg, but client-side extended attributes are disabled in config.");

      res = FhgfsOpsErr_NOTSUPP;
      goto resp;
   }

   res = MsgHelperXAttr::removexattr(entryInfo, name);

resp:
   RemoveXAttrRespMsg respMsg(res);

   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   app->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_REMOVEXATTR,
         getMsgHeaderUserID() );

   return true;
}

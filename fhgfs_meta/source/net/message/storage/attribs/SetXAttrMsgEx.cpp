#include <program/Program.h>
#include <common/net/message/storage/attribs/SetXAttrRespMsg.h>
#include <net/msghelpers/MsgHelperXAttr.h>
#include "SetXAttrMsgEx.h"

bool SetXAttrMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock, char* respBuf,
   size_t bufLen, HighResolutionStats* stats)
{
   App* app = Program::getApp();
   Config* config = app->getConfig();
   const char* logContext = "SetXAttrMsg incoming";
   std::string localNodeID = app->getLocalNode()->getID();
   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   EntryInfo* entryInfo = this->getEntryInfo();
   const std::string& name = this->getName();
   const CharVector& value = this->getValue();
   const int flags = this->getFlags();

   LOG_DEBUG(logContext, Log_DEBUG,
      std::string("Received a SetXAttrMsg from: ") + peer + "; name: " + name + ";");

   FhgfsOpsErr setXAttrRes;

   if (!config->getStoreClientXAttrs() )
   {
      LogContext(logContext).log(Log_ERR,
         "Received a SetXAttrMsg, but client-side extended attributes are disabled in config.");

      setXAttrRes = FhgfsOpsErr_NOTSUPP;
      goto resp;
   }

   setXAttrRes = MsgHelperXAttr::setxattr(entryInfo, name, value, flags);

resp:
   SetXAttrRespMsg respMsg(setXAttrRes);

   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   app->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_SETXATTR,
      getMsgHeaderUserID() );

   return true;
}

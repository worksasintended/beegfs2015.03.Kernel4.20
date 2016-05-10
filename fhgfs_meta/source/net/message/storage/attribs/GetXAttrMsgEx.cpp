#include <program/Program.h>
#include <common/net/message/storage/attribs/GetXAttrRespMsg.h>
#include <net/msghelpers/MsgHelperXAttr.h>
#include "GetXAttrMsgEx.h"


bool GetXAttrMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock, char* respBuf,
   size_t bufLen, HighResolutionStats* stats)
{
   App* app = Program::getApp();
   const char* logContext = "GetXAttrMsg incoming";
   Config* config = app->getConfig();
   std::string localNodeID = app->getLocalNode()->getID();
   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   EntryInfo* entryInfo = this->getEntryInfo();
   ssize_t size = this->getSize();
   const std::string& name = this->getName();

   LOG_DEBUG(logContext, Log_DEBUG, std::string("Received a GetXAttrMsg from: ") + peer + "; name: "
      + name + "; size: " + StringTk::intToStr(size) + ";");

   CharVector xAttrValue;
   FhgfsOpsErr getXAttrRes;

   if (!config->getStoreClientXAttrs() )
   {
      LogContext(logContext).log(Log_ERR,
         "Received a GetXAttrMsg, but client-side extended attributes are disabled in config.");

      getXAttrRes = FhgfsOpsErr_NOTSUPP;
      goto resp;
   }

   if(size > MsgHelperXAttr::MAX_VALUE_SIZE)
      size = MsgHelperXAttr::MAX_VALUE_SIZE + 1;

   getXAttrRes = MsgHelperXAttr::getxattr(entryInfo, name, xAttrValue, size);

   if (size >= MsgHelperXAttr::MAX_VALUE_SIZE + 1 && getXAttrRes == FhgfsOpsErr_SUCCESS)
   {
      // The xattr on disk is at least one byte too large. In this case, we have to return
      // an internal error because it won't fit the net message.
      xAttrValue.clear();
      getXAttrRes = FhgfsOpsErr_INTERNAL;
   }

resp:
   GetXAttrRespMsg respMsg(xAttrValue, size, getXAttrRes);

   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   app->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_GETXATTR,
      getMsgHeaderUserID() );

   return true;
}

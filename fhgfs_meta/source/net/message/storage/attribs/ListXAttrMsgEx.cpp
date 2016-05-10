#include <program/Program.h>
#include <common/net/message/storage/attribs/ListXAttrRespMsg.h>
#include <net/msghelpers/MsgHelperXAttr.h>
#include "ListXAttrMsgEx.h"


bool ListXAttrMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock, char* respBuf,
   size_t bufLen, HighResolutionStats* stats)
{
   App* app = Program::getApp();
   const char* logContext = "ListXAttrMsg incoming";
   Config* config = app->getConfig();
   std::string localNodeID = app->getLocalNode()->getID();
   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   EntryInfo* entryInfo = this->getEntryInfo();
   size_t listSize = 0;

   LOG_DEBUG(logContext, Log_DEBUG, std::string("Received a ListXAttrMsg from: ") + peer
      + "; size: " + StringTk::intToStr(this->getSize()) + ";");

   StringVector xAttrVec;
   FhgfsOpsErr listXAttrRes;

   if (!config->getStoreClientXAttrs() )
   {
      LogContext(logContext).log(Log_ERR,
         "Received a ListXAttrMsg, but client-side extended attributes are disabled in config.");

      listXAttrRes = FhgfsOpsErr_NOTSUPP;
      goto resp;
   }

   listXAttrRes = MsgHelperXAttr::listxattr(entryInfo, xAttrVec);

   for (StringVectorConstIter it = xAttrVec.begin(); it != xAttrVec.end(); ++it)
      listSize += it->size() + 1;

   // note: MsgHelperXAttr::MAX_SIZE is a ssize_t, which is always positive, so it will always fit
   // into a size_t
   if ((listSize >= (size_t)MsgHelperXAttr::MAX_VALUE_SIZE + 1)
      && (listXAttrRes == FhgfsOpsErr_SUCCESS))
   {
      // The xattr list on disk is at least one byte too large. In this case, we have to return
      // an internal error because it won't fit the net message.
      xAttrVec.clear();
      listXAttrRes = FhgfsOpsErr_TOOBIG;
   }

resp:
   ListXAttrRespMsg respMsg(xAttrVec, listSize, listXAttrRes);

   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   app->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_LISTXATTR,
         getMsgHeaderUserID() );

   return true;
}

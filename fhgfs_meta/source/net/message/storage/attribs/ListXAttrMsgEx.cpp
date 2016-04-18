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
   ssize_t size = this->getSize();

   LOG_DEBUG(logContext, Log_DEBUG, std::string("Received a ListXAttrMsg from: ") + peer
      + "; size: " + StringTk::intToStr(size) + ";");

   StringVector xAttrVec;
   FhgfsOpsErr listXAttrRes;

   if (!config->getStoreClientXAttrs() )
   {
      LogContext(logContext).log(Log_ERR,
         "Received a ListXAttrMsg, but client-side extended attributes are disabled in config.");

      listXAttrRes = FhgfsOpsErr_NOTSUPP;
      goto resp;
   }

   if(size > XATTR_LIST_MAX)
   {
      listXAttrRes = FhgfsOpsErr_RANGE;
      goto resp;
   }

   if(size != 0)
   {
      char* xAttrBuf = static_cast<char*>(malloc(size) );

      listXAttrRes = MsgHelperXAttr::listxattr(entryInfo, xAttrBuf, size);

      if(listXAttrRes == FhgfsOpsErr_SUCCESS)
      {
         // convert '\0'-separated extended attribute list into string vector
         for(char* xAttrPtr = xAttrBuf; xAttrPtr != xAttrBuf + size;
             xAttrPtr = strchr(xAttrPtr, '\0') + 1)
         {
            xAttrVec.push_back(std::string(xAttrPtr) );
         }
      }

      free(xAttrBuf);
   }
   else // size is zero: User just wants to know the size needed for the buffer
   {
      listXAttrRes = MsgHelperXAttr::listxattr(entryInfo, NULL, size);
   }

resp:
   ListXAttrRespMsg respMsg(xAttrVec, size, listXAttrRes);

   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   app->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_LISTXATTR,
         getMsgHeaderUserID() );

   return true;
}

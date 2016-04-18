#include <program/Program.h>
#include <common/net/message/storage/attribs/GetEntryInfoMsg.h>
#include <common/net/message/storage/attribs/GetEntryInfoRespMsg.h>
#include <common/net/message/storage/attribs/UpdateBacklinkMsg.h>
#include <common/net/message/storage/attribs/UpdateBacklinkRespMsg.h>
#include <common/net/message/storage/creating/HardlinkRespMsg.h>
#include <net/msghelpers/MsgHelperChunkBacklinks.h>
#include <common/toolkit/MessagingTk.h>
#include "HardlinkMsgEx.h"

bool HardlinkMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("HardlinkMsg incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername(); 
   LOG_DEBUG_CONTEXT(log, 4, std::string("Received a HardlinkMsg from: ") + peer);

   EntryInfo* fromDirInfo  = this->getFromDirInfo();
   EntryInfo* toDirInfo    = this->getToDirInfo();
   EntryInfo* fromFileInfo = this->getFromInfo();
   std::string fromName    = this->getFromName();
   std::string toName      = this->getToName();
   
   FhgfsOpsErr linkRes;

   if (isMsgHeaderFeatureFlagSet(HARDLINK_FLAG_IS_TO_DENTRY_CREATE) )
   {
      LOG_DEBUG_CONTEXT(log, 4,
         " Creating remote link dentry:"
         " ToDirID: "           + toDirInfo->getEntryID()            +
         " toName: '"           + toName + "'");

      linkRes = createLinkDentry(fromFileInfo, toDirInfo, toName);
   }
   else
   {
      LOG_DEBUG_CONTEXT(log, 4,
         " FromDirID: " + fromDirInfo->getEntryID()   +
         " FromID: "    + fromFileInfo->getEntryID()        +
         " fromName: '" + fromName + "'"                    +
         " ToDirID: "   + toDirInfo->getEntryID()           +
         " toName: '"   + toName + "'");

      linkRes = createLink(fromDirInfo, fromFileInfo, fromName, toDirInfo, toName);
   }

   HardlinkRespMsg respMsg(linkRes);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   App* app = Program::getApp();
   app->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_HARDLINK,
      getMsgHeaderUserID() );

   return true;
}

FhgfsOpsErr HardlinkMsgEx::createLink(EntryInfo* fromDirInfo, EntryInfo* fromFileInfo,
   std::string& fromName, EntryInfo* toDirInfo, std::string& toName)
{
   const char* logContext = "create hard link";
   FhgfsOpsErr retVal = FhgfsOpsErr_INTERNAL;

   App* app = Program::getApp();
   MetaStore* metaStore = app->getMetaStore();

   if (isInSameDir(fromDirInfo, toDirInfo) )
   { // simple and cheap link (everything local)

      // TODO: must check if the inode is inlined and if inode is on remote meta node

      LOG_DEBUG(logContext, Log_SPAM, "Method: link in same dir"); // debug in
      IGNORE_UNUSED_VARIABLE(logContext);


      retVal = metaStore->linkInSameDir(fromDirInfo, fromFileInfo, fromName, toName);
   }
   else
   {  // multi dir hard link
      retVal = createMultiDirHardlink(fromFileInfo, toDirInfo, toName);
   }

   return retVal;
}

FhgfsOpsErr HardlinkMsgEx::createMultiDirHardlink(EntryInfo* fromFileInfo, EntryInfo* toDirInfo,
   std::string toName)
{
   const char* logContext = "Create multi-dir hardlink";

   App* app = Program::getApp();

   // TODO: de-ineline, increase inode link count


   // prepare request

   uint16_t toNodeID = toDirInfo->getOwnerNodeID();

   HardlinkMsg createLinkDentryMsg(fromFileInfo, toDirInfo, toName);

   RequestResponseArgs rrArgs(NULL, &createLinkDentryMsg, NETMSGTYPE_HardlinkResp);

   RequestResponseNode rrNode(toNodeID, app->getMetaNodes() );
   rrNode.setTargetStates(app->getMetaStateStore() );


   // send request to node and receive response

   FhgfsOpsErr requestRes = MessagingTk::requestResponseNode(&rrNode, &rrArgs);

   if(unlikely(requestRes != FhgfsOpsErr_SUCCESS) )
   { // communication error
      LogContext(logContext).log(Log_WARNING,
         "Communication with metadata node failed. "
         "nodeID: " + StringTk::uintToStr(toNodeID) );
      return requestRes;
   }

   HardlinkRespMsg* dentryRespMsg = (HardlinkRespMsg*)rrArgs.outRespMsg;

   FhgfsOpsErr retVal = dentryRespMsg->getResult();

   // TODO: decrease link count and possibly de-inline if retVal != SUCESS

   return retVal;
}

FhgfsOpsErr HardlinkMsgEx::createLinkDentry(EntryInfo* fromFileInfo, EntryInfo* toDirInfo,
   std::string& toName)
{
   FhgfsOpsErr retVal = FhgfsOpsErr_INTERNAL; // not supported yet, TODO

   return retVal;
}


/**
 * Checks whether the requested rename operation is not actually a move operation, but really
 * just a simple rename (in which case nothing is moved to a new directory)
 *
 * @return true if it is a simple rename (no moving involved)
 */
bool HardlinkMsgEx::isInSameDir(EntryInfo* fromDirInfo, EntryInfo* toDirInfo)
{
   if (fromDirInfo->getEntryID() == toDirInfo->getEntryID() )
      return true;

   return false;
}



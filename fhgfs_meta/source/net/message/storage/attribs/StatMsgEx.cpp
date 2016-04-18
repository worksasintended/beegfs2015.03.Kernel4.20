#include <program/Program.h>
#include <common/net/message/storage/attribs/StatRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <net/msghelpers/MsgHelperStat.h>
#include "StatMsgEx.h"


bool StatMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   #ifdef BEEGFS_DEBUG
      const char* logContext = "StatMsgEx incoming";

      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, 4, std::string("Received a StatMsg from: ") + peer);
   #endif // BEEGFS_DEBUG

   App* app = Program::getApp();
   std::string localNodeID = app->getLocalNode()->getID();
   EntryInfo* entryInfo = this->getEntryInfo();

   LOG_DEBUG(logContext, 5, "ParentID: " + entryInfo->getParentEntryID() +
      " EntryID: " + entryInfo->getEntryID() +
      " EntryType: " + StringTk::intToStr(entryInfo->getEntryType()));
   
   StatData statData;
   FhgfsOpsErr statRes;

   uint16_t parentNodeID = 0;
   std::string parentEntryID;
   
   if(entryInfo->getParentEntryID().empty() || (entryInfo->getEntryID() == META_ROOTDIR_ID_STR) )
   { // special case: stat for root directory
      statRes = statRoot(statData);

      parentNodeID = 0;
   }
   else
   {
      statRes = MsgHelperStat::stat(entryInfo, true, getMsgHeaderUserID(), statData, &parentNodeID,
         &parentEntryID);
   }

   LOG_DEBUG(logContext, 4, std::string("statRes: ") + FhgfsOpsErrTk::toErrString(statRes) );

   StatRespMsg respMsg(statRes, statData);

   if(isMsgHeaderCompatFeatureFlagSet(STATMSG_COMPAT_FLAG_GET_PARENTINFO) && parentNodeID)
      respMsg.addParentInfo(parentNodeID, parentEntryID);

   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   app->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_STAT,
      getMsgHeaderUserID() );

   return true;      
}

FhgfsOpsErr StatMsgEx::statRoot(StatData& outStatData)
{
   DirInode* rootDir = Program::getApp()->getRootDir();

   uint16_t localNodeID = Program::getApp()->getLocalNode()->getNumID();

   if(localNodeID != rootDir->getOwnerNodeID() )
   {
      return FhgfsOpsErr_NOTOWNER;
   }
   
//   size_t subdirsSize = rootDir->getSubdirs()->getSize();
//
//   outStatData->mode = S_IFDIR | 0777;
//   outStatData->size = rootDir->getFiles()->getSize() + subdirsSize;
//   outStatData->nlink = 2 + subdirsSize; // 2 + numSubDirs by definition

   rootDir->getStatData(outStatData);

   
   return FhgfsOpsErr_SUCCESS;
}


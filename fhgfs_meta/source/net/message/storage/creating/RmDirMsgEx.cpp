#include <common/net/message/control/GenericResponseMsg.h>
#include <common/net/message/storage/creating/RmDirRespMsg.h>
#include <common/net/message/storage/creating/RmLocalDirMsg.h>
#include <common/net/message/storage/creating/RmLocalDirRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <components/ModificationEventFlusher.h>
#include <program/Program.h>
#include "RmDirMsgEx.h"


bool RmDirMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   #ifdef BEEGFS_DEBUG
      const char* logContext = "RmDirMsg incoming";

      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, 4, std::string("Received a RmDirMsg from: ") + peer);
   #endif // BEEGFS_DEBUG

   ModificationEventFlusher* modEventFlusher = Program::getApp()->getModificationEventFlusher();
   bool modEventLoggingEnabled = modEventFlusher->isLoggingEnabled();

   EntryInfo delEntryInfo;

   FhgfsOpsErr rmRes = rmDir(delEntryInfo);
   
   if ( (rmRes == FhgfsOpsErr_SUCCESS) && (modEventLoggingEnabled) )
   {
      std::string entryID = delEntryInfo.getEntryID();
      modEventFlusher->add(ModificationEvent_DIRREMOVED, entryID);
   }

   // send response

   if(unlikely(rmRes == FhgfsOpsErr_COMMUNICATION) )
   { // forward comm error as indirect communication error to client
      GenericResponseMsg respMsg(GenericRespMsgCode_INDIRECTCOMMERR,
         "Communication with other metadata server failed");
      respMsg.serialize(respBuf, bufLen);
      sock->sendto(respBuf, respMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );
   }
   else
   { // normal response
      RmDirRespMsg respMsg(rmRes);
      respMsg.serialize(respBuf, bufLen);
      sock->sendto(respBuf, respMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );
   }

   // update operation counters
   Program::getApp()->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_RMDIR,
      getMsgHeaderUserID() );

   return true;
}

FhgfsOpsErr RmDirMsgEx::rmDir(EntryInfo& delEntryInfo)
{
   MetaStore* metaStore = Program::getApp()->getMetaStore();
   
   FhgfsOpsErr retVal;

   EntryInfo* parentInfo = this->getParentInfo();
   std::string delDirName = this->getDelDirName();

   // reference parent
   DirInode* parentDir = metaStore->referenceDir(parentInfo->getEntryID(), true);
   if(!parentDir)
      return FhgfsOpsErr_PATHNOTEXISTS;
   
   DirEntry removeDirEntry(delDirName);
   bool getEntryRes = parentDir->getDirDentry(delDirName, removeDirEntry);
   if(!getEntryRes)
      retVal = FhgfsOpsErr_PATHNOTEXISTS;
   else
   {
      int additionalFlags = 0;

      std::string parentEntryID = parentInfo->getEntryID();
      removeDirEntry.getEntryInfo(parentEntryID, additionalFlags, &delEntryInfo);

      // remove the remote dir inode first (to make sure it's empty and not in use)
      retVal = rmRemoteDirInode(&delEntryInfo);
      
      if(retVal == FhgfsOpsErr_SUCCESS)
      { // local removal succeeded => remove meta dir
         retVal = parentDir->removeDir(delDirName, NULL);
      }
   }

   // clean-up
   metaStore->releaseDir(parentInfo->getEntryID() );
   
   return retVal;
}

/**
 * Remove the inode of this directory
 */
FhgfsOpsErr RmDirMsgEx::rmRemoteDirInode(EntryInfo* delEntryInfo)
{
   const char* logContext = "RmDirMsg (rm dir inode)";
   
   FhgfsOpsErr retVal = FhgfsOpsErr_SUCCESS;
   
   App* app = Program::getApp();
   uint16_t nodeID = delEntryInfo->getOwnerNodeID();
   
   LOG_DEBUG(logContext, Log_DEBUG,
      "Removing dir inode from metadata node: " + StringTk::uintToStr(nodeID) + "; "
      "dirname: " + delEntryInfo->getFileName() );

   // prepare request

   RmLocalDirMsg rmMsg(delEntryInfo);

   RequestResponseArgs rrArgs(NULL, &rmMsg, NETMSGTYPE_RmLocalDirResp);

   RequestResponseNode rrNode(nodeID, app->getMetaNodes() );
   rrNode.setTargetStates(app->getMetaStateStore() );
   
   do // (this loop just exists to enable the "break"-jump, so it's not really a loop)
   {
      // send request to other mds and receive response
      
      FhgfsOpsErr requestRes = MessagingTk::requestResponseNode(&rrNode, &rrArgs);
   
      if(requestRes != FhgfsOpsErr_SUCCESS)
      { // communication error
         LogContext(logContext).log(Log_WARNING,
            "Communication with metadata server failed. "
            "nodeID: " + StringTk::uintToStr(nodeID) + "; "
            "dirID: " + delEntryInfo->getEntryID() + "; " +
            "dirname: " + delEntryInfo->getFileName() );
         retVal = requestRes;
         break;
      }
      
      // correct response type received
      RmLocalDirRespMsg* rmRespMsg = (RmLocalDirRespMsg*)rrArgs.outRespMsg;
      
      retVal = rmRespMsg->getResult();
      if(unlikely( (retVal != FhgfsOpsErr_SUCCESS) && (retVal != FhgfsOpsErr_NOTEMPTY) ) )
      { // error: dir inode not removed
         std::string errString = FhgfsOpsErrTk::toErrString(retVal);

         LogContext(logContext).log(Log_DEBUG,
            "Metadata server was unable to remove dir inode. "
            "node: "    + StringTk::uintToStr(nodeID)   + "; "
            "dirID: "   + delEntryInfo->getEntryID()    + "; " +
            "dirname: " + delEntryInfo->getFileName()   + "; " +
            "error: "   + errString);
         
         break;
      }
      
      // success: dir inode removed
      LOG_DEBUG(logContext, Log_DEBUG,
         "Metadata server removed dir inode: "
         "nodeID: " + StringTk::uintToStr(nodeID)      + "; "
         "dirID: " + delEntryInfo->getEntryID()        + "; " +
         "dirname: " + delEntryInfo->getFileName() );
      
   } while(0);

   
   return retVal;
}


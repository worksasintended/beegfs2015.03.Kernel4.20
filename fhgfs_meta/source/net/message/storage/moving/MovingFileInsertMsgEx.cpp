#include <program/Program.h>
#include <common/net/message/storage/moving/MovingFileInsertMsg.h>
#include <common/net/message/storage/moving/MovingFileInsertRespMsg.h>
#include <common/storage/striping/Raid0Pattern.h>
#include <common/toolkit/MessagingTk.h>
#include <net/msghelpers/MsgHelperChunkBacklinks.h>
#include <net/msghelpers/MsgHelperUnlink.h>
#include <net/msghelpers/MsgHelperXAttr.h>

#include "MovingFileInsertMsgEx.h"


bool MovingFileInsertMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("MovingFileInsertMsg incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername(); 
   LOG_DEBUG_CONTEXT(log, Log_DEBUG, std::string("Received a MovingFileInsertMsg from: ") + peer);

   FileInode* unlinkInode = NULL;

   FhgfsOpsErr insertRes = this->insert(sock, &unlinkInode); // create the new file here

   if (insertRes == FhgfsOpsErr_SUCCESS)
   {
      // update backlinks
      bool backlinksEnabled = Program::getApp()->getConfig()->getStoreBacklinksEnabled();

      if (backlinksEnabled)
      {
         EntryInfo* toDirInfo = this->getToDirInfo();
         std::string newName  = this->getNewName();
         MsgHelperChunkBacklinks::updateBacklink(toDirInfo->getEntryID(), newName);
      }
   }

   // prepare response data
   unsigned inodeBufLen;
   char* inodeBuf;

   if(unlinkInode)
   {
      inodeBuf = (char*)malloc(META_SERBUF_SIZE);
      if (unlikely(!inodeBuf) )
      {  // out of memory, we are going to leak an inode and chunks
         inodeBufLen = 0;
         inodeBuf    = NULL;
         log.logErr(std::string("Malloc failed, leaking chunks for id: ") +
            unlinkInode->getEntryID() );
      }
      else
         inodeBufLen    = unlinkInode->serializeMetaData(inodeBuf);

      delete unlinkInode;
   }
   else
   {  // no file overwritten
      inodeBufLen = 0;
      inodeBuf    = NULL;
   }
   
   MovingFileInsertRespMsg respMsg(insertRes, inodeBufLen, inodeBuf);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );
   
   // cleanup
   SAFE_FREE_NOSET(inodeBuf);
      
   return true;
}


FhgfsOpsErr MovingFileInsertMsgEx::insert(Socket* socket, FileInode** outUnlinkedFile)
{
   MetaStore* metaStore = Program::getApp()->getMetaStore();

   EntryInfo* fromFileInfo = this->getFromFileInfo();
   EntryInfo* toDirInfo    = this->getToDirInfo();
   std::string newName     = this->getNewName();

   FhgfsOpsErr moveRes = metaStore->moveRemoteFileInsert(
      fromFileInfo, toDirInfo->getEntryID(), newName, getSerialBuf(), outUnlinkedFile,
      newFileInfo);
   if (moveRes != FhgfsOpsErr_SUCCESS)
      return moveRes;

   std::string xattrName;
   CharVector xattrValue;

   FhgfsOpsErr retVal = FhgfsOpsErr_SUCCESS;

   while (true)
   {
      retVal = getNextXAttr(socket, xattrName, xattrValue);
      if (retVal == FhgfsOpsErr_SUCCESS)
         break;
      else if (retVal != FhgfsOpsErr_AGAIN)
         goto xattr_error;

      retVal = MsgHelperXAttr::setxattr(&newFileInfo, xattrName, xattrValue, 0);
      if (retVal != FhgfsOpsErr_SUCCESS)
         goto xattr_error;

      xattrNames.push_back(xattrName);
   }

   return FhgfsOpsErr_SUCCESS;

xattr_error:
   MsgHelperUnlink::unlinkMetaFile(toDirInfo->getEntryID(), newName, NULL);

   return retVal;
}

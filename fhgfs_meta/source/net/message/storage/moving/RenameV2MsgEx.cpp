#include <common/net/message/control/GenericResponseMsg.h>
#include <common/net/message/storage/attribs/GetEntryInfoMsg.h>
#include <common/net/message/storage/attribs/GetEntryInfoRespMsg.h>
#include <common/net/message/storage/attribs/UpdateBacklinkMsg.h>
#include <common/net/message/storage/attribs/UpdateBacklinkRespMsg.h>
#include <common/net/message/storage/attribs/UpdateDirParentMsg.h>
#include <common/net/message/storage/attribs/UpdateDirParentRespMsg.h>
#include <common/net/message/storage/moving/MovingDirInsertMsg.h>
#include <common/net/message/storage/moving/MovingDirInsertRespMsg.h>
#include <common/net/message/storage/moving/MovingFileInsertMsg.h>
#include <common/net/message/storage/moving/MovingFileInsertRespMsg.h>
#include <common/net/message/storage/moving/RenameRespMsg.h>
#include <common/net/message/storage/creating/UnlinkLocalFileMsg.h>
#include <common/net/message/storage/creating/UnlinkLocalFileRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/MetadataTk.h>
#include <components/ModificationEventFlusher.h>
#include <net/msghelpers/MsgHelperChunkBacklinks.h>
#include <net/msghelpers/MsgHelperUnlink.h>
#include <net/msghelpers/MsgHelperXAttr.h>
#include <program/Program.h>
#include "RenameV2MsgEx.h"

bool RenameV2MsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   const char* logContext = "RenameV2Msg incoming";

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername(); 
   LOG_DEBUG(logContext, Log_DEBUG, std::string("Received a RenameV2Msg from: ") + peer);
   IGNORE_UNUSED_VARIABLE(logContext);

   MetaStore* metaStore = Program::getApp()->getMetaStore();
   ModificationEventFlusher* modEventFlusher = Program::getApp()->getModificationEventFlusher();
   bool modEventLoggingEnabled = modEventFlusher->isLoggingEnabled();

   FhgfsOpsErr renameRes;
   
   EntryInfo* fromDirInfo = this->getFromDirInfo();
   EntryInfo* toDirInfo   = this->getToDirInfo();
   std::string oldName    = this->getOldName();
   std::string newName    = this->getNewName();
   DirEntryType entryType = this->getEntryType();
   
   LOG_DEBUG(logContext, Log_DEBUG, "FromDirID: " + fromDirInfo->getEntryID() + "; "
      "oldName: '" + oldName + "'; "
      "ToDirID: " + toDirInfo->getEntryID() + "; "
      "newName: '" + newName + "'");

   std::string movedEntryID;
   std::string unlinkedEntryID;

   this->socket = sock;

   if ( modEventLoggingEnabled )
   {
      DirInode* parentDir = metaStore->referenceDir(fromDirInfo->getEntryID(), true);

      if ( parentDir )
      {
         EntryInfo entryInfo;
         parentDir->getEntryInfo(oldName, entryInfo);
         metaStore->releaseDir(parentDir->getID());

         movedEntryID = entryInfo.getEntryID();
      }
   }

   renameRes = moveFrom(fromDirInfo, oldName, entryType, toDirInfo, newName, unlinkedEntryID);

   if ( modEventLoggingEnabled )
   {
      if ( DirEntryType_ISDIR(entryType))
         modEventFlusher->add(ModificationEvent_DIRMOVED, movedEntryID);
      else
      {
         modEventFlusher->add(ModificationEvent_FILEMOVED, movedEntryID);
         if ( !unlinkedEntryID.empty() )
            modEventFlusher->add(ModificationEvent_FILEREMOVED, unlinkedEntryID);
      }
   }

   // send response

   if(unlikely(renameRes == FhgfsOpsErr_COMMUNICATION) )
   { // forward comm error as indirect communication error to client
      GenericResponseMsg respMsg(GenericRespMsgCode_INDIRECTCOMMERR,
         "Communication with other metadata server failed");
      respMsg.serialize(respBuf, bufLen);
      sock->sendto(respBuf, respMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );
   }
   else
   { // normal response
      RenameRespMsg respMsg(renameRes);
      respMsg.serialize(respBuf, bufLen);
      sock->sendto(respBuf, respMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );
   }

   // update operation counters
   Program::getApp()->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_RENAME,
      getMsgHeaderUserID() );

   return true;      
}

/**
 * Checks existence of the from-part and calls movingPerform().
 */
FhgfsOpsErr RenameV2MsgEx::moveFrom(EntryInfo* fromDirInfo, std::string& oldName,
   DirEntryType entryType, EntryInfo* toDirInfo, std::string& newName, std::string& unlinkedEntryID)
{
   MetaStore* metaStore = Program::getApp()->getMetaStore();
   
   // reference fromParent
   DirInode* fromParent = metaStore->referenceDir(fromDirInfo->getEntryID(), true);
   if(!fromParent)
      return FhgfsOpsErr_PATHNOTEXISTS;

   FhgfsOpsErr renameRes = movingPerform(fromParent, fromDirInfo, oldName, entryType,
      toDirInfo, newName, unlinkedEntryID);

   // clean-up
   metaStore->releaseDir(fromDirInfo->getEntryID() );
   
   return renameRes;
}

FhgfsOpsErr RenameV2MsgEx::movingPerform(DirInode* fromParent, EntryInfo* fromDirInfo,
   std::string& oldName, DirEntryType entryType, EntryInfo* toDirInfo, std::string& newName,
   std::string& unlinkedEntryID)
{
   const char* logContext = "RenameV2MsgEx::movingPerform";
   IGNORE_UNUSED_VARIABLE(logContext);

   App* app = Program::getApp();
   uint16_t localNodeID = app->getLocalNode()->getNumID();
   
   FhgfsOpsErr retVal = FhgfsOpsErr_INTERNAL;
   
   // is this node the owner of the fromParent dir?
   if(localNodeID != fromParent->getOwnerNodeID() )
      return FhgfsOpsErr_NOTOWNER;

   if (unlikely(entryType == DirEntryType_INVALID) )
   {
      LOG_DEBUG(logContext, Log_SPAM, "Received an invalid entry type!");
      return FhgfsOpsErr_INTERNAL;
  }

   if (checkRenameSimple(fromDirInfo, toDirInfo) )
   { // simple rename (<= not a move && everything local)
      LOG_DEBUG(logContext, Log_SPAM, "Method: rename in same dir"); // debug in

      retVal = renameInSameDir(fromParent, oldName, newName, unlinkedEntryID);
   }
   else
   if (entryType == DirEntryType_DIRECTORY)
      retVal = renameDir(fromParent, fromDirInfo, oldName, toDirInfo, newName);
   else
      retVal = renameFile(fromParent, fromDirInfo, oldName, toDirInfo, newName,
         unlinkedEntryID);
   
   return retVal;
}

/**
 * Rename a directory
 */
FhgfsOpsErr RenameV2MsgEx::renameDir(DirInode* fromParent, EntryInfo* fromDirInfo,
   std::string& oldName, EntryInfo* toDirInfo, std::string& newName)
{
   const char* logContext = "RenameV2MsgEx::renameDir";

   FhgfsOpsErr retVal = FhgfsOpsErr_INTERNAL;

   DirEntry fromDirEntry(oldName);
   bool dirEntryCopyRes = fromParent->getDirDentry(oldName, fromDirEntry);
   if (dirEntryCopyRes == false)
   {
      LOG_DEBUG("RenameV2MsgEx::movingPerform", Log_SPAM, "getDirEntryCopy() failed");
      return FhgfsOpsErr_NOTADIR;
   }

   // when we we verified the 'oldName' is really a directory

#if 0
   // TODO: Add local dir-rename/move again, but use the remote functions directly
   App* app = Program::getApp();
   uint16_t localNodeID = app->getLocalNode()->getNumID();

   if(localNodeID == toDirInfo->getOwnerNodeID() )
   { // local dir move (target parent dir is local)
      LOG_DEBUG("RenameV2MsgEx::movingPerform", 5, "Method: local dir move."); // debug in

      retVal = moveLocalDirTo(fromParent, oldName, toDirInfo, newName);

      // TODO trac #124 - possible renameat() race
   }
   else
#endif
   { // remote dir move (target parent dir owned by another node)

      LOG_DEBUG(logContext, Log_SPAM, "Method: remote dir move."); // debug in

      // prepare local part of the move operation
      char* serialBuf = (char*)malloc(META_SERBUF_SIZE);

      /* Put all meta data of this dentry into the given buffer. The buffer then will be
       * used on the remote side to fill the new dentry */
      size_t usedBufLen = fromDirEntry.serializeDentry(serialBuf);

      { // remote insertion
         retVal = remoteDirInsert(toDirInfo, newName, serialBuf, usedBufLen );


         // finish local part of the move operation
         if(retVal == FhgfsOpsErr_SUCCESS)
         {
            DirEntry* rmDirEntry;

            retVal = fromParent->removeDir(oldName, &rmDirEntry);

            if (retVal == FhgfsOpsErr_SUCCESS)
            {
               std::string parentID = fromParent->getID();
               EntryInfo removedInfo;

               rmDirEntry->getEntryInfo(parentID, 0, &removedInfo);

               updateRenamedDirInode(&removedInfo, toDirInfo);

            }
            else
            {
               // TODO: We now need to undo the remote DirInsert.

               LogContext(logContext).log(Log_CRITICAL,
                  std::string("Failed to remove fromDir: ") + oldName +
                  std::string(" Error: ") + FhgfsOpsErrTk::toErrString(retVal) );
            }

            SAFE_DELETE(rmDirEntry);

         }

      }

      // clean up
      free(serialBuf);
   }

   return retVal;
}

/**
 * Rename a file
 */
FhgfsOpsErr RenameV2MsgEx::renameFile(DirInode* fromParent, EntryInfo* fromDirInfo,
   std::string& oldName, EntryInfo* toDirInfo, std::string& newName, std::string& unlinkedEntryID)
{
   const char* logContext = "RenameV2MsgEx::renameFile";
   App* app = Program::getApp();
   MetaStore* metaStore = app->getMetaStore();

   FhgfsOpsErr retVal = FhgfsOpsErr_INTERNAL;

   EntryInfo fromFileEntryInfo;
   bool getRes = fromParent->getFileEntryInfo(oldName, fromFileEntryInfo);
   if(getRes == false )
   {
      LOG_DEBUG(logContext, Log_SPAM, "Error: fromDir does not exist.");
      return FhgfsOpsErr_PATHNOTEXISTS;
   }

   // when we are here we verified the file to be renamed is really a file


   // the buffer is used to transfer all data of the data of the dir-entry
   char* serialBuf = (char*)malloc(META_SERBUF_SIZE);
   size_t usedSerialBufLen;
   StringVector xattrNames;

   retVal = metaStore->moveRemoteFileBegin(
      fromParent, &fromFileEntryInfo, serialBuf, META_SERBUF_SIZE, &usedSerialBufLen);
   if (retVal != FhgfsOpsErr_SUCCESS)
   {
      free(serialBuf);
      return retVal;
   }

   // TODO: call moveRemoteFileInsert() if toDir is on the same server
#if 0
   uint16_t localNodeID = app->getLocalNode()->getNumID();
   if(localNodeID == toDirInfo->getOwnerNodeID() )
   { // local file move (target parent dir is local)

      LOG_DEBUG(logContext, Log_SPAM, "Method: local file move."); // debug in

      retVal = moveLocalFileTo(fromParent, oldName, toDirInfo, newName);
   }
   else
#endif
   { // remote file move (target parent dir owned by another node)

      LOG_DEBUG(logContext, Log_SPAM, "Method: remote file move."); // debug in

      if (app->getConfig()->getStoreClientXAttrs())
      {
         FhgfsOpsErr listXAttrRes = MsgHelperXAttr::listxattr(&fromFileEntryInfo, xattrNames);
         if (listXAttrRes != FhgfsOpsErr_SUCCESS)
         {
            metaStore->moveRemoteFileComplete(fromParent, fromFileEntryInfo.getEntryID());
            return FhgfsOpsErr_TOOBIG;
         }
      }

      // Do the remote operation (insert and possible unlink of an existing toFile)
      retVal = remoteFileInsertAndUnlink(&fromFileEntryInfo, toDirInfo, newName, serialBuf,
         usedSerialBufLen, xattrNames, unlinkedEntryID);


      // finish local part of the owned file move operation (+ remove local file-link)
      if(retVal == FhgfsOpsErr_SUCCESS)
      {
         // We are not interested in the inode here , as we are not going to delete storage chunks.
         EntryInfo entryInfo;
         FhgfsOpsErr unlinkRes = metaStore->unlinkFile(fromParent->getID(), oldName, &entryInfo,
            NULL);

         if (unlikely (unlinkRes) )
         {
            /* TODO: We now need to undo the remote FileInsert. If the user removes either
             *       entry herself, the other entry will be invalid, as the chunk files already
             *       will be removed on the first unlink. */

               LogContext(logContext).logErr(std::string("Error: Failed to unlink fromFile: ") +
                  "DirID: " + fromFileEntryInfo.getParentEntryID() + " "
                  "entryName: " + oldName + ". " +
                  "Remote toFile was successfully created.");
         }
      }

      metaStore->moveRemoteFileComplete(fromParent, fromFileEntryInfo.getEntryID() );


      // clean up
      free(serialBuf);
   }

   return retVal;
}


/**
 * Checks whether the requested rename operation is not actually a move operation, but really
 * just a simple rename (in which case nothing is moved to a new directory)
 * 
 * @return true if it is a simple rename (no moving involved)
 */ 
bool RenameV2MsgEx::checkRenameSimple(EntryInfo* fromDirInfo, EntryInfo* toDirInfo)
{
   if (fromDirInfo->getEntryID() == toDirInfo->getEntryID() )
      return true;

   return false;
}

/**
 * Renames a directory or a file (no moving between different directories involved).
 *
 * Note: We do not need to update chunk backlinks here, as nothing relevant to an
 *       for those will be modified here.
 */
FhgfsOpsErr RenameV2MsgEx::renameInSameDir(DirInode* fromParent, std::string oldName,
   std::string toName, std::string& unlinkedEntryID)
{
   const char* logContext = "RenameV2MsgEx::renameInSameDir";
   MetaStore* metaStore = Program::getApp()->getMetaStore();

   /* we are passing here the very same fromParent pointer also a toParent pointer, which is
    * essential in order not to dead-lock */

   FileInode* unlinkInode; // inode belong to a possibly existing toName file

   FhgfsOpsErr renameRes = metaStore->renameInSameDir(fromParent, oldName, toName,
      &unlinkInode);

   if (renameRes == FhgfsOpsErr_SUCCESS)
   {
      // handle unlinkInode
      if (unlinkInode)
      {
         unlinkedEntryID = unlinkInode->getEntryID();

         FhgfsOpsErr chunkUnlinkRes = MsgHelperUnlink::unlinkChunkFiles(
            unlinkInode, getMsgHeaderUserID() );

         if (chunkUnlinkRes != FhgfsOpsErr_SUCCESS)
         {
            LogContext(logContext).logErr(std::string("Rename succeeded, but unlinking storage ") +
            "chunk files of the overwritten targetFileName (" + toName + ") failed. " +
            "Entry-ID: " + unlinkedEntryID);

            // we can't do anything about it, so we won't even inform the user
         }
      }
   }

   return renameRes;
}

/**
 * Note: This method not only sends the insertion message, but also unlinks an overwritten local
 * file if it is contained in the response.
 *
 * @param serialBuf   the inode values serialized into this buffer.
 */
FhgfsOpsErr RenameV2MsgEx::remoteFileInsertAndUnlink(EntryInfo* fromFileInfo, EntryInfo* toDirInfo,
   std::string newName, char* serialBuf, size_t serialBufLen, StringVector& xattrs,
   std::string& unlinkedEntryID)
{
   LogContext log("RenameV2MsgEx::remoteFileInsert");
   
   FhgfsOpsErr retVal = FhgfsOpsErr_SUCCESS;
   
   App* app = Program::getApp();
   uint16_t toNodeID = toDirInfo->getOwnerNodeID();

   LOG_DEBUG_CONTEXT(log, Log_DEBUG,
      "Inserting remote parentID: " + toDirInfo->getEntryID() + "; "
      "newName: '" + newName + "'; "
      "entryID: '" + fromFileInfo->getEntryID() + "'; "
      "node: " + StringTk::uintToStr(toNodeID) );

   // prepare request

   MovingFileInsertMsg insertMsg(fromFileInfo, toDirInfo, newName, serialBuf, serialBufLen);

   RequestResponseArgs rrArgs(NULL, &insertMsg, NETMSGTYPE_MovingFileInsertResp);

   RequestResponseNode rrNode(toNodeID, app->getMetaNodes() );
   rrNode.setTargetStates(app->getMetaStateStore() );

   // send request and receive response

   MsgHelperXAttr::StreamXAttrState streamState = { socket, fromFileInfo, &xattrs };
   insertMsg.registerStreamoutHook(rrArgs, MsgHelperXAttr::streamXAttrFn, &streamState);

   FhgfsOpsErr requestRes = MessagingTk::requestResponseNode(&rrNode, &rrArgs);

   if(requestRes != FhgfsOpsErr_SUCCESS)
   { // communication error
      log.log(Log_WARNING,
         "Communication with metadata sever failed. "
         "nodeID: " + StringTk::uintToStr(toNodeID) );
      return requestRes;
   }

   // correct response type received

   MovingFileInsertRespMsg* insertRespMsg = (MovingFileInsertRespMsg*)rrArgs.outRespMsg;
   
   retVal = insertRespMsg->getResult();
   
   unsigned unlinkedInodeBufLen = insertRespMsg->getInodeBufLen();
   if (unlinkedInodeBufLen)
   {
      const char* unlinkedInodeBuf = insertRespMsg->getInodeBuf();
      FileInode* toUnlinkInode = new FileInode();

      bool deserialRes = toUnlinkInode->deserializeMetaData(unlinkedInodeBuf);
      if(unlikely(!deserialRes) )
      { // deserialization of received inode failed (should never happen)
         log.logErr("Failed to deserialize unlinked file inode. "
            "nodeID: " + StringTk::uintToStr(toNodeID) );
         delete(toUnlinkInode);
      }
      else
      {
         MsgHelperUnlink::unlinkChunkFiles(
            toUnlinkInode, getMsgHeaderUserID() ); // destructs toUnlinkInode
      }
   }

   if(retVal != FhgfsOpsErr_SUCCESS)
   { // error: remote file not inserted
      LOG_DEBUG_CONTEXT(log, Log_NOTICE,
         "Metadata server was unable to insert file. "
         "nodeID: " + StringTk::uintToStr(toNodeID) + "; "
         "Error: " + FhgfsOpsErrTk::toErrString(retVal) );
   }
   else
   {
      // success: remote file inserted
      LOG_DEBUG_CONTEXT(log, Log_DEBUG, "Metadata server inserted file. "
         "nodeID: " + StringTk::uintToStr(toNodeID) );
   }

   return retVal;
}

/**
 * Insert a remote directory dir-entry
 */
FhgfsOpsErr RenameV2MsgEx::remoteDirInsert(EntryInfo* toDirInfo, std::string& newName,
   char* serialBuf, size_t serialBufLen)
{
   LogContext log("RenameV2MsgEx::remoteDirInsert");
   
   FhgfsOpsErr retVal = FhgfsOpsErr_SUCCESS;

   App* app = Program::getApp();
   uint16_t toNodeID = toDirInfo->getOwnerNodeID();

   LOG_DEBUG_CONTEXT(log, Log_DEBUG,
      "Inserting remote parentID: " + toDirInfo->getEntryID() + "; "
      "newName: '" + newName + "'; "
      "node: " + StringTk::uintToStr(toNodeID) );

   // prepare request

   MovingDirInsertMsg insertMsg(toDirInfo, newName, serialBuf, serialBufLen);

   RequestResponseArgs rrArgs(NULL, &insertMsg, NETMSGTYPE_MovingDirInsertResp);

   RequestResponseNode rrNode(toNodeID, app->getMetaNodes() );
   rrNode.setTargetStates(app->getMetaStateStore() );

   // send request and receive response

   FhgfsOpsErr requestRes = MessagingTk::requestResponseNode(&rrNode, &rrArgs);

   if(requestRes != FhgfsOpsErr_SUCCESS)
   { // communication error
      log.log(Log_WARNING, "Communication with metadata server failed. "
         "nodeID: " + StringTk::uintToStr(toNodeID) );
      return requestRes;
   }

   // correct response type received
   MovingDirInsertRespMsg* insertRespMsg = (MovingDirInsertRespMsg*)rrArgs.outRespMsg;

   retVal = insertRespMsg->getResult();
   if(retVal != FhgfsOpsErr_SUCCESS)
   { // error: remote dir not inserted
      LOG_DEBUG_CONTEXT(log, Log_NOTICE,
         "Metdata server was unable to insert directory. "
         "nodeID: " + StringTk::uintToStr(toNodeID) + "; "
         "Error: " + FhgfsOpsErrTk::toErrString(retVal) );
   }
   else
   {
      // success: remote dir inserted
      LOG_DEBUG_CONTEXT(log, Log_DEBUG,
         "Metadata server inserted directory. "
         "nodeID: " + StringTk::uintToStr(toNodeID) );
   }

   return retVal;
}

/**
 * Set the new parent information to the inode of renamedDirEntryInfo
 */
FhgfsOpsErr RenameV2MsgEx::updateRenamedDirInode(EntryInfo* renamedDirEntryInfo,
   EntryInfo* toDirInfo)
{
   LogContext log("RenameV2MsgEx::updateRenamedDirInode");

   std::string parentEntryID = toDirInfo->getEntryID();
   renamedDirEntryInfo->setParentEntryID(parentEntryID);

   App* app = Program::getApp();
   uint16_t toNodeID = renamedDirEntryInfo->getOwnerNodeID();

   LOG_DEBUG_CONTEXT(log, Log_DEBUG,
      "Update remote inode: " + renamedDirEntryInfo->getEntryID() + "; "
      "node: " + StringTk::uintToStr(toNodeID) );

   // prepare request

   UpdateDirParentMsg updateMsg(renamedDirEntryInfo, toDirInfo->getOwnerNodeID() );

   RequestResponseArgs rrArgs(NULL, &updateMsg, NETMSGTYPE_UpdateDirParentResp);

   RequestResponseNode rrNode(toNodeID, app->getMetaNodes() );
   rrNode.setTargetStates(app->getMetaStateStore() );

   // send request and receive response

   FhgfsOpsErr requestRes = MessagingTk::requestResponseNode(&rrNode, &rrArgs);

   if(requestRes != FhgfsOpsErr_SUCCESS)
   { // communication error
      log.log(Log_WARNING,
         "Communication with metadata server failed. "
         "nodeID: " + StringTk::uintToStr(toNodeID) );
      return requestRes;
   }

   // correct response type received
   UpdateDirParentRespMsg* updateRespMsg = (UpdateDirParentRespMsg*)rrArgs.outRespMsg;

   FhgfsOpsErr retVal = (FhgfsOpsErr)updateRespMsg->getValue();
   if(retVal != FhgfsOpsErr_SUCCESS)
   { // error
      LOG_DEBUG_CONTEXT(log, Log_NOTICE,
         "Failed to update ParentEntryID: " + renamedDirEntryInfo->getEntryID() + "; "
         "nodeID: " + StringTk::uintToStr(toNodeID) + "; "
         "Error: " + FhgfsOpsErrTk::toErrString(retVal) );
   }

   return retVal;
}

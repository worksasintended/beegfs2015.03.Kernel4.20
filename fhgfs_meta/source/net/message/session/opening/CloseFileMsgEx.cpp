#include <common/components/streamlistenerv2/IncomingPreprocessedMsgWork.h>
#include <common/net/message/control/GenericResponseMsg.h>
#include <common/net/message/session/opening/CloseFileRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/SessionTk.h>
#include <program/Program.h>
#include <storage/MetaStore.h>
#include <net/msghelpers/MsgHelperClose.h>
#include <net/msghelpers/MsgHelperLocking.h>
#include "CloseFileMsgEx.h"


bool CloseFileMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   #ifdef BEEGFS_DEBUG
      const char* logContext = "CloseFileMsg incoming";

      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, Log_DEBUG, std::string("Received a CloseFileMsg from: ") + peer);
   #endif // BEEGFS_DEBUG

   App* app = Program::getApp();

   FhgfsOpsErr closeRes;
   bool unlinkDisposalFile = false;

   EntryInfo* entryInfo = getEntryInfo();

   // update operation counters (here on top because we have an early sock release in this msg)
   app->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_CLOSE,
      getMsgHeaderUserID() );


   if(isMsgHeaderFeatureFlagSet(CLOSEFILEMSG_FLAG_CANCELAPPENDLOCKS) )
   { // client requests cleanup of granted or pending locks for this handle

      unsigned ownerFD = SessionTk::ownerFDFromHandleID(getFileHandleID() );
      EntryLockDetails lockDetails(getSessionID(), 0, 0, "", ENTRYLOCKTYPE_CANCEL);

      MsgHelperLocking::flockAppend(entryInfo, ownerFD, lockDetails);
   }


   /* two alternatives:
         1) early response before chunk file close (if client isn't interested in chunks result).
         2) normal response after chunk file close. */

   if(isMsgHeaderFeatureFlagSet(CLOSEFILEMSG_FLAG_EARLYRESPONSE) )
   { // alternative 1: client requests an early response

      /* note: linux won't even return the vfs release() result to userspace apps, so there's
         usually no point in waiting for the chunk file close result before sending the response */

      unsigned accessFlags;
      FileInode* inode;

      closeRes = MsgHelperClose::closeSessionFile(
         getSessionID(), getFileHandleID(), entryInfo, &accessFlags, &inode);

      // send response
      CloseFileRespMsg respMsg(closeRes);
      respMsg.serialize(respBuf, bufLen);
      sock->sendto(respBuf, respMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

      IncomingPreprocessedMsgWork::releaseSocket(app, &sock, this);

      if(likely(closeRes == FhgfsOpsErr_SUCCESS) )
         closeRes = closeFileAfterEarlyResponse(inode, accessFlags, &unlinkDisposalFile);
   }
   else
   { // alternative 2: normal response (after chunk file close)

      closeRes = MsgHelperClose::closeFile(getSessionID(), getFileHandleID(),
         entryInfo, getMaxUsedNodeIndex(), getMsgHeaderUserID(), &unlinkDisposalFile);

      // send response
      // (caller won't be interested in the unlink result below, so no reason to wait for that)

      if(unlikely(closeRes == FhgfsOpsErr_COMMUNICATION) )
      { // forward comm error as indirect communication error to client
         GenericResponseMsg respMsg(GenericRespMsgCode_INDIRECTCOMMERR,
            "Communication with storage target failed");
         respMsg.serialize(respBuf, bufLen);
         sock->sendto(respBuf, respMsg.getMsgLength(), 0,
            (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );
      }
      else
      { // normal response
         CloseFileRespMsg respMsg(closeRes);
         respMsg.serialize(respBuf, bufLen);
         sock->sendto(respBuf, respMsg.getMsgLength(), 0,
            (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );
      }
   }


   // unlink if file marked as disposable
   if( (closeRes == FhgfsOpsErr_SUCCESS) && unlinkDisposalFile)
   { // check whether file has been unlinked (and perform the unlink operation on last close)

      /* note: we do this only if also the chunk file close succeeded, because if storage servers
         are down, unlinkDisposableFile() will keep the file in the disposal store anyways */
      
      MsgHelperClose::unlinkDisposableFile(entryInfo->getEntryID(), getMsgHeaderUserID() );
   }
   

   return true;
}

/**
 * The rest after MsgHelperClose::closeSessionFile(), i.e. MsgHelperClose::closeChunkFile()
 * and metaStore->closeFile().
 *
 * @param inode inode for closed file (as returned by MsgHelperClose::closeSessionFile() )
 * @param maxUsedNodeIndex zero-based index, -1 means "none"
 * @param outFileWasUnlinked true if the hardlink count of the file was 0
 */
FhgfsOpsErr CloseFileMsgEx::closeFileAfterEarlyResponse(FileInode* inode, unsigned accessFlags,
   bool* outUnlinkDisposalFile)
{
   MetaStore* metaStore = Program::getApp()->getMetaStore();

   unsigned numHardlinks;
   unsigned numInodeRefs;

   *outUnlinkDisposalFile = false;


   FhgfsOpsErr chunksRes = MsgHelperClose::closeChunkFile(
      getSessionID(), getFileHandleID(), getMaxUsedNodeIndex(), inode, getEntryInfo(),
      getMsgHeaderUserID() );


   metaStore->closeFile(getEntryInfo(), inode, accessFlags, &numHardlinks, &numInodeRefs);


   if (!numHardlinks && !numInodeRefs)
      *outUnlinkDisposalFile = true;

   return chunksRes;
}

#include <common/net/message/session/locking/FLockEntryRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/SessionTk.h>
#include <net/msghelpers/MsgHelperLocking.h>
#include <program/Program.h>
#include <session/SessionStore.h>
#include <storage/MetaStore.h>
#include "FLockEntryMsgEx.h"


bool FLockEntryMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   /* note: this code is very similar to FLockRangeMsgEx::processIncoming(), so if you change
      something here, you probably want to change it there, too. */

   #ifdef BEEGFS_DEBUG
      const char* logContext = "FLockEntryMsg incoming";

      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, Log_DEBUG, std::string("Received a FLockEntryMsg from: ") + peer);
   #endif // BEEGFS_DEBUG

   App* app = Program::getApp();
   SessionStore* sessions = app->getSessions();
   MetaStore* metaStore = app->getMetaStore();

   FhgfsOpsErr clientResult = FhgfsOpsErr_INTERNAL; // reponse error code

   unsigned ownerFD = SessionTk::ownerFDFromHandleID(getFileHandleID() );

   EntryLockDetails lockDetails(getClientID(), getClientFD(), getOwnerPID(), getLockAckID(),
      getLockTypeFlags() );

   LOG_DEBUG(logContext, Log_SPAM, lockDetails.toString() );

   EntryInfo* entryInfo = getEntryInfo();

   // find sessionFile
   Session* session = sessions->referenceSession(getClientID(), true);
   SessionFileStore* sessionFiles = session->getFiles();

   SessionFile* sessionFile = sessionFiles->referenceSession(ownerFD);
   if(!sessionFile)
   { // sessionFile not exists (mds restarted?)

      // check if this is just an UNLOCK REQUEST

      if(getLockTypeFlags() & ENTRYLOCKTYPE_UNLOCK)
      { // it's an unlock => we'll just ignore it (since the locks are gone anyways)
         clientResult = FhgfsOpsErr_SUCCESS;

         goto cleanup_session;
      }

      // check if this is a LOCK CANCEL REQUEST

      if(getLockTypeFlags() & ENTRYLOCKTYPE_CANCEL)
      { // it's a lock cancel
         /* this is an important special case, because client might have succeeded in closing the
         file but the conn might have been interrupted during unlock, so we definitely have to try
         canceling the lock here */

         // if the file still exists, just do the lock cancel without session recovery attempt

         FileInode* lockCancelFile = metaStore->referenceFile(entryInfo);
         if(lockCancelFile)
         {
            lockCancelFile->flockEntry(lockDetails);
            metaStore->releaseFile(entryInfo->getParentEntryID(), lockCancelFile);
         }

         clientResult = FhgfsOpsErr_SUCCESS;

         goto cleanup_session;
      }

      // it's a LOCK REQUEST => try to recover session file to do the locking

      clientResult = MsgHelperLocking::trySesssionRecovery(entryInfo, getClientID(),
         ownerFD, sessionFiles, &sessionFile);

      // (note: sessionFile==NULL now if recovery was not successful)

   } // end of session file session recovery attempt

   if(sessionFile)
   { // sessionFile exists (or was successfully recovered)
      FileInode* file = sessionFile->getInode();
      bool lockGranted = file->flockEntry(lockDetails);
      if(!lockGranted)
         clientResult = FhgfsOpsErr_WOULDBLOCK;
      else
         clientResult = FhgfsOpsErr_SUCCESS;

      // cleanup
      sessionFiles->releaseSession(sessionFile, entryInfo);
   }


   // cleanup
cleanup_session:
   sessions->releaseSession(session);

   LOG_DEBUG(logContext, Log_SPAM, std::string("Client result: ") +
      FhgfsOpsErrTk::toErrString(clientResult) );

   // send response
   FLockEntryRespMsg respMsg(clientResult);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   app->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_FLOCKENTRY,
      getMsgHeaderUserID() );

   return true;
}


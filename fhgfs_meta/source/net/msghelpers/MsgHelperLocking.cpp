#include <program/Program.h>
#include "MsgHelperLocking.h"


/**
 * Try to recover a file session that got lost (e.g. due to a mds restart) and was implicitly
 * reported to exist by a client, e.g. during a flock request. The session will be inserted into
 * the store.
 *
 * Note: We re-open the file in r+w mode here, because we don't know the orig mode.
 *
 * @param outSessionFile will be set to referenced (recovered) session if success is returned,
 * NULL otherwise
 */
FhgfsOpsErr MsgHelperLocking::trySesssionRecovery(EntryInfo* entryInfo, const char* clientID,
   unsigned ownerFD, SessionFileStore* sessionFiles, SessionFile** outSessionFile)
{
   const char* logContext = "MsgHelperLocking (try session recovery)";

   App* app = Program::getApp();
   MetaStore* metaStore = app->getMetaStore();

   LogContext(logContext).log(Log_WARNING, std::string("Attempting recovery of file session ") +
      "(session: " + clientID                      + "; "
      "handle: " + StringTk::uintToStr(ownerFD)    + "; "
      "parentID: " + entryInfo->getParentEntryID() + "; "
      "entryID: "  + entryInfo->getEntryID()       + ")" );

   *outSessionFile = NULL;

   FileInode* recoveryFile;
   unsigned recoveryAccessFlags = OPENFILE_ACCESS_READWRITE; /* (r+w is our only option, since we
      don't know the original flags) */

   FhgfsOpsErr openRes = metaStore->openFile(entryInfo, recoveryAccessFlags, &recoveryFile);
   if(openRes != FhgfsOpsErr_SUCCESS)
   { // file could not be opened => there's nothing we can do in this case
      LogContext(logContext).log(Log_WARNING, std::string("Recovery of file session failed: ") +
         FhgfsOpsErrTk::toErrString(openRes) );

      return openRes;
   }
   else
   { // file opened => try to insert the recovery file session
      SessionFile* recoverySessionFile = new SessionFile(recoveryFile, recoveryAccessFlags,
         entryInfo);
      recoverySessionFile->setSessionID(ownerFD);

      *outSessionFile = sessionFiles->addAndReferenceRecoverySession(recoverySessionFile);

      if(!*outSessionFile)
      { // bad, our recovery session ID was used by someone in the meantime => cleanup
         unsigned numHardlinks; // ignored here
         unsigned numInodeRefs; // ignored here

         LogContext(logContext).log(Log_WARNING,
            "Recovery of file session failed: SessionID is in use by another file now.");

         metaStore->closeFile(entryInfo, recoveryFile, recoveryAccessFlags, &numHardlinks,
            &numInodeRefs);

         delete(recoverySessionFile);

         return FhgfsOpsErr_INTERNAL;
      }
      else
      { // recovery succeeded
         LogContext(logContext).log(Log_NOTICE, "File session recovered.");
      }
   }


   return FhgfsOpsErr_SUCCESS;
}

/**
 * Note: This method also tries session recovery for lock requests.
 */
FhgfsOpsErr MsgHelperLocking::flockAppend(EntryInfo* entryInfo, unsigned ownerFD,
   EntryLockDetails& lockDetails)
{
   App* app = Program::getApp();
   SessionStore* sessions = app->getSessions();
   MetaStore* metaStore = app->getMetaStore();

   FhgfsOpsErr retVal = FhgfsOpsErr_INTERNAL;


   // find sessionFile
   Session* session = sessions->referenceSession(lockDetails.getClientID(), true);
   SessionFileStore* sessionFiles = session->getFiles();

   SessionFile* sessionFile = sessionFiles->referenceSession(ownerFD);
   if(!sessionFile)
   { // sessionFile not exists (mds restarted?)

      // check if this is just an UNLOCK REQUEST

      if(lockDetails.isUnlock() )
      { // it's an unlock => we'll just ignore it (since the locks are gone anyways)
         retVal = FhgfsOpsErr_SUCCESS;

         goto cleanup_session;
      }

      // check if this is a LOCK CANCEL REQUEST

      if(lockDetails.isCancel() )
      { // it's a lock cancel
         /* this is an important special case, because client might have succeeded in closing the
         file but the conn might have been interrupted during unlock, so we definitely have to try
         canceling the lock here */

         // if the file still exists, just do the lock cancel without session recovery attempt

         FileInode* lockCancelFile = metaStore->referenceFile(entryInfo);
         if(lockCancelFile)
         {
            lockCancelFile->flockAppend(lockDetails);
            metaStore->releaseFile(entryInfo->getParentEntryID(), lockCancelFile);
         }

         retVal = FhgfsOpsErr_SUCCESS;

         goto cleanup_session;
      }

      // it's a LOCK REQUEST => try to recover session file to do the locking

      retVal = MsgHelperLocking::trySesssionRecovery(entryInfo, lockDetails.getClientID().c_str(),
         ownerFD, sessionFiles, &sessionFile);

      // (note: sessionFile==NULL now if recovery was not successful)

   } // end of session file session recovery attempt

   if(sessionFile)
   { // sessionFile exists (or was successfully recovered)
      FileInode* file = sessionFile->getInode();
      bool lockGranted = file->flockAppend(lockDetails);
      if(!lockGranted)
         retVal = FhgfsOpsErr_WOULDBLOCK;
      else
         retVal = FhgfsOpsErr_SUCCESS;

      // cleanup
      sessionFiles->releaseSession(sessionFile, entryInfo);
   }


   // cleanup
cleanup_session:
   sessions->releaseSession(session);

   return retVal;
}

#include "ClientSyncer.h"

#include <common/toolkit/SessionTk.h>
#include <common/toolkit/NodesTk.h>
#include <components/InternodeSyncer.h>
#include <net/msghelpers/MsgHelperClose.h>
#include <program/Program.h>
#include <app/App.h>

ClientSyncer::ClientSyncer() throw (ComponentInitException)
   : PThread("ClientSync"), log("ClientSync"), downloadForced(false)
{ }

void ClientSyncer::run()
{
   try
   {
      registerSignalHandler();
      syncLoop();
      log.log(Log_DEBUG, "Component stopped.");
   }
   catch (std::exception& e)
   {
      PThread::getCurrentThreadApp()->handleComponentException(e);
   }
}


void ClientSyncer::syncLoop()
{
   const int sleepTimeMS = 5000;

   const unsigned downloadClientsIntervalMS = 300000;

   Time lastDownloadClientsT;

   while (!waitForSelfTerminateOrder(sleepTimeMS))
   {
      if (getAndResetDownloadForced() ||
            lastDownloadClientsT.elapsedMS() > downloadClientsIntervalMS)
      {
         downloadAndSyncClients();
         lastDownloadClientsT.setToNow();
      }
   }
}

void ClientSyncer::downloadAndSyncClients()
{
   App* app = Program::getApp();
   NodeStoreServersEx* mgmtNodes = app->getMgmtNodes();

   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   if(!mgmtNode)
      return;

   NodeStoreClientsEx* clientNodes = Program::getApp()->getClientNodes();
   Node* localNode = app->getLocalNode();

   NodeList clientNodesList;
   StringList addedClientNodes;
   StringList removedClientNodes;

   if(NodesTk::downloadNodes(mgmtNode, NODETYPE_Client, &clientNodesList, true) )
   {
      /* copy the nodesList, because syncNodes() and syncClients() both will remove all elements
         from their lists */
      NodeList clientNodesListCopy;
      NodesTk::copyListNodes(&clientNodesList, &clientNodesListCopy);

      clientNodes->syncNodes(&clientNodesList, &addedClientNodes, &removedClientNodes, true,
         localNode);

      syncClients(&clientNodesListCopy, true); // sync client sessions
   }
}

void ClientSyncer::forceDownload()
{
   SafeMutexLock lock(&downloadForcedMutex);
   downloadForced = true;
   lock.unlock();
}

/**
 * Synchronize local client sessions with registered mgmtd clients to release orphaned sessions.
 *
 * @param clientsList must be ordered; contained nodes will be removed and may no longer be
 * accessed after calling this method.
 * @param allowRemoteComm usually true; setting this to false is only useful when called during
 * app shutdown to avoid communication; if false, unlocking of user locks, closing of storage server
 * files and disposal of unlinked files won't be performed
 */
void ClientSyncer::syncClients(NodeList* clientsList, bool allowRemoteComm)
{
   App* app = Program::getApp();
   MetaStore* metaStore = Program::getApp()->getMetaStore();
   HeartbeatManager* hbmanager = Program::getApp()->getHeartbeatMgr();
   SessionStore* sessions = app->getSessions();

   SessionList removedSessions;
   StringList unremovableSessions;

   bool removeClients = hbmanager->isMgmtInitDone();
   sessions->syncSessions(clientsList, removeClients, &removedSessions, &unremovableSessions);

   // print client session removal results (upfront)
   if(!removedSessions.empty() || !unremovableSessions.empty() )
   {
      std::ostringstream logMsgStream;
      logMsgStream << "Removing " << removedSessions.size() << " client sessions. ";

      if(unremovableSessions.empty() )
         log.log(Log_DEBUG, logMsgStream.str() ); // no unremovable sessions
      else
      { // unremovable sessions found => log warning
         logMsgStream << "(" << unremovableSessions.size() << " are unremovable)";
         log.log(Log_WARNING, logMsgStream.str() );
      }
   }


   // walk over all removed sessions (to cleanup the contained files)

   SessionListIter sessionIter = removedSessions.begin();
   for( ; sessionIter != removedSessions.end(); sessionIter++)
   { // walk over all client sessions: cleanup each session
      Session* session = *sessionIter;
      std::string sessionID = session->getSessionID();
      SessionFileStore* sessionFiles = session->getFiles();

      SessionFileList removedSessionFiles;
      UIntList referencedSessionFiles;

      sessionFiles->removeAllSessions(&removedSessionFiles, &referencedSessionFiles);

      /* note: referencedSessionFiles should always be empty, because otherwise the reference holder
         would also hold a reference to the client session (and we woudn't be here if the client
         session had any references) */


      // print session files results (upfront)

      if(removedSessionFiles.size() || referencedSessionFiles.size() )
      {
         std::ostringstream logMsgStream;
         logMsgStream << sessionID << ": Removing " << removedSessionFiles.size() <<
            " file sessions. " << "(" << referencedSessionFiles.size() << " are unremovable)";
         log.log(Log_NOTICE, logMsgStream.str() );
      }


      // walk over all files in the current session (to clean them up)

      SessionFileListIter fileIter = removedSessionFiles.begin();

      for( ; fileIter != removedSessionFiles.end(); fileIter++)
      { // walk over all files: unlock user locks, close meta, close local, dispose unlinked
         SessionFile* sessionFile = *fileIter;
         unsigned ownerFD = sessionFile->getSessionID();
         unsigned accessFlags = sessionFile->getAccessFlags();
         unsigned numHardlinks;
         unsigned numInodeRefs;

         FileInode* inode = sessionFile->getInode();
         std::string fileID = inode->getEntryID();

         std::string fileHandleID = SessionTk::generateFileHandleID(ownerFD, fileID);

         // save nodeIDs for later
         StripePattern* pattern = inode->getStripePattern();
         int maxUsedNodesIndex = pattern->getStripeTargetIDs()->size() - 1;

         if(allowRemoteComm)
         { // unlock all user locks
            inode->flockAppendCancelByClientID(sessionID);
            inode->flockEntryCancelByClientID(sessionID);
            inode->flockRangeCancelByClientID(sessionID);
         }

         EntryInfo* entryInfo = sessionFile->getEntryInfo();

         if(allowRemoteComm)
            MsgHelperClose::closeChunkFile(sessionID.c_str(), fileHandleID.c_str(),
               maxUsedNodesIndex, inode, entryInfo, NETMSG_DEFAULT_USERID);

         log.log(Log_NOTICE, std::string("closing file ") + "ParentID: " +
            entryInfo->getParentEntryID() + " FileName: " + entryInfo->getFileName() );

         metaStore->closeFile(entryInfo, inode, accessFlags, &numHardlinks, &numInodeRefs);

         delete(sessionFile);

         if(allowRemoteComm && !numHardlinks && !numInodeRefs)
            MsgHelperClose::unlinkDisposableFile(fileID, NETMSG_DEFAULT_USERID);
      } // end of files loop


      delete(session);

   } // end of client sessions loop
}

bool ClientSyncer::getAndResetDownloadForced()
{
   SafeMutexLock lock(&downloadForcedMutex);
   const bool res = downloadForced;
   downloadForced = false;
   lock.unlock();
   return res;
}

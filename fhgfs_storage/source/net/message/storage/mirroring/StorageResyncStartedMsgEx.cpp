#include <common/net/message/storage/mirroring/StorageResyncStartedRespMsg.h>
#include <common/toolkit/MessagingTk.h>

#include <program/Program.h>

#include "StorageResyncStartedMsgEx.h"

bool StorageResyncStartedMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   #ifdef BEEGFS_DEBUG
      const char* logContext = "StorageResyncStartedMsg incoming";
      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, Log_DEBUG,
         std::string("Received a StorageResyncStartedMsg from: ") + peer);
   #endif // BEEGFS_DEBUG

   uint16_t targetID = getValue();

   deleteMirrorSessions(targetID);

   StorageResyncStartedRespMsg respMsg;
   respMsg.serialize(respBuf, bufLen);
   sock->send(respBuf, respMsg.getMsgLength(), 0);

   return true;
}

void StorageResyncStartedMsgEx::deleteMirrorSessions(uint16_t targetID)
{
   SessionStore* sessions = Program::getApp()->getSessions();
   StringList sessionIDs;
   sessions->getAllSessionIDs(&sessionIDs);

   for (StringListIter iter = sessionIDs.begin(); iter != sessionIDs.end(); iter++)
   {
      std::string sessionID = *iter;
      Session* session = sessions->referenceSession(sessionID, false);

      if (!session) // meanwhile deleted
         continue;

      SessionLocalFileStore* sessionLocalFiles = session->getLocalFiles();
      sessionLocalFiles->removeAllMirrorSessions(targetID);
   }
}

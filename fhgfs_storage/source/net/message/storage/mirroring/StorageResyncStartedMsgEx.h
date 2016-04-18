#ifndef STORAGERESYNCSTARTEDMSGEX_H_
#define STORAGERESYNCSTARTEDMSGEX_H_

#include <common/net/message/storage/mirroring/StorageResyncStartedMsg.h>

class StorageResyncStartedMsgEx : public StorageResyncStartedMsg
{
   public:
      StorageResyncStartedMsgEx() : StorageResyncStartedMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
   
   private:
      void deleteMirrorSessions(uint16_t targetID);
};

#endif /*STORAGERESYNCSTARTEDMSGEX_H_*/

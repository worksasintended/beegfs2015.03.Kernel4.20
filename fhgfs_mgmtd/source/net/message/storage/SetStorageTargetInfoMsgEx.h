#ifndef SETSTORAGETARGETINFOMSGEX_H_
#define SETSTORAGETARGETINFOMSGEX_H_

#include <common/net/message/storage/SetStorageTargetInfoMsg.h>

class SetStorageTargetInfoMsgEx : public SetStorageTargetInfoMsg
{
   public:
      SetStorageTargetInfoMsgEx() : SetStorageTargetInfoMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);

   protected:

   private:
};

#endif /*SETSTORAGETARGETINFOMSGEX_H_*/

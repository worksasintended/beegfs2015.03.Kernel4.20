#ifndef GETSTORAGETARGETINFOMSGEX_H
#define GETSTORAGETARGETINFOMSGEX_H

#include <common/net/message/storage/GetStorageTargetInfoMsg.h>
#include <common/nodes/TargetStateStore.h>


class GetStorageTargetInfoMsgEx : public GetStorageTargetInfoMsg
{
   public:
      GetStorageTargetInfoMsgEx() : GetStorageTargetInfoMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);

   protected:


   public:
};


#endif /*GETSTORAGETARGETINFOMSGEX_H*/

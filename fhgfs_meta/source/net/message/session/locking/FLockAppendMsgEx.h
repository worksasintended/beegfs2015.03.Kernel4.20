#ifndef FLOCKAPPENDMSGEX_H_
#define FLOCKAPPENDMSGEX_H_

#include <common/net/message/session/locking/FLockAppendMsg.h>
#include <common/storage/StorageErrors.h>
#include <storage/FileInode.h>


class FLockAppendMsgEx : public FLockAppendMsg
{
   public:
      FLockAppendMsgEx() : FLockAppendMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);


   protected:

   private:

};

#endif /* FLOCKAPPENDMSGEX_H_ */

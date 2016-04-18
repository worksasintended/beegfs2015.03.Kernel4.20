#ifndef MOVECHUNKFILEMSGEX_H
#define MOVECHUNKFILEMSGEX_H

#include <common/net/message/NetMessage.h>
#include <common/net/message/fsck/MoveChunkFileMsg.h>
#include <common/net/message/fsck/MoveChunkFileRespMsg.h>

class MoveChunkFileMsgEx : public MoveChunkFileMsg
{
   public:
      MoveChunkFileMsgEx() : MoveChunkFileMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock, char* respBuf,
         size_t bufLen, HighResolutionStats* stats);

   private:
};

#endif /*MOVECHUNKFILEMSGEX_H*/

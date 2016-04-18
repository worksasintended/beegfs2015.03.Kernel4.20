#ifndef DELETECHUNKSMSGEX_H
#define DELETECHUNKSMSGEX_H

#include <common/net/message/NetMessage.h>
#include <common/net/message/fsck/DeleteChunksMsg.h>
#include <common/net/message/fsck/DeleteChunksRespMsg.h>

class DeleteChunksMsgEx : public DeleteChunksMsg
{
   public:
      DeleteChunksMsgEx() : DeleteChunksMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
               char* respBuf, size_t bufLen, HighResolutionStats* stats);

   private:
};

#endif /*DELETECHUNKSMSGEX_H*/

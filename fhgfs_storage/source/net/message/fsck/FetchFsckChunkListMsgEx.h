#ifndef FETCHFSCKCHUNKSMSGEX_H
#define FETCHFSCKCHUNKSMSGEX_H

#include <common/net/message/fsck/FetchFsckChunkListMsg.h>
#include <common/net/message/fsck/FetchFsckChunkListRespMsg.h>

class FetchFsckChunkListMsgEx : public FetchFsckChunkListMsg
{
   public:
      FetchFsckChunkListMsgEx() : FetchFsckChunkListMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
               char* respBuf, size_t bufLen, HighResolutionStats* stats);

   private:
};

#endif /*FETCHFSCKCHUNKSMSGEX_H*/

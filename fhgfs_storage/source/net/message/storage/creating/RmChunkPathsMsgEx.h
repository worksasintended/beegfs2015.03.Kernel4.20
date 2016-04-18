#ifndef RMCHUNKPATHSMSGEX_H_
#define RMCHUNKPATHSMSGEX_H_

#include <common/net/message/storage/creating/RmChunkPathsMsg.h>

class RmChunkPathsMsgEx : public RmChunkPathsMsg
{
   public:
      RmChunkPathsMsgEx() : RmChunkPathsMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
   
   protected:
   
   private:
   
};

#endif /*RMCHUNKPATHSMSGEX_H_*/

#ifndef REMOVEINODESMSGEX_H
#define REMOVEINODESMSGEX_H

#include <common/net/message/fsck/RemoveInodesMsg.h>

class RemoveInodesMsgEx : public RemoveInodesMsg
{
   public:
      RemoveInodesMsgEx() : RemoveInodesMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats) throw(SocketException);
};

#endif /*REMOVEINODESMSGEX_H*/

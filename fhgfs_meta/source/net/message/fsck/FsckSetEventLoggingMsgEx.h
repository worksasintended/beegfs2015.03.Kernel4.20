#ifndef FSCKSETEVENTLOGGINGMSGEX_H
#define FSCKSETEVENTLOGGINGMSGEX_H

#include <common/net/message/fsck/FsckSetEventLoggingMsg.h>
#include <common/net/message/fsck/FsckSetEventLoggingRespMsg.h>

class FsckSetEventLoggingMsgEx : public FsckSetEventLoggingMsg
{
   public:
      FsckSetEventLoggingMsgEx() : FsckSetEventLoggingMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
               char* respBuf, size_t bufLen, HighResolutionStats* stats);
};

#endif /*FSCKSETEVENTLOGGINGMSGEX_H*/

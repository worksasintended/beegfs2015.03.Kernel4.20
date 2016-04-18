#ifndef FSCKMODIFICATIONEVENTMSGEX_H
#define FSCKMODIFICATIONEVENTMSGEX_H

#include <common/net/message/fsck/FsckModificationEventMsg.h>

class FsckModificationEventMsgEx : public FsckModificationEventMsg
{
   public:
      FsckModificationEventMsgEx() : FsckModificationEventMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats) throw(SocketException);
};

#endif /*FSCKMODIFICATIONEVENTMSGEX_H*/

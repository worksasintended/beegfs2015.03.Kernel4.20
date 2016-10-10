#ifndef REMOVEBUDDYGROUPMSGEX_H
#define REMOVEBUDDYGROUPMSGEX_H

#include <common/net/message/nodes/RemoveBuddyGroupMsg.h>

class RemoveBuddyGroupMsgEx : public RemoveBuddyGroupMsg
{
   public:
      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
};

#endif

#ifndef UNMAPTARGETMSGEX_H_
#define UNMAPTARGETMSGEX_H_

#include <common/net/message/nodes/UnmapTargetMsg.h>

class UnmapTargetMsgEx : public UnmapTargetMsg
{
   public:
      UnmapTargetMsgEx() : UnmapTargetMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);


   protected:
};


#endif /* UNMAPTARGETMSGEX_H_ */

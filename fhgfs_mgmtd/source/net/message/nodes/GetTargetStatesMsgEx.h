#ifndef GETTARGETSTATESMSGEX_H_
#define GETTARGETSTATESMSGEX_H_

#include <common/net/message/nodes/GetTargetStatesMsg.h>

class GetTargetStatesMsgEx : public GetTargetStatesMsg
{
   public:
      GetTargetStatesMsgEx() : GetTargetStatesMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);


   protected:
};


#endif /* GETTARGETSTATESMSGEX_H_ */

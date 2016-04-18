#ifndef REFRESHTARGETSTATESMSGEX_H_
#define REFRESHTARGETSTATESMSGEX_H_

#include <common/net/message/nodes/RefreshTargetStatesMsg.h>


class RefreshTargetStatesMsgEx : public RefreshTargetStatesMsg
{
   public:
      RefreshTargetStatesMsgEx() : RefreshTargetStatesMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);


   protected:
};

#endif /* REFRESHTARGETSTATESMSGEX_H_ */

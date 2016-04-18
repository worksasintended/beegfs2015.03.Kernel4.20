#ifndef SETTARGETCONSISTENCYSTATESMSGEX_H
#define SETTARGETCONSISTENCYSTATESMSGEX_H

#include <common/net/message/nodes/SetTargetConsistencyStatesMsg.h>

class SetTargetConsistencyStatesMsgEx : public SetTargetConsistencyStatesMsg
{
   public:
      SetTargetConsistencyStatesMsgEx() : SetTargetConsistencyStatesMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);

   protected:


   private:
};

#endif /* SETTARGETCONSISTENCYSTATESMSGEX_H */

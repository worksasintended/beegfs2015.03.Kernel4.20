#ifndef CHANGETARGETCONSISTENCYSTATESMSGEX_H
#define CHANGETARGETCONSISTENCYSTATESMSGEX_H

#include <common/net/message/nodes/ChangeTargetConsistencyStatesMsg.h>

class ChangeTargetConsistencyStatesMsgEx : public ChangeTargetConsistencyStatesMsg
{
   public:
      ChangeTargetConsistencyStatesMsgEx() : ChangeTargetConsistencyStatesMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);

   protected:


   private:
};

#endif /* CHANGETARGETSTATESMSGEX_H */

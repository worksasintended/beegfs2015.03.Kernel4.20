#ifndef SETMIRRORBUDDYGROUPMSGEX_H_
#define SETMIRRORBUDDYGROUPMSGEX_H_

#include <common/net/message/nodes/SetMirrorBuddyGroupMsg.h>

class SetMirrorBuddyGroupMsgEx : public SetMirrorBuddyGroupMsg
{
   public:
      SetMirrorBuddyGroupMsgEx() : SetMirrorBuddyGroupMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);


   protected:
};


#endif /* SETMIRRORBUDDYGROUPMSGEX_H_ */

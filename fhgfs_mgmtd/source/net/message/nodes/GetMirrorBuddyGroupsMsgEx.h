#ifndef GETMIRRORBUDDYGROUPSMSGEX_H_
#define GETMIRRORBUDDYGROUPSMSGEX_H_

#include <common/net/message/nodes/GetMirrorBuddyGroupsMsg.h>

class GetMirrorBuddyGroupsMsgEx : public GetMirrorBuddyGroupsMsg
{
   public:
      GetMirrorBuddyGroupsMsgEx() : GetMirrorBuddyGroupsMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
};


#endif /* GETMIRRORBUDDYGROUPSMSGEX_H_ */

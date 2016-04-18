#ifndef GETSTATESANDBUDDYGROUPSMSGEX_H_
#define GETSTATESANDBUDDYGROUPSMSGEX_H_

#include <common/net/message/nodes/GetStatesAndBuddyGroupsMsg.h>

class GetStatesAndBuddyGroupsMsgEx : public GetStatesAndBuddyGroupsMsg
{
   public:
      GetStatesAndBuddyGroupsMsgEx() : GetStatesAndBuddyGroupsMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
};

#endif /*GETSTATESANDBUDDYGROUPSMSGEX_H_*/

#ifndef REMOVEBUDDYGROUPRESPMSG_H_
#define REMOVEBUDDYGROUPRESPMSG_H_

#include <common/net/message/SimpleIntMsg.h>

class RemoveBuddyGroupRespMsg : public SimpleIntMsg
{
   public:
      RemoveBuddyGroupRespMsg(FhgfsOpsErr result):
         SimpleIntMsg(NETMSGTYPE_RemoveBuddyGroupResp, result)
      {
      }

      RemoveBuddyGroupRespMsg() : SimpleIntMsg(NETMSGTYPE_RemoveBuddyGroupResp)
      {
      }

   public:
      FhgfsOpsErr getResult() const { return FhgfsOpsErr(getValue()); }
};

#endif

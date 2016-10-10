#ifndef REMOVEBUDDYGROUPMSG_H_
#define REMOVEBUDDYGROUPMSG_H_

#include <common/Common.h>
#include <common/net/message/AcknowledgeableMsg.h>
#include <common/nodes/Node.h>

class RemoveBuddyGroupMsg : public NetMessage
{
   public:
      RemoveBuddyGroupMsg(NodeType type, uint16_t groupID, bool checkOnly):
         NetMessage(NETMSGTYPE_RemoveBuddyGroup), type(type), groupID(groupID), checkOnly(checkOnly)
      {
      }

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      virtual unsigned calcMessageLength();

   protected:
      RemoveBuddyGroupMsg() : NetMessage(NETMSGTYPE_RemoveNode)
      {
      }

   protected:
      NodeType type;
      uint16_t groupID;
      bool checkOnly;
};

#endif

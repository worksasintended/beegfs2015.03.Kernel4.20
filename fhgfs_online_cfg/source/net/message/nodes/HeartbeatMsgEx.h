#ifndef HEARTBEATMSGEX_H_
#define HEARTBEATMSGEX_H_

#include <common/net/message/nodes/HeartbeatMsg.h>

class HeartbeatMsgEx : public HeartbeatMsg
{
   public:
      HeartbeatMsgEx() : HeartbeatMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);

     
   protected:
      
   private:
      void processIncomingRoot();
      void processNewNode(std::string nodeIDWithTypeStr, NodeType nodeType, NicAddressList* nicList,
         std::string sourcePeer);
};

#endif /*HEARTBEATMSGEX_H_*/

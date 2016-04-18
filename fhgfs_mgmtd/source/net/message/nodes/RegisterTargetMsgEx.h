#ifndef REGISTERTARGETMSGEX_H_
#define REGISTERTARGETMSGEX_H_


#include <common/net/message/nodes/RegisterTargetMsg.h>

class RegisterTargetMsgEx : public RegisterTargetMsg
{
   public:
      RegisterTargetMsgEx() : RegisterTargetMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);


   protected:
};

#endif /* REGISTERTARGETMSGEX_H_ */

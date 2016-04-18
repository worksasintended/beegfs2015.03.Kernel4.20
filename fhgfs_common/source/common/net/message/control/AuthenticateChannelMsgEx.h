#ifndef AUTHENTICATECHANNELMSGEX_H_
#define AUTHENTICATECHANNELMSGEX_H_

#include <common/net/message/control/AuthenticateChannelMsg.h>


class AuthenticateChannelMsgEx : public AuthenticateChannelMsg
{
   public:
      AuthenticateChannelMsgEx() : AuthenticateChannelMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);


   protected:

   private:

};


#endif /* AUTHENTICATECHANNELMSGEX_H_ */

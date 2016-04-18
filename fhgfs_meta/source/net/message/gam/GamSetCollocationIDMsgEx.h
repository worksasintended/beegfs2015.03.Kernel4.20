#ifndef GAMSETCOLLOCATIONIDMSGEX_H
#define GAMSETCOLLOCATIONIDMSGEX_H

#include <common/net/message/gam/GamSetCollocationIDMsg.h>
#include <common/net/message/gam/GamSetCollocationIDRespMsg.h>

class GamSetCollocationIDMsgEx : public GamSetCollocationIDMsg
{
   public:
      GamSetCollocationIDMsgEx() : GamSetCollocationIDMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);

     
   protected:
      
   private:

};

#endif /*GAMSETCOLLOCATIONIDMSGEX_H*/

#ifndef GAMGETCOLLOCATIONIDMETAMSGEX_H
#define GAMGETCOLLOCATIONIDMETAMSGEX_H

#include <common/net/message/gam/GamGetCollocationIDMetaMsg.h>
#include <common/net/message/gam/GamGetCollocationIDMetaRespMsg.h>

class GamGetCollocationIDMetaMsgEx : public GamGetCollocationIDMetaMsg
{
   public:
      GamGetCollocationIDMetaMsgEx() : GamGetCollocationIDMetaMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);

     
   protected:
      
   private:

};

#endif /*GAMGETCOLLOCATIONIDMETAMSGEX_H*/

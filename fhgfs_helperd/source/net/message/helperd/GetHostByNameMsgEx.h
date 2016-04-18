#ifndef GETHOSTBYNAMEMSGEX_H_
#define GETHOSTBYNAMEMSGEX_H_

#include <common/net/message/helperd/GetHostByNameMsg.h>

class GetHostByNameMsgEx : public GetHostByNameMsg
{
   public:
      GetHostByNameMsgEx() : GetHostByNameMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
   
   protected:
   
   private:

};

#endif /*GETHOSTBYNAMEMSGEX_H_*/

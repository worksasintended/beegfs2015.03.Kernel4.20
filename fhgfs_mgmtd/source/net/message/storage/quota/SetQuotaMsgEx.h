#ifndef SETQUOTAMSGEX_H_
#define SETQUOTAMSGEX_H_


#include <common/net/message/storage/quota/SetQuotaMsg.h>


class SetQuotaMsgEx : public SetQuotaMsg
{
   public:
      SetQuotaMsgEx() : SetQuotaMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);

   private:
      bool processQuotaLimits();
};

#endif /* SETQUOTAMSGEX_H_ */

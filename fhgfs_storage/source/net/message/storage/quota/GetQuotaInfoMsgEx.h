#ifndef GETQUOTAINFOMSGEX_H_
#define GETQUOTAINFOMSGEX_H_


#include <common/net/message/storage/quota/GetQuotaInfoMsg.h>
#include <common/Common.h>



class GetQuotaInfoMsgEx : public GetQuotaInfoMsg
{
   public:
      GetQuotaInfoMsgEx() : GetQuotaInfoMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);

   protected:

   private:

};

#endif /* GETQUOTAINFOMSGEX_H_ */

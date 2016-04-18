#ifndef SETEXCEEDEDQUOTAMSGEX_H_
#define SETEXCEEDEDQUOTAMSGEX_H_


#include <common/net/message/storage/quota/SetExceededQuotaMsg.h>
#include <common/Common.h>


class SetExceededQuotaMsgEx : public SetExceededQuotaMsg
{
   public:
      SetExceededQuotaMsgEx() : SetExceededQuotaMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock, char* respBuf,
         size_t bufLen, HighResolutionStats* stats);
};

#endif /* SETEXCEEDEDQUOTAMSGEX_H_ */

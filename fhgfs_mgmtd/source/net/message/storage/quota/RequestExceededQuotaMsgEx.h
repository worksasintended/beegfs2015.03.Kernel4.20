#ifndef REQUESTEXCEEDEDQUOTAMSGEX_H_
#define REQUESTEXCEEDEDQUOTAMSGEX_H_


#include <common/net/message/storage/quota/RequestExceededQuotaMsg.h>
#include <common/Common.h>


class RequestExceededQuotaMsgEx : public RequestExceededQuotaMsg
{
   public:
      RequestExceededQuotaMsgEx() : RequestExceededQuotaMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock, char* respBuf,
         size_t bufLen, HighResolutionStats* stats);
};

#endif /* REQUESTEXCEEDEDQUOTAMSGEX_H_ */

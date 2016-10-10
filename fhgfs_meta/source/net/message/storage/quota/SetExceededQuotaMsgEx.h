#ifndef META_SETEXCEEDEDQUOTAMSGEX_H_
#define META_SETEXCEEDEDQUOTAMSGEX_H_


#include <common/net/message/storage/quota/SetExceededQuotaMsg.h>
#include <common/Common.h>


class SetExceededQuotaMsgEx : public SetExceededQuotaMsg
{
   public:
      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
};

#endif /* META_SETEXCEEDEDQUOTAMSGEX_H_ */

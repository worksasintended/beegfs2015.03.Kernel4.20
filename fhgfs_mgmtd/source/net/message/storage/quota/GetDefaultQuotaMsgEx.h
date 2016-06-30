#ifndef SOURCE_NET_MESSAGE_STORAGE_QUOTA_GETDEFAULTQUOTAMSGEX_H_
#define SOURCE_NET_MESSAGE_STORAGE_QUOTA_GETDEFAULTQUOTAMSGEX_H_



#include <common/net/message/storage/quota/GetDefaultQuotaMsg.h>


class GetDefaultQuotaMsgEx : public GetDefaultQuotaMsg
{
   public:
      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
};

#endif /* SOURCE_NET_MESSAGE_STORAGE_QUOTA_GETDEFAULTQUOTAMSGEX_H_ */

#ifndef GETSTORAGERESYNCSTATSMSGEX_H_
#define GETSTORAGERESYNCSTATSMSGEX_H_

#include <common/net/message/storage/mirroring/GetStorageResyncStatsMsg.h>
#include <common/storage/StorageErrors.h>

class GetStorageResyncStatsMsgEx : public GetStorageResyncStatsMsg
{
   public:
      GetStorageResyncStatsMsgEx() : GetStorageResyncStatsMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
};

#endif /*GETSTORAGERESYNCSTATSMSGEX_H_*/

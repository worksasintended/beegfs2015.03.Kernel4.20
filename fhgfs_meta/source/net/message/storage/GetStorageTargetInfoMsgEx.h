#ifndef GETSTORAGETARGETINFOMSGEX_H
#define GETSTORAGETARGETINFOMSGEX_H

#include <common/net/message/storage/GetStorageTargetInfoMsg.h>
#include <common/nodes/TargetStateStore.h>

class GetStorageTargetInfoMsgEx : public GetStorageTargetInfoMsg
{
   public:
      GetStorageTargetInfoMsgEx() : GetStorageTargetInfoMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
   
   protected:
   

   private:
      void getStatInfo(int64_t* outSizeTotal, int64_t* outSizeFree, int64_t* outInodesTotal,
         int64_t* outInodesFree);
};


#endif /*GETSTORAGETARGETINFOMSGEX_H*/

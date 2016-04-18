#ifndef UNLINKLOCALFILEMSGEX_H_
#define UNLINKLOCALFILEMSGEX_H_

#include <common/net/message/storage/creating/UnlinkLocalFileMsg.h>

class UnlinkLocalFileMsgEx : public UnlinkLocalFileMsg
{
   public:
      UnlinkLocalFileMsgEx() : UnlinkLocalFileMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
   
   protected:
   
   private:
      int getTargetFD(struct sockaddr_in* fromAddr, Socket* sock, char* respBuf, size_t bufLen,
         uint16_t actualTargetID, bool* outResponseSent);
      FhgfsOpsErr forwardToSecondary(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, uint16_t actualTargetID, bool* outChunkLocked);
   
};

#endif /*UNLINKLOCALFILEMSGEX_H_*/

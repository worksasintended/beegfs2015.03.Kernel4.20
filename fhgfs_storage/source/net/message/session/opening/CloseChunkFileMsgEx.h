#ifndef CLOSECHUNKFILEMSGEX_H_
#define CLOSECHUNKFILEMSGEX_H_

#include <common/net/message/session/opening/CloseChunkFileMsg.h>

class CloseChunkFileMsgEx : public CloseChunkFileMsg
{
   private:
      struct DynamicAttribs
      {
         int64_t filesize;
         int64_t allocedBlocks; // allocated 512byte blocks (relevant for sparse files)
         int64_t modificationTimeSecs;
         int64_t lastAccessTimeSecs;
         uint64_t storageVersion;
      };
   
   public:
      CloseChunkFileMsgEx() : CloseChunkFileMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);

     
   protected:
   
   private:
      FhgfsOpsErr forwardToSecondary(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen);
      bool getDynamicAttribsByFD(int fd, std::string fileHandleID, uint16_t targetID,
         DynamicAttribs& outDynAttribs);
      bool getDynamicAttribsByPath(std::string fileHandleID, uint16_t targetID,
         DynamicAttribs& outDynAttribs);
};

#endif /*CLOSECHUNKFILEMSGEX_H_*/

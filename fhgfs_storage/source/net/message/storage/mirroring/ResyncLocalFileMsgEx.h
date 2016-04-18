#ifndef RESYNCLOCALFILEMSGEX_H_
#define RESYNCLOCALFILEMSGEX_H_

#include <common/net/message/storage/mirroring/ResyncLocalFileMsg.h>
#include <common/storage/StorageErrors.h>

class ResyncLocalFileMsgEx : public ResyncLocalFileMsg
{
   public:
      ResyncLocalFileMsgEx() : ResyncLocalFileMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
   
   protected:
   
   private:
      bool doWrite(int fd, const char* buf, size_t count, off_t offset, int& outErrno);
      bool doWriteSparse(int fd, const char* buf, size_t count, off_t offset, int& outErrno);
      bool doTrunc(int fd, off_t offset, int& outErrno);
      FhgfsOpsErr fhgfsErrFromSysErr(int64_t errCode);

};

#endif /*RESYNCLOCALFILEMSGEX_H_*/

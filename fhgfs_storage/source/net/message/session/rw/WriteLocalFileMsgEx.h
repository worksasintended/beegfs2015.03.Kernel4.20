#ifndef WRITELOCALFILEMSGEX_H_
#define WRITELOCALFILEMSGEX_H_

#include <common/net/message/session/rw/WriteLocalFileMsg.h>
#include <session/SessionLocalFile.h>
#include <common/storage/StorageErrors.h>


#define WRITEMSG_MIRROR_RETRIES_NUM    1


class WriteLocalFileMsgEx : public WriteLocalFileMsg
{
   public:
      WriteLocalFileMsgEx() : WriteLocalFileMsg()
      {
         mirrorToSock = NULL;
         mirrorRetriesLeft = WRITEMSG_MIRROR_RETRIES_NUM;
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
   
   protected:
   
   private:
      Socket* mirrorToSock;
      unsigned mirrorRetriesLeft;

      int64_t incrementalRecvAndWriteStateful(Socket* sock, char* buf, ssize_t bufLen,
         SessionLocalFile* sessionLocalFile);
      void incrementalRecvPadding(Socket* sock, char* buf, size_t bufLen, int64_t padLen,
         SessionLocalFile* sessionLocalFile);

      ssize_t doWrite(int fd, char* buf, size_t count, off_t offset, int& outErrno);

      FhgfsOpsErr openFile(SessionLocalFile* sessionLocalFile);

      FhgfsOpsErr prepareMirroring(char* respBuf, size_t bufLen,
         SessionLocalFile* sessionLocalFile);
      FhgfsOpsErr sendToMirror(const char* buf, size_t bufLen, int64_t offset, int64_t toBeMirrored,
         SessionLocalFile* sessionLocalFile);
      FhgfsOpsErr finishMirroring(char* buf, size_t bufLen,
         SessionLocalFile* sessionLocalFile);

      FhgfsOpsErr fhgfsErrFromSysErr(int64_t errCode);

      bool doSessionCheck();
};

#endif /*WRITELOCALFILEMSGEX_H_*/

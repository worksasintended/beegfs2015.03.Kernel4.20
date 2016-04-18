#ifndef TRUNCLOCALFILEMSGEX_H_
#define TRUNCLOCALFILEMSGEX_H_

#include <common/net/message/storage/TruncLocalFileMsg.h>
#include <common/storage/StorageErrors.h>
#include <common/storage/PathVec.h>

class TruncLocalFileMsgEx : public TruncLocalFileMsg
{
   private:
      struct DynamicAttribs
      {
         DynamicAttribs() : filesize(0), allocedBlocks(0), modificationTimeSecs(0),
            lastAccessTimeSecs(0), storageVersion(0) {}

         int64_t filesize;
         int64_t allocedBlocks; // allocated 512byte blocks (relevant for sparse files)
         int64_t modificationTimeSecs;
         int64_t lastAccessTimeSecs;
         uint64_t storageVersion;
      };

   public:
      TruncLocalFileMsgEx() : TruncLocalFileMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
   
   protected:
   
   private:
      FhgfsOpsErr truncFile(int targetFD, PathVec* chunkDirPath, std::string& chunkFilePathStr,
         std::string entryID, bool hasOrigFeature);
      int getTargetFD(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, uint16_t actualTargetID, bool* outResponseSent);
      bool getDynamicAttribsByPath(const int dirFD, const char* path, uint16_t targetID,
         std::string fileID, DynamicAttribs& outDynAttribs);
      bool getFakeDynAttribs(uint16_t targetID, std::string fileID, DynamicAttribs& outDynAttribs);
      FhgfsOpsErr forwardToSecondary(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, uint16_t actualTargetID, bool* outChunkLocked);
      FhgfsOpsErr fhgfsErrFromSysErr(int errCode);
};

#endif /*TRUNCLOCALFILEMSGEX_H_*/

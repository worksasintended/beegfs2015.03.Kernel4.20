#ifndef CLOSECHUNKFILEWORK_H_
#define CLOSECHUNKFILEWORK_H_

#include <common/components/worker/Work.h>
#include <common/net/sock/Socket.h>
#include <common/storage/striping/StripePattern.h>
#include <common/storage/StorageErrors.h>
#include <common/toolkit/SynchronizedCounter.h>
#include <common/storage/striping/ChunkFileInfo.h>
#include <common/Common.h>


class CloseChunkFileWork : public Work
{
   public:
      /**
       * @param outDynAttribs may be NULL if caller is not interested
       */
      CloseChunkFileWork(std::string& sessionID, std::string& fileHandleID, StripePattern* pattern,
         uint16_t targetID, PathInfo* pathInfo, DynamicFileAttribs *outDynAttribs,
         FhgfsOpsErr* outResult, SynchronizedCounter* counter):
         sessionID(sessionID), fileHandleID(fileHandleID), pattern(pattern), targetID(targetID),
         pathInfoPtr(pathInfo), outDynAttribs(outDynAttribs), outResult(outResult),
         counter(counter), entryInfo(NULL), useBuddyMirrorSecond(false),
         msgUserID(NETMSG_DEFAULT_USERID)
      {
         // all assignments done in initializer list
      }

      virtual ~CloseChunkFileWork() {}


      virtual void process(char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen);


   private:
      std::string sessionID;
      std::string fileHandleID;

      StripePattern* pattern;
      uint16_t targetID;
      PathInfo* pathInfoPtr; // to find chunk files

      DynamicFileAttribs* outDynAttribs;
      FhgfsOpsErr* outResult;
      SynchronizedCounter* counter;

      EntryInfo* entryInfo; // needed for backlinks

      // for HSM integration
      HsmCollocationID hsmCollocationID;

      bool useBuddyMirrorSecond;

      unsigned msgUserID;


      FhgfsOpsErr communicate();


   public:
      // getters & setters

      void setEntryInfo(EntryInfo* entryInfo)
      {
         this->entryInfo = entryInfo;
      }

      void setHsmCollocationID(HsmCollocationID hsmCollocationID)
      {
         this->hsmCollocationID = hsmCollocationID;
      }

      void setMsgUserID(unsigned msgUserID)
      {
         this->msgUserID = msgUserID;
      }

      void setUseBuddyMirrorSecond()
      {
         this->useBuddyMirrorSecond = true;
      }

};

#endif /* CLOSECHUNKFILEWORK_H_ */

#ifndef READLOCALFILEV2MSGEX_H_
#define READLOCALFILEV2MSGEX_H_

#include <common/net/message/session/rw/ReadLocalFileV2Msg.h>
#include <common/storage/StorageErrors.h>
#include <session/SessionLocalFileStore.h>

class ReadLocalFileV2MsgEx : public ReadLocalFileV2Msg
{
   public:
      ReadLocalFileV2MsgEx() : ReadLocalFileV2Msg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
         
     
   protected:
      
   private:
      SessionLocalFileStore* sessionLocalFiles;
      SessionLocalFile* sessionLocalFile;
      bool needFileUnlockAndRelease;


      int64_t incrementalReadStatefulAndSendV2(Socket* sock,
         char* buf, size_t bufLen, SessionLocalFile* sessionLocalFile, HighResolutionStats* stats);

      void checkAndStartReadAhead(SessionLocalFile* sessionLocalFile, ssize_t readAheadTriggerSize,
         off_t currentOffset, off_t readAheadSize);

      FhgfsOpsErr openFile(SessionLocalFile* sessionLocalFile);


      // inliners
      
      /**
       * Send only length information without a data packet. Typically used for the final length
       * info at the end of the requested data.
       */
      inline void sendLengthInfo(Socket* sock, int64_t lengthInfo)
      {
         char lengthInfoBuf[sizeof(int64_t)];
         Serialization::serializeInt64(lengthInfoBuf, lengthInfo);

         sock->send(&lengthInfoBuf, sizeof(int64_t), 0);   
      }

      /**
       * Send length information and the corresponding data packet buffer.
       *
       * Note: lengthInfo is used to compute buf length for send()
       * 
       * @param lengthInfo must not be negative
       * @param buf the buffer with a preceding gap for the length info
       * @param isFinal true if this is the last send, i.e. we have read all data
       */
      inline void sendLengthInfoAndData(Socket* sock, int64_t lengthInfo, char* buf, bool isFinal)
      {
         if (isFinal)
         {
            Serialization::serializeInt64(buf, lengthInfo);
            Serialization::serializeInt64(&buf[sizeof(int64_t) + lengthInfo], 0);

            sock->send(buf, (2*sizeof(int64_t) ) + lengthInfo, 0);
         }
         else
         {
            Serialization::serializeInt64(buf, lengthInfo);

            sock->send(buf, sizeof(int64_t) + lengthInfo, 0);
         }
      }

      /*
       * Unlock and release the file session.
       */
      inline void releaseFile(void)
      {
         sessionLocalFiles->releaseSession(sessionLocalFile);

         sessionLocalFile = NULL;
         this->needFileUnlockAndRelease = false;
      }
};


#endif /*READLOCALFILEV2MSGEX_H_*/

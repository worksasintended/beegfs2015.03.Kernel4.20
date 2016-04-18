#ifndef READLOCALFILEV2MSG_H_
#define READLOCALFILEV2MSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/PathInfo.h>


#define READLOCALFILEMSG_FLAG_SESSION_CHECK       1 /* if session check infos should be done */
#define READLOCALFILEMSG_FLAG_DISABLE_IO          2 /* disable read syscall for net bench */
#define READLOCALFILEMSG_FLAG_BUDDYMIRROR         4 /* given targetID is a buddymirrorgroup ID */
#define READLOCALFILEMSG_FLAG_BUDDYMIRROR_SECOND  8 /* secondary of group, otherwise primary */


class ReadLocalFileV2Msg : public NetMessage
{
   public:

      /**
       * @param sessionID just a reference, so do not free it as long as you use this object!
       * @param fileHandleID just a reference, so do not free it as long as you use this object!
       */
      ReadLocalFileV2Msg(const char* sessionID, const char* fileHandleID, uint16_t targetID,
         PathInfo* pathInfoPtr, unsigned accessFlags, int64_t offset, int64_t count) :
         NetMessage(NETMSGTYPE_ReadLocalFileV2)
      {
         this->sessionID = sessionID;
         this->sessionIDLen = strlen(sessionID);
         
         this->fileHandleID = fileHandleID;
         this->fileHandleIDLen = strlen(fileHandleID);

         this->targetID = targetID;

         this->pathInfoPtr = pathInfoPtr;

         this->accessFlags = accessFlags;

         this->offset = offset;
         this->count = count;
      }


     
   protected:
      /**
       * For deserialization only!
       */
      ReadLocalFileV2Msg() : NetMessage(NETMSGTYPE_ReadLocalFileV2) {}


      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenInt64()                    + // offset
            Serialization::serialLenInt64()                    + // count
            Serialization::serialLenUInt()                     + // accessFlags
            Serialization::serialLenStrAlign4(fileHandleIDLen) +
            Serialization::serialLenStrAlign4(sessionIDLen)    +
            this->pathInfoPtr->serialLen()                     + // pathInfo
            Serialization::serialLenUShort();                    // targetID
      }

      unsigned getSupportedHeaderFeatureFlagsMask() const
      {
         return READLOCALFILEMSG_FLAG_SESSION_CHECK | READLOCALFILEMSG_FLAG_DISABLE_IO |
            READLOCALFILEMSG_FLAG_BUDDYMIRROR | READLOCALFILEMSG_FLAG_BUDDYMIRROR_SECOND;
      }


   private:
      int64_t offset;
      int64_t count;
      unsigned accessFlags;
      const char* fileHandleID;
      unsigned fileHandleIDLen;
      const char* sessionID;
      unsigned sessionIDLen;
      uint16_t targetID;

      // for serialization
      PathInfo* pathInfoPtr;

      // for deserialization
      PathInfo pathInfo;

   public:
      // getters & setters
      const char* getSessionID() const
      {
         return sessionID;
      }
      
      const char* getFileHandleID() const
      {
         return fileHandleID;
      }

      uint16_t getTargetID() const
      {
         return targetID;
      }

      unsigned getAccessFlags() const
      {
         return accessFlags;
      }
      
      int64_t getOffset() const
      {
         return offset;
      }
      
      int64_t getCount() const
      {
         return count;
      }
      
      PathInfo* getPathInfo()
      {
         return &this->pathInfo;
      }
};


#endif /*READLOCALFILEV2MSG_H_*/

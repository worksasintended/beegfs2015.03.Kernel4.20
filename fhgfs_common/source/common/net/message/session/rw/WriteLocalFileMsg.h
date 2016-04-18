#ifndef WRITELOCALFILEMSG_H_
#define WRITELOCALFILEMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/PathInfo.h>


#define WRITELOCALFILEMSG_FLAG_SESSION_CHECK        1 /* if session check should be done */
#define WRITELOCALFILEMSG_FLAG_USE_QUOTA            2 /* if msg contains quota info */
#define WRITELOCALFILEMSG_FLAG_DISABLE_IO           4 /* disable write syscall for net bench mode */
#define WRITELOCALFILEMSG_FLAG_BUDDYMIRROR          8 /* given targetID is a buddymirrorgroup ID */
#define WRITELOCALFILEMSG_FLAG_BUDDYMIRROR_SECOND  16 /* secondary of group, otherwise primary */
#define WRITELOCALFILEMSG_FLAG_BUDDYMIRROR_FORWARD 32 /* forward msg to secondary */


class WriteLocalFileMsg : public NetMessage
{
   public:
      
      /**
       * @param sessionID just a reference, so do not free it as long as you use this object!
       * @param fileHandleID just a reference, so do not free it as long as you use this object!
       * @param pathInfo just a reference, so do not free it as long as you use this object!
       */
      WriteLocalFileMsg(const char* sessionID, const char* fileHandleID, uint16_t targetID,
         PathInfo* pathInfo, unsigned accessFlags, int64_t offset, int64_t count) :
         NetMessage(NETMSGTYPE_WriteLocalFile)
      {
         this->sessionID = sessionID;
         this->sessionIDLen = strlen(sessionID);
         
         this->fileHandleID = fileHandleID;
         this->fileHandleIDLen = strlen(fileHandleID);

         this->targetID = targetID;

         this->pathInfoPtr = pathInfo;

         this->accessFlags = accessFlags;

         this->offset = offset;
         this->count = count;
      }


     
   protected:
      /**
       * For deserialization only!
       */
      WriteLocalFileMsg() : NetMessage(NETMSGTYPE_WriteLocalFile) {}


      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         unsigned retVal = 0;

         retVal += NETMSG_HEADER_LENGTH;
         retVal += Serialization::serialLenInt64();                    // offset
         retVal += Serialization::serialLenInt64();                    // count
         retVal += Serialization::serialLenUInt();                     // accessFlags

         if(isMsgHeaderFeatureFlagSet(WRITELOCALFILEMSG_FLAG_USE_QUOTA) )
         {
            retVal += Serialization::serialLenUInt(); // userID
            retVal += Serialization::serialLenUInt(); // groupID
         }

         retVal += Serialization::serialLenStrAlign4(fileHandleIDLen);
         retVal += Serialization::serialLenStrAlign4(sessionIDLen);
         retVal += this->pathInfoPtr->serialLen();                      // pathInfo
         retVal += Serialization::serialLenUShort();                    // targetID

         return retVal;
      }

      unsigned getSupportedHeaderFeatureFlagsMask() const
      {
         return WRITELOCALFILEMSG_FLAG_SESSION_CHECK | WRITELOCALFILEMSG_FLAG_USE_QUOTA |
            WRITELOCALFILEMSG_FLAG_DISABLE_IO | WRITELOCALFILEMSG_FLAG_BUDDYMIRROR |
            WRITELOCALFILEMSG_FLAG_BUDDYMIRROR_SECOND | WRITELOCALFILEMSG_FLAG_BUDDYMIRROR_FORWARD;
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

      unsigned userID;
      unsigned groupID;

      // for serialization
      PathInfo* pathInfoPtr;

      // for deserilization
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
      
      PathInfo* getPathInfo ()
      {
         return &this->pathInfo;
      }

      unsigned getUserID() const
      {
         return userID;
      }
 
      unsigned getGroupID() const
      {
         return groupID;
      }

      void setUserdataForQuota(unsigned userID, unsigned groupID)
      {
         addMsgHeaderFeatureFlag(WRITELOCALFILEMSG_FLAG_USE_QUOTA);

         this->userID = userID;
         this->groupID = groupID;
      }

      // inliners

      void sendData(const char* data, Socket* sock)
      {
         sock->send(data, count, 0);
      }


};

#endif /*WRITELOCALFILEMSG_H_*/

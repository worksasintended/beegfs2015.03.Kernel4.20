#ifndef FSYNCLOCALFILEMSG_H_
#define FSYNCLOCALFILEMSG_H_

#include <common/net/message/NetMessage.h>


#define FSYNCLOCALFILEMSG_FLAG_NO_SYNC          1 /* if a sync is not needed */
#define FSYNCLOCALFILEMSG_FLAG_SESSION_CHECK    2 /* if session check should be done */
#define FSYNCLOCALFILEMSG_FLAG_BUDDYMIRROR      4 /* given targetID is a buddymirrorgroup ID */
#define FSYNCLOCALFILEMSG_FLAG_BUDDYMIRROR_SECOND 8 /* secondary of group, otherwise primary */


class FSyncLocalFileMsg : public NetMessage
{
   public:
      
      /**
       * @param sessionID just a reference, so do not free it as long as you use this object!
       * @param fileHandleID just a reference, so do not free it as long as you use this object!
       */
      FSyncLocalFileMsg(const char* sessionID, const char* fileHandleID, uint16_t targetID)
         : NetMessage(NETMSGTYPE_FSyncLocalFile)
      {
         this->sessionID = sessionID;
         this->sessionIDLen = strlen(sessionID);
         
         this->fileHandleID = fileHandleID;
         this->fileHandleIDLen = strlen(fileHandleID);

         this->targetID = targetID;
      }


   protected:
      /**
       * For deserialization only!
       */
      FSyncLocalFileMsg() : NetMessage(NETMSGTYPE_FSyncLocalFile){}


      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         unsigned retVal = NETMSG_HEADER_LENGTH +
            Serialization::serialLenStrAlign4(sessionIDLen) +
            Serialization::serialLenStrAlign4(fileHandleIDLen) +
            Serialization::serialLenUShort();    // targetID

         return retVal;
      }

      unsigned getSupportedHeaderFeatureFlagsMask() const
      {
         return FSYNCLOCALFILEMSG_FLAG_BUDDYMIRROR | FSYNCLOCALFILEMSG_FLAG_NO_SYNC |
            FSYNCLOCALFILEMSG_FLAG_SESSION_CHECK | FSYNCLOCALFILEMSG_FLAG_BUDDYMIRROR_SECOND;
      }


   private:
      const char* sessionID;
      unsigned sessionIDLen;
      const char* fileHandleID;
      unsigned fileHandleIDLen;
      uint16_t targetID;


   public:
      // getters & setters
      const char* getSessionID()
      {
         return sessionID;
      }
      
      const char* getFileHandleID()
      {
         return fileHandleID;
      }
      
      uint16_t getTargetID()
      {
         return targetID;
      }
};


#endif /*FSYNCLOCALFILEMSG_H_*/

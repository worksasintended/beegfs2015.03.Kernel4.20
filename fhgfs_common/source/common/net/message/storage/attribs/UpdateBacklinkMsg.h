#ifndef UPDATEBACKLINKMSG_H
#define UPDATEBACKLINKMSG_H

#include <common/net/message/NetMessage.h>
#include <common/storage/PathInfo.h>


#define UPDATEBACKLINKSMSG_FLAG_BUDDYMIRROR        1 /* given targetID is a buddymirrorgroup ID */
#define UPDATEBACKLINKSMSG_FLAG_BUDDYMIRROR_SECOND 2 /* secondary of group, otherwise primary */


class UpdateBacklinkMsg : public NetMessage
{
   public:

      /**
       * @param entryID just a reference, so do not free it as long as you use this object!
       * @param targetID just a reference, so do not free it as long as you use this object!
       * @param pathInfo just a reference, so do not free it as long as you use this object!
       * @param entryInfoBuf the serialized entryInfo
       */
      UpdateBacklinkMsg(std::string& entryID, uint16_t targetID, PathInfo* pathInfo,
         const char* entryInfoBuf,
         unsigned entryInfoBufLen) : NetMessage(NETMSGTYPE_UpdateBacklink)
      {
         this->entryID = entryID.c_str();
         this->entryIDLen = entryID.length();
         this->targetID = targetID;
         this->pathInfoPtr = pathInfo;
         this->entryInfoBufLen = entryInfoBufLen;
         this->entryInfoBuf = entryInfoBuf;
      }

   protected:
      UpdateBacklinkMsg() : NetMessage(NETMSGTYPE_UpdateBacklink) {}

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenStrAlign4(entryIDLen)   + // entryID
            this->pathInfoPtr->serialLen()                  + // pathInfo
            Serialization::serialLenUShort()                + // targetID
            Serialization::serialLenUInt()                  + // entryInfoBufLen
            entryInfoBufLen;                                // entryInfoBuf
      }

      unsigned getSupportedHeaderFeatureFlagsMask() const
      {
         return UPDATEBACKLINKSMSG_FLAG_BUDDYMIRROR | UPDATEBACKLINKSMSG_FLAG_BUDDYMIRROR_SECOND;
      }

   private:
      unsigned entryIDLen;
      const char* entryID;
      uint16_t targetID;
      unsigned entryInfoBufLen;
      const char* entryInfoBuf;

      // for serialization
      PathInfo* pathInfoPtr;

      // for deserialization
      PathInfo pathInfo;

   public:
   
      // getters & setters
      std::string getEntryID()
      {
         return this->entryID;
      }
      
      uint16_t getTargetID()
      {
         return this->targetID;
      }

      const char* getEntryInfoBuf()
      {
         return this->entryInfoBuf;
      }

      unsigned getEntryInfoBufLen()
      {
         return this->entryInfoBufLen;
      }

      PathInfo* getPathInfo()
      {
         return &this->pathInfo;
      }

};

#endif /*UPDATEBACKLINKMSG_H*/

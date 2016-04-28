#ifndef SETLOCALATTRMSG_H_
#define SETLOCALATTRMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/PathInfo.h>
#include <common/storage/StorageDefinitions.h>


#define SETLOCALATTRMSG_FLAG_USE_QUOTA             1 // if the message contains quota informations
#define SETLOCALATTRMSG_FLAG_BUDDYMIRROR           2 // given targetID is a buddymirrorgroup ID
#define SETLOCALATTRMSG_FLAG_BUDDYMIRROR_SECOND    4 // secondary of group, otherwise primary

#define SETLOCALATTRMSG_COMPAT_FLAG_EXTEND_REPLY   1 // send the attribs and a storage version in
                                                     // reply

class SetLocalAttrMsg : public NetMessage
{
   friend class AbstractNetMessageFactory;
   
   public:

      /**
       * @param entryID just a reference, so do not free it as long as you use this object!
       * @param pathInfo just a reference, so do not free it as long as you use this object!
       * @param validAttribs a combination of SETATTR_CHANGE_...-Flags
       */
      SetLocalAttrMsg(std::string& entryID, uint16_t targetID, PathInfo* pathInfo, int validAttribs,
         SettableFileAttribs* attribs, bool enableCreation) : NetMessage(NETMSGTYPE_SetLocalAttr)
      {
         this->entryID = entryID.c_str();
         this->entryIDLen = entryID.length();
         this->targetID = targetID;
         this->pathInfoPtr = pathInfo;

         this->validAttribs = validAttribs;
         this->attribs = *attribs;
         this->enableCreation = enableCreation;
      }

   protected:
      /**
       * For deserialization only!
       */
      SetLocalAttrMsg() : NetMessage(NETMSGTYPE_SetLocalAttr) {}


      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenInt64()                 + // modificationTimeSecs
            Serialization::serialLenInt64()                 + // lastAccessTimeSecs
            Serialization::serialLenInt()                   + // validAttribs
            Serialization::serialLenInt()                   + // mode
            Serialization::serialLenUInt()                  + // userID
            Serialization::serialLenUInt()                  + // groupID
            Serialization::serialLenStrAlign4(entryIDLen)   +
            this->pathInfoPtr->serialLen()                  +
            Serialization::serialLenUShort()                + // targetID
            Serialization::serialLenBool();                   // enableCreation
      }

      unsigned getSupportedHeaderFeatureFlagsMask() const
      {
         return SETLOCALATTRMSG_FLAG_USE_QUOTA | SETLOCALATTRMSG_FLAG_BUDDYMIRROR |
            SETLOCALATTRMSG_FLAG_BUDDYMIRROR_SECOND;
      }



   private:
      unsigned entryIDLen;
      const char* entryID;
      uint16_t targetID;
      int validAttribs;
      SettableFileAttribs attribs;
      bool enableCreation;

      // for serialization
      PathInfo* pathInfoPtr;

      // for deserialization
      PathInfo pathInfo;


   public:
   
      // getters & setters
      const char* getEntryID() const
      {
         return entryID;
      }
      
      uint16_t getTargetID() const
      {
         return targetID;
      }

      int getValidAttribs() const
      {
         return validAttribs;
      }
      
      const SettableFileAttribs* getAttribs() const
      {
         return &attribs;
      }
      
      bool getEnableCreation() const
      {
         return enableCreation;
      }

      const PathInfo* getPathInfo() const
      {
         return &this->pathInfo;
      }

};

#endif /*SETLOCALATTRMSG_H_*/

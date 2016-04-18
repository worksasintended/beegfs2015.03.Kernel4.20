#ifndef TRUNCLOCALFILEMSG_H_
#define TRUNCLOCALFILEMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/PathInfo.h>
#include <common/storage/EntryInfo.h>


#define TRUNCLOCALFILEMSG_FLAG_NODYNAMICATTRIBS   1 /* skip retrieval of current dyn attribs */
#define TRUNCLOCALFILEMSG_FLAG_USE_QUOTA          2 /* if the message contains quota informations */
#define TRUNCLOCALFILEMSG_FLAG_BUDDYMIRROR        4 /* given targetID is a buddymirrorgroup ID */
#define TRUNCLOCALFILEMSG_FLAG_BUDDYMIRROR_SECOND 8 /* secondary of group, otherwise primary */

class TruncLocalFileMsg : public NetMessage
{
   public:

      /**
       * @param entryID just a reference, so do not free it as long as you use this object!
       * @param pathInfo just a reference, so do not free it as long as you use this object!
       */
      TruncLocalFileMsg(int64_t filesize, std::string& entryID, uint16_t targetID,
         PathInfo* pathInfo) :
         NetMessage(NETMSGTYPE_TruncLocalFile)
      {
         this->filesize = filesize;

         this->entryID = entryID.c_str();
         this->entryIDLen = entryID.length();

         this->targetID = targetID;

         this->pathInfoPtr = pathInfo;
      }
     
   protected:
      /**
       * For deserialization only!
       */
      TruncLocalFileMsg() : NetMessage(NETMSGTYPE_TruncLocalFile) {}


      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         unsigned retVal = NETMSG_HEADER_LENGTH +
            Serialization::serialLenInt64(); // filesize

         if(isMsgHeaderFeatureFlagSet(TRUNCLOCALFILEMSG_FLAG_USE_QUOTA) )
         {
            retVal += Serialization::serialLenUInt(); // userID
            retVal += Serialization::serialLenUInt(); // groupID
         }

         retVal += Serialization::serialLenStrAlign4(entryIDLen);
         retVal += this->pathInfoPtr->serialLen();
         retVal += Serialization::serialLenUShort(); // targetID

         return retVal;
      }

      unsigned getSupportedHeaderFeatureFlagsMask() const
      {
         return TRUNCLOCALFILEMSG_FLAG_NODYNAMICATTRIBS | TRUNCLOCALFILEMSG_FLAG_USE_QUOTA |
            TRUNCLOCALFILEMSG_FLAG_BUDDYMIRROR | TRUNCLOCALFILEMSG_FLAG_BUDDYMIRROR_SECOND;
      }


   private:
      int64_t filesize;

      unsigned entryIDLen;
      const char* entryID;

      uint16_t targetID;

      unsigned userID;
      unsigned groupID;

      // for serialization
      PathInfo* pathInfoPtr;

      // for deserialization
      PathInfo pathInfo;


   public:
   
      // getters & setters

      int64_t getFilesize() const
      {
         return filesize;
      }
      
      const char* getEntryID() const
      {
         return entryID;
      }

      uint16_t getTargetID() const
      {
         return targetID;
      }

      const PathInfo* getPathInfo() const
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
         this->addMsgHeaderFeatureFlag(TRUNCLOCALFILEMSG_FLAG_USE_QUOTA);

         this->userID = userID;
         this->groupID = groupID;
      }
};

#endif /*TRUNCLOCALFILEMSG_H_*/

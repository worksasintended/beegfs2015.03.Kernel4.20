#ifndef HARDLINKMSG_H_
#define HARDLINKMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/Path.h>

#define HARDLINK_FLAG_IS_TO_DENTRY_CREATE 1   /* Message sent from the from-dir meta-server
                                            * to create the dentry in the to-dir */

class HardlinkMsg : public NetMessage
{
   friend class AbstractNetMessageFactory;
   
   public:
      
      /**
       * Use this NetMsg from a client to create a hard link. NetMsg send to fromDir meta node.
       *
       * @param fromInfo just a reference, so do not free it as long as you use this object!
       * @param fromDirInfo just a reference, so do not free it as long as you use this object!
       * @param toDirInfo just a reference, so do not free it as long as you use this object!
       */
      HardlinkMsg(EntryInfo* fromDirInfo, std::string& fromName, EntryInfo* fromInfo,
          EntryInfo* toDirInfo, std::string& toName) : NetMessage(NETMSGTYPE_Hardlink)
      {
         this->fromName        = fromName;
         this->fromInfoPtr     = fromInfo;

         this->fromDirInfoPtr  = fromDirInfo;

         this->toName          = toName;

         this->toDirInfoPtr    = toDirInfo;
      }

      /**
       * Use this constructor to send a remote dentry create. NetMsg send to toDir meta node.
       *
       * @param fromInfo just a reference, so do not free it as long as you use this object!
       * @param fromDirInfo just a reference, so do not free it as long as you use this object!
       * @param toDirInfo just a reference, so do not free it as long as you use this object!
       */
      HardlinkMsg(EntryInfo* fromInfo, EntryInfo* toDirInfo, std::string& toName) :
         NetMessage(NETMSGTYPE_Hardlink)
      {
         this->addMsgHeaderFeatureFlag(HARDLINK_FLAG_IS_TO_DENTRY_CREATE);

         this->fromInfoPtr     = fromInfo;

         this->toDirInfoPtr    = toDirInfo;
         this->toName          = toName;

         this->fromDirInfoPtr = NULL; // not required for a link dentry
      }

      /*
       * For deserialization
       */
      HardlinkMsg() : NetMessage(NETMSGTYPE_Hardlink)
      {
      }

      virtual TestingEqualsRes testingEquals(NetMessage* cloneMsg);


   protected:


   protected:


      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         unsigned msgLen =  NETMSG_HEADER_LENGTH                           +
            this->fromInfoPtr->serialLen()                                 + // fromInfo
            this->toDirInfoPtr->serialLen()                                + // toDirInfo
            Serialization::serialLenStrAlign4(this->toName.length() );       // toName

         if (this->fromDirInfoPtr)
         {
            msgLen +=
               this->fromDirInfoPtr->serialLen()                           + // fromDirInfo
               Serialization::serialLenStrAlign4(this->fromName.length() );  // fromName

         }

         return msgLen;
      }


   private:

      std::string fromName;
      std::string toName;

      // for serialization
      EntryInfo* fromInfoPtr;    // not owned by this object
      EntryInfo* fromDirInfoPtr; // not owned by this object
      EntryInfo* toDirInfoPtr;   // not owned by this object

      // for deserialization
      EntryInfo fromInfo;
      EntryInfo fromDirInfo;
      EntryInfo toDirInfo;

   public:
   
      // inliners
      


      // getters & setters

      EntryInfo* getFromInfo()
      {
         return &this->fromInfo;
      }

      EntryInfo* getFromDirInfo()
      {
         return &this->fromDirInfo;
      }

      EntryInfo* getToDirInfo()
      {
         return &this->toDirInfo;
      }

      std::string getFromName()
      {
         return this->fromName;
      }

      std::string getToName()
      {
         return this->toName;
      }

      unsigned getSupportedHeaderFeatureFlagsMask() const
      {
         return HARDLINK_FLAG_IS_TO_DENTRY_CREATE;
      }

};


#endif /*HARDLINKMSG_H_*/

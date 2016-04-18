#ifndef RMDIRENTRYMSG_H_
#define RMDIRENTRYMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/Path.h>
#include <common/storage/EntryInfo.h>


class RmDirEntryMsg : public NetMessage
{
   friend class AbstractNetMessageFactory;

   public:

      /**
       * @param path just a reference, so do not free it as long as you use this object!
       */
      RmDirEntryMsg(EntryInfo* parentInfo, std::string& entryName) :
         NetMessage(NETMSGTYPE_RmDirEntry), entryName(entryName)
      {
         this->parentInfoPtr = parentInfo;

      }


   protected:
      RmDirEntryMsg() : NetMessage(NETMSGTYPE_RmDirEntry)
      {
      }


      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH                              +
            this->parentInfoPtr->serialLen()                       +
            Serialization::serialLenStrAlign4(this->entryName.length() );
      }


   private:

      std::string entryName;

      // for serialization
      EntryInfo* parentInfoPtr;

      // for deserialization
      EntryInfo parentInfo;



   public:

      std::string getEntryName() const
      {
         return this->entryName;
      }

      EntryInfo* getParentInfo()
      {
         return &this->parentInfo;
      }

      // getters & setters

};


#endif /* RMDIRENTRYMSG_H_ */

#ifndef REMOVEXATTRMSG_H_
#define REMOVEXATTRMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/EntryInfo.h>

class RemoveXAttrMsg : public NetMessage
{
   friend class AbstractNetMessageFactory;

   public:
      /**
       * @param entryInfo just a reference, so do not free it as long as you use this object!
       */
      RemoveXAttrMsg(EntryInfo* entryInfo, const std::string& name)
            : NetMessage(NETMSGTYPE_RemoveXAttr)
      {
         this->entryInfoPtr = entryInfo;
         this->name = name;
      }

   protected:
      /**
       * For deserialization only!
       */
      RemoveXAttrMsg() : NetMessage(NETMSGTYPE_RemoveXAttr) {}

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
               this->entryInfoPtr->serialLen() + // entryInfo
               Serialization::serialLenStr(this->name.length() ); // name
      }

   private:

      // for serialization
      EntryInfo* entryInfoPtr;

      // for deserialization
      EntryInfo entryInfo;

      std::string name;

   public:
      // getters and setters

      EntryInfo* getEntryInfo()
      {
         return &this->entryInfo;
      }

      const std::string& getName(void)
      {
         return this->name;
      }
};

#endif /*REMOVEXATTRMSG_H_*/

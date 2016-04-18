#ifndef GETXATTRMSG_H_
#define GETXATTRMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/EntryInfo.h>

class GetXAttrMsg : public NetMessage
{
   friend class AbstractNetMessageFactory;

   public:
      /**
       * @param entryInfo just a reference, so do not free it as long as you use this object!
       */
      GetXAttrMsg(EntryInfo* entryInfo, const std::string& name, int size)
         : NetMessage(NETMSGTYPE_GetXAttr),
           entryInfoPtr(entryInfo), size(size), name(name)
      {
      }


   protected:
      /**
       * For deserialization only!
       */
      GetXAttrMsg() : NetMessage(NETMSGTYPE_GetXAttr) {}


      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            this->entryInfoPtr->serialLen() +
            Serialization::serialLenStr(this->name.length() ) + // name
            Serialization::serialLenInt(); // size
      }


   private:

      // for serialization
      EntryInfo* entryInfoPtr;

      // for deserialization
      EntryInfo entryInfo;

      int size;
      std::string name;

   public:
      // getters and setters

      EntryInfo* getEntryInfo(void)
      {
         return &this->entryInfo;
      }

      const std::string& getName(void) const
      {
         return this->name;
      }

      int getSize(void) const
      {
         return this->size;
      }
};

#endif /*LISTXATTRMSG_H_*/

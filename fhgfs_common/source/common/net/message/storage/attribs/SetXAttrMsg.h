#ifndef SETXATTRMSG_H_
#define SETXATTRMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/EntryInfo.h>

class SetXAttrMsg : public NetMessage
{
   friend class AbstractNetMessageFactory;

   public:
      /**
       * @param entryInfo just a reference, do not free it as long as you use this object!
       */
      SetXAttrMsg(EntryInfo* entryInfo, const std::string& name, const CharVector& value, int flags)
            : NetMessage(NETMSGTYPE_SetXAttr)
      {
         this->entryInfoPtr = entryInfo;
         this->name = name;
         this->value = value;
         this->flags = flags;
      }

   protected:
      /**
       * For deserialization only!
       */
      SetXAttrMsg() : NetMessage(NETMSGTYPE_SetXAttr) {}

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
               this->entryInfoPtr->serialLen() +
               Serialization::serialLenStr(this->name.length() ) + // name
               Serialization::serialLenCharVector(&this->value) + // value
               Serialization::serialLenInt(); // flags
      }

      private:
         // for serialization
         EntryInfo* entryInfoPtr;

         // for deserialization
         EntryInfo entryInfo;

         std::string name;
         CharVector value;
         int flags;

      public:
         // getters and setters

         EntryInfo* getEntryInfo(void)
         {
            return &this->entryInfo;
         }

         const std::string& getName(void)
         {
            return this->name;
         }

         const CharVector& getValue(void)
         {
            return this->value;
         }

         int getFlags(void)
         {
            return this->flags;
         }
};

#endif /*SETXATTRMSG_H_*/

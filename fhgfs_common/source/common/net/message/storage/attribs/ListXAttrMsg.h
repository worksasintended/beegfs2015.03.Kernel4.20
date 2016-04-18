#ifndef LISTXATTRMSG_H_
#define LISTXATTRMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/EntryInfo.h>

class ListXAttrMsg : public NetMessage
{
   friend class AbstractNetMessageFactory;

   public:
      /**
       * @param entryInfo just a reference, so do not free it as long as you use this object!
       */
      ListXAttrMsg(EntryInfo* entryInfo, int size) : NetMessage(NETMSGTYPE_ListXAttr),
         entryInfoPtr(entryInfo), size(size)
      {
      }


   protected:
      /**
       * For deserialization only!
       */
      ListXAttrMsg() : NetMessage(NETMSGTYPE_ListXAttr) {}


      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            this->entryInfoPtr->serialLen() +
            Serialization::serialLenInt(); // size
      }


   private:

      // for serialization
      EntryInfo* entryInfoPtr;

      // for deserialization
      EntryInfo entryInfo;

      int size;

   public:
      // getters and setters

      EntryInfo* getEntryInfo(void)
      {
         return &this->entryInfo;
      }

      int getSize(void) const
      {
         return this->size;
      }
};

#endif /*LISTXATTRMSG_H_*/

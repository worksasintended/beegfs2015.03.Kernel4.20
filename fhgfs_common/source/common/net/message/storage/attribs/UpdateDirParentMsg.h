#ifndef UPDATEDIRPARENTMSG_H
#define UPDATEDIRPARENTMSG_H_

#include <common/net/message/NetMessage.h>

class UpdateDirParentMsg : public NetMessage
{
   public:

      /**
       * @param entryInfoPtr just a reference, so do not free it as long as you use this object!
       */
      UpdateDirParentMsg(EntryInfo* entryInfoPtr, uint16_t parentOwnerNodeID) :
         NetMessage(NETMSGTYPE_UpdateDirParent)
      {
         this->entryInfoPtr      = entryInfoPtr;
         this->parentOwnerNodeID = parentOwnerNodeID;
      }

   protected:
      UpdateDirParentMsg() : NetMessage(NETMSGTYPE_UpdateDirParent) {}

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH         +
            entryInfoPtr->serialLen()       +
            Serialization::serialLenUInt16();
      }


   private:

      uint16_t parentOwnerNodeID;

      // for serialization
      EntryInfo *entryInfoPtr;

      // for deserialization
      EntryInfo entryInfo;

   public:

      // getters & setters
      EntryInfo* getEntryInfo()
      {
         return &this->entryInfo;
      }

      uint16_t getParentNodeID()
      {
         return parentOwnerNodeID;
      }
};



#endif // UPDATEDIRPARENTMSG_H_

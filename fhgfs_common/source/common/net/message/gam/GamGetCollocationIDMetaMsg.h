#ifndef GAMGETCHUNKCOLLOCATIONIDMETAMSG_H
#define GAMGETCHUNKCOLLOCATIONIDMETAMSG_H

#include <common/net/message/NetMessage.h>

class GamGetCollocationIDMetaMsg : public NetMessage
{
   public:
      GamGetCollocationIDMetaMsg(EntryInfoList* entryInfoList) :
         NetMessage(NETMSGTYPE_GamGetCollocationIDMeta)
      {
         this->entryInfoList = entryInfoList;
      }

      GamGetCollocationIDMetaMsg() : NetMessage(NETMSGTYPE_GamGetCollocationIDMeta)
      {
      }

      virtual TestingEqualsRes testingEquals(NetMessage *msg);
   protected:

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
             Serialization::serialLenEntryInfoList(entryInfoList);
      }


   private:
      EntryInfoList* entryInfoList;

      //for deserialization
      unsigned int entryInfoListElemNum;
      const char* entryInfoListStart;
      unsigned int entryInfoListLen;

   public:

      // getters & setters
      void parseEntryInfoList(EntryInfoList* outList)
      {
         Serialization::deserializeEntryInfoList(entryInfoListLen, entryInfoListElemNum,
            entryInfoListStart, outList);
      }
};


#endif /*GAMGETCHUNKCOLLOCATIONIDMETAMSG_H*/

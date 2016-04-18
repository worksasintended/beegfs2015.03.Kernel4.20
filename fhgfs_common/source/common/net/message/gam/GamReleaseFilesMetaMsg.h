#ifndef GAMRELEASEFILESMETAMSG_H
#define GAMRELEASEFILESMETAMSG_H

#include <common/net/message/NetMessage.h>

class GamReleaseFilesMetaMsg : public NetMessage
{
   public:
      GamReleaseFilesMetaMsg(EntryInfoList *entryInfoList, uint8_t urgency) :
         NetMessage(NETMSGTYPE_GamReleaseFilesMeta)
      {
         this->entryInfoList = entryInfoList;
         this->urgency = urgency;
      }

      GamReleaseFilesMetaMsg(EntryInfoList *entryInfoList) :
         NetMessage(NETMSGTYPE_GamReleaseFilesMeta)
      {
         this->entryInfoList = entryInfoList;
         this->urgency = 0;
      }

      virtual TestingEqualsRes testingEquals(NetMessage *msg);

   protected:
      GamReleaseFilesMetaMsg() : NetMessage(NETMSGTYPE_GamReleaseFilesMeta)
      {
      }


      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
             Serialization::serialLenEntryInfoList(entryInfoList) +
             Serialization::serialLenUInt8();
      }


   private:
      EntryInfoList *entryInfoList;
      uint8_t urgency;

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

      uint8_t getUrgency()
      {
         return urgency;
      }
};


#endif /*GAMRELEASEFILESMETAMSG_H*/

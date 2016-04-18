#ifndef GAMRECALLEDFILESMETAMSG_H
#define GAMRECALLEDFILESMETAMSG_H

#include <common/net/message/NetMessage.h>

class GamRecalledFilesMetaMsg : public NetMessage
{
   public:
      GamRecalledFilesMetaMsg(EntryInfoList *entryInfoList) :
         NetMessage(NETMSGTYPE_GamRecalledFilesMeta)
      {
         this->entryInfoList = entryInfoList;
      }

      virtual TestingEqualsRes testingEquals(NetMessage *msg);

   protected:
      GamRecalledFilesMetaMsg() : NetMessage(NETMSGTYPE_GamRecalledFilesMeta)
      {
      }


      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
             Serialization::serialLenEntryInfoList(entryInfoList);
      }


   private:
      EntryInfoList *entryInfoList;

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


#endif /*GAMRECALLEDFILESMETAMSG_H*/

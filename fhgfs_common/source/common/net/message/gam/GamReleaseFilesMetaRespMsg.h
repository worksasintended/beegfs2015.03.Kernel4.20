#ifndef GAMRELEASEFILESMETARESPMSG_H
#define GAMRELEASEFILESMETARESPMSG_H

#include <common/net/message/NetMessage.h>

class GamReleaseFilesMetaRespMsg : public NetMessage
{
   public:
      GamReleaseFilesMetaRespMsg(EntryInfoList *entryInfoList) :
         NetMessage(NETMSGTYPE_GamReleaseFilesMetaResp)
      {
         this->entryInfoList = entryInfoList;
      }

      virtual TestingEqualsRes testingEquals(NetMessage *msg);

      GamReleaseFilesMetaRespMsg() : NetMessage(NETMSGTYPE_GamReleaseFilesMetaResp)
      {
      }

   protected:
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


#endif /*GAMRELEASEFILESMETARESPMSG_H*/

#ifndef GAMGETCOLLOCATIONIDMETARESPMSG_H
#define GAMGETCOLLOCATIONIDMETARESPMSG_H

#include <common/net/message/NetMessage.h>

class GamGetCollocationIDMetaRespMsg : public NetMessage
{
   public:
      GamGetCollocationIDMetaRespMsg(EntryInfoList *entryInfoList, UShortVector* collocationIDs) :
         NetMessage(NETMSGTYPE_GamGetCollocationIDMetaResp)
      {
         this->entryInfoList = entryInfoList;
         this->collocationIDs = collocationIDs;
      }

      GamGetCollocationIDMetaRespMsg() : NetMessage(NETMSGTYPE_GamGetCollocationIDMetaResp)
      {
      }

      virtual TestingEqualsRes testingEquals(NetMessage *msg);
   protected:

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
         Serialization::serialLenEntryInfoList(entryInfoList) +
         Serialization::serialLenUShortVector(collocationIDs);
      }


   private:
      EntryInfoList* entryInfoList;
      UShortVector* collocationIDs;

      //for deserialization
      unsigned int entryInfoListElemNum;
      const char* entryInfoListStart;
      unsigned int entryInfoListLen;

      unsigned int collocationIDsElemNum;
      const char* collocationIDsStart;
      unsigned int collocationIDsLen;
   public:

      // getters & setters
      void parseEntryInfoList(EntryInfoList* outList)
      {
         Serialization::deserializeEntryInfoList(entryInfoListLen, entryInfoListElemNum,
            entryInfoListStart, outList);
      }

      void parseCollocationIDs(UShortVector* outVec)
      {
         Serialization::deserializeUShortVector(collocationIDsLen, collocationIDsElemNum,
            collocationIDsStart, outVec);
      }
};


#endif /*GAMGETCOLLOCATIONIDMETARESPMSG_H*/

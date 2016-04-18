#ifndef GAMSETCOLLOCATIONIDMSG_H
#define GAMSETCOLLOCATIONIDMSG_H

#include <common/net/message/NetMessage.h>
#include <common/storage/HsmFileMetaData.h>

class GamSetCollocationIDMsg : public NetMessage
{
   public:
      GamSetCollocationIDMsg(StringVector* dirEntryIDs,
         GamCollocationIDVector* collocationIDs) : NetMessage(NETMSGTYPE_GamSetCollocationID)
      {
         this->dirEntryIDs = dirEntryIDs;
         this->collocationIDs = collocationIDs;
      }

      virtual TestingEqualsRes testingEquals(NetMessage *msg);

   protected:
      GamSetCollocationIDMsg() : NetMessage(NETMSGTYPE_GamSetCollocationID)
      {
      }

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenStringVec(dirEntryIDs) +
            Serialization::serialLenUShortVector(collocationIDs);
      }


   private:
      StringVector* dirEntryIDs;
      GamCollocationIDVector* collocationIDs;

      //for deserialization
      unsigned int dirEntryIDsElemNum;
      const char* dirEntryIDsStart;
      unsigned int dirEntryIDsLen;

      unsigned int collocationIDsElemNum;
      const char* collocationIDsStart;
      unsigned int collocationIDsLen;

   public:
      // getters & setters
      void parseDirEntryIDs(StringVector* outVec)
      {
         Serialization::deserializeStringVec(dirEntryIDsLen, dirEntryIDsElemNum,
            dirEntryIDsStart, outVec);
      }

      void parseCollocationIDs(UShortVector* outVec)
      {
         Serialization::deserializeUShortVector(collocationIDsLen, collocationIDsElemNum,
            collocationIDsStart, outVec);
      }
};


#endif /*GAMSETCOLLOCATIONIDMSG_H*/

#ifndef GAMGETCHUNKCOLLOCATIONIDRESPMSG_H
#define GAMGETCHUNKCOLLOCATIONIDRESPMSG_H

#include <common/net/message/NetMessage.h>

class GamGetChunkCollocationIDRespMsg : public NetMessage
{
   public:
      GamGetChunkCollocationIDRespMsg(StringVector *chunkIDs, UShortVector* collocationIDs) :
         NetMessage(NETMSGTYPE_GamGetChunkCollocationIDResp)
      {
         this->chunkIDs = chunkIDs;
         this->collocationIDs = collocationIDs;
      }

      GamGetChunkCollocationIDRespMsg() : NetMessage(NETMSGTYPE_GamGetChunkCollocationIDResp)
      {
      }

      virtual TestingEqualsRes testingEquals(NetMessage *msg);
   protected:

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
         Serialization::serialLenStringVec(chunkIDs) +
         Serialization::serialLenUShortVector(collocationIDs);
      }


   private:
      StringVector* chunkIDs;
      UShortVector* collocationIDs;

      //for deserialization
      unsigned int chunkIDsElemNum;
      const char* chunkIDsStart;
      unsigned int chunkIDsLen;

      unsigned int collocationIDsElemNum;
      const char* collocationIDsStart;
      unsigned int collocationIDsLen;
   public:

      // getters & setters
      void parseChunkIDs(StringVector* outVec)
      {
         Serialization::deserializeStringVec(chunkIDsLen, chunkIDsElemNum,
            chunkIDsStart, outVec);
      }

      void parseCollocationIDs(UShortVector* outVec)
      {
         Serialization::deserializeUShortVector(collocationIDsLen, collocationIDsElemNum,
            collocationIDsStart, outVec);
      }
};


#endif /*GAMGETCHUNKCOLLOCATIONIDRESPMSG_H*/

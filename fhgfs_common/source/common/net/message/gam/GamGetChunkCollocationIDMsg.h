#ifndef GAMGETCHUNKCOLLOCATIONIDMSG_H
#define GAMGETCHUNKCOLLOCATIONIDMSG_H

#include <common/net/message/NetMessage.h>

class GamGetChunkCollocationIDMsg : public NetMessage
{
   public:
      GamGetChunkCollocationIDMsg(uint16_t targetID, StringVector *chunkIDs) :
         NetMessage(NETMSGTYPE_GamGetChunkCollocationID)
      {
         this->targetID = targetID;
         this->chunkIDs = chunkIDs;
      }

      GamGetChunkCollocationIDMsg() : NetMessage(NETMSGTYPE_GamGetChunkCollocationID)
      {
      }

      virtual TestingEqualsRes testingEquals(NetMessage *msg);
   protected:

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenUShort() + // targetID
            Serialization::serialLenStringVec(chunkIDs); // chunkIDs
      }


   private:
      uint16_t targetID;
      StringVector *chunkIDs;

      //for deserialization
      unsigned int chunkIDsElemNum;
      const char* chunkIDsStart;
      unsigned int chunkIDsLen;

   public:

      // getters & setters
      void parseChunkIDs(StringVector* outVec)
      {
         Serialization::deserializeStringVec(chunkIDsLen, chunkIDsElemNum,
            chunkIDsStart, outVec);
      }

      uint16_t getTargetID()
      {
         return targetID;
      }
};


#endif /*GAMGETCHUNKCOLLOCATIONIDMSG_H*/

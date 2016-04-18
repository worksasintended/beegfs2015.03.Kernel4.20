#ifndef GAMRECALLEDFILESMSG_H
#define GAMRECALLEDFILESMSG_H

#include <common/net/message/NetMessage.h>

class GamRecalledFilesMsg : public NetMessage
{
   public:
      GamRecalledFilesMsg(uint16_t targetID, StringVector *chunkIDVec):
         NetMessage(NETMSGTYPE_GamRecalledFiles)
      {
         this->targetID = targetID;
         this->chunkIDVec = chunkIDVec;
      }

      virtual TestingEqualsRes testingEquals(NetMessage *msg);

   protected:
      GamRecalledFilesMsg() : NetMessage(NETMSGTYPE_GamRecalledFiles)
      {
      }

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenUShort() + // targetID
            Serialization::serialLenStringVec(chunkIDVec); // chunkIDVec
      }


   private:
      uint16_t targetID;
      StringVector *chunkIDVec;

      //for deserialization
      unsigned int chunkIDVecElemNum;
      const char* chunkIDVecStart;
      unsigned int chunkIDVecLen;

   public:

      // getters & setters
      void parseChunkIDVec(StringVector* outVec)
      {
         Serialization::deserializeStringVec(chunkIDVecLen, chunkIDVecElemNum, chunkIDVecStart,
            outVec);
      }

      uint16_t getTargetID()
      {
         return targetID;
      }
};


#endif /*GAMRECALLEDFILESMSG_H*/

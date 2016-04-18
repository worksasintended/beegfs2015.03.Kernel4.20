#ifndef GAMRELEASEFILESMSG_H
#define GAMRELEASEFILESMSG_H

#include <common/net/message/NetMessage.h>

class GamReleaseFilesMsg : public NetMessage
{
   public:
      GamReleaseFilesMsg(uint16_t targetID, StringVector *chunkIDVec, uint8_t urgency) :
         NetMessage(NETMSGTYPE_GamReleaseFiles)
      {
         this->targetID = targetID;
         this->chunkIDVec = chunkIDVec;
         this->urgency = urgency;
      }

      GamReleaseFilesMsg(uint16_t targetID, StringVector *chunkIDVec) :
         NetMessage(NETMSGTYPE_GamReleaseFiles)
      {
         this->targetID = targetID;
         this->chunkIDVec = chunkIDVec;
         this->urgency = 0;
      }

      virtual TestingEqualsRes testingEquals(NetMessage *msg);

   protected:
      GamReleaseFilesMsg() : NetMessage(NETMSGTYPE_GamReleaseFiles)
      {
      }


      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenUShort() + // targetID
            Serialization::serialLenStringVec(chunkIDVec) + // chunkIDVec
            Serialization::serialLenUInt8(); // urgency
      }


   private:
      uint16_t targetID;
      StringVector *chunkIDVec;
      uint8_t urgency;

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

      uint8_t getUrgency()
      {
         return urgency;
      }

      uint16_t getTargetID()
      {
         return targetID;
      }
};


#endif /*GAMRELEASEFILESMSG_H*/

#ifndef GAMRELEASEFILESRESPMSG_H
#define GAMRELEASEFILESRESPMSG_H

#include <common/net/message/NetMessage.h>

class GamReleaseFilesRespMsg : public NetMessage
{
   public:
      GamReleaseFilesRespMsg(StringVector *chunkIDVec) : NetMessage(NETMSGTYPE_GamReleaseFilesResp)
      {
         this->chunkIDVec = chunkIDVec;
      }

      GamReleaseFilesRespMsg() : NetMessage(NETMSGTYPE_GamReleaseFilesResp)
      {
      }

      virtual TestingEqualsRes testingEquals(NetMessage *msg);

   protected:


      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
             Serialization::serialLenStringVec(chunkIDVec);
      }


   private:
      StringVector *chunkIDVec;

      //for deserialization
      unsigned int chunkIDVecElemNum;
      const char* chunkIDVecStart;
      unsigned int chunkIDVecLen;

   public:

      // getters & setters
      void parseChunkIDVec(StringVector* outVector)
      {
         Serialization::deserializeStringVec(chunkIDVecLen, chunkIDVecElemNum, chunkIDVecStart,
            outVector);
      }
};


#endif /*GAMRELEASEFILESRESPMSG_H*/

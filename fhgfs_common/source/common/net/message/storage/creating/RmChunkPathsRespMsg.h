#ifndef RMCHUNKPATHSRESPMSG_H
#define RMCHUNKPATHSRESPMSG_H

#include <common/net/message/NetMessage.h>
#include <common/toolkit/serialization/Serialization.h>

class RmChunkPathsRespMsg : public NetMessage
{
   public:
      RmChunkPathsRespMsg(StringList* failedPaths) : NetMessage(NETMSGTYPE_RmChunkPathsResp)
      {
         this->failedPaths = failedPaths;
      }

      RmChunkPathsRespMsg() : NetMessage(NETMSGTYPE_RmChunkPathsResp)
      {
      }


      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenStringList(failedPaths); // failedPaths
      }


   private:
      // for serialization
      StringList* failedPaths;

      // for deserialization
      unsigned failedPathsBufLen;
      unsigned failedPathsElemNum;
      const char* failedPathsStart;

   public:
      void parseFailedPaths(StringList* outPaths)
      {
         Serialization::deserializeStringList(failedPathsBufLen, failedPathsElemNum,
            failedPathsStart, outPaths);
      }
};


#endif /*RMCHUNKPATHSRESPMSG_H*/

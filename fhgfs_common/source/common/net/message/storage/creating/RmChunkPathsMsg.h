#ifndef RMCHUNKPATHSMSG_H
#define RMCHUNKPATHSMSG_H

#include <common/net/message/NetMessage.h>
#include <common/toolkit/serialization/Serialization.h>

#define RMCHUNKPATHSMSG_FLAG_BUDDYMIRROR        1 /* given targetID is a buddymirrorgroup ID */

class RmChunkPathsMsg : public NetMessage
{
   public:
      RmChunkPathsMsg(uint16_t targetID, StringList* relativePaths) :
         NetMessage(NETMSGTYPE_RmChunkPaths)
      {
         this->targetID = targetID;
         this->relativePaths = relativePaths;
      }


   protected:
      RmChunkPathsMsg() : NetMessage(NETMSGTYPE_RmChunkPaths)
      {
      }


      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenUInt16()                       + //targetID
            Serialization::serialLenStringList(this->relativePaths); // relativePaths
      }

      unsigned getSupportedHeaderFeatureFlagsMask() const
      {
         return RMCHUNKPATHSMSG_FLAG_BUDDYMIRROR;
      }

   private:
      uint16_t targetID;

      // for serialization
      StringList* relativePaths;

      // for deserialization
      unsigned relativePathsBufLen;
      unsigned relativePathsElemNum;
      const char* relativePathsStart;

   public:
      uint16_t getTargetID() const
      {
         return targetID;
      }

      void parseRelativePaths(StringList* outPaths)
      {
         Serialization::deserializeStringList(relativePathsBufLen, relativePathsElemNum,
            relativePathsStart, outPaths);
      }
};


#endif /*RMCHUNKPATHSMSG_H*/

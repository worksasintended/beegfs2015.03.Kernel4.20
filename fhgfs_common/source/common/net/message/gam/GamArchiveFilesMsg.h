#ifndef GAMARCHIVEFILESMSG_H
#define GAMARCHIVEFILESMSG_H

#include <common/net/message/NetMessage.h>

class GamArchiveFilesMsg : public NetMessage
{
   public:
      GamArchiveFilesMsg(StringVector* chunkIDs, UShortVector* collocationIDs) :
         NetMessage(NETMSGTYPE_GamArchiveFiles)
      {
         this->chunkIDs = chunkIDs;
         this->collocationIDs = collocationIDs;
      }

      virtual TestingEqualsRes testingEquals(NetMessage *msg);

   protected:
      GamArchiveFilesMsg() : NetMessage(NETMSGTYPE_GamArchiveFiles)
      {
      }

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


#endif /*GAMARCHIVEFILESMSG_H*/

#ifndef GAMSETCOLLOCATIONIDRESPMSG_H
#define GAMSETCOLLOCATIONIDRESPMSG_H

#include <common/net/message/NetMessage.h>

class GamSetCollocationIDRespMsg : public NetMessage
{
   public:
      GamSetCollocationIDRespMsg(StringVector *failedDirEntryIDs) :
         NetMessage(NETMSGTYPE_GamSetCollocationIDResp)
      {
         this->dirEntryIDs = failedDirEntryIDs;
      }

      GamSetCollocationIDRespMsg() : NetMessage(NETMSGTYPE_GamSetCollocationIDResp)
      {
      }

      virtual TestingEqualsRes testingEquals(NetMessage *msg);
   protected:

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
             Serialization::serialLenStringVec(dirEntryIDs);
      }


   private:
      StringVector *dirEntryIDs;

      //for deserialization
      unsigned int dirEntryIDsElemNum;
      const char* dirEntryIDsStart;
      unsigned int dirEntryIDsLen;

   public:

      // getters & setters
      void parseFailedDirEntryIDs(StringVector* outVec)
      {
         Serialization::deserializeStringVec(dirEntryIDsLen, dirEntryIDsElemNum,
            dirEntryIDsStart, outVec);
      }
};


#endif /*GAMSETCOLLOCATIONIDRESPMSG_H*/

#ifndef RETRIEVEINODESMSG_H
#define RETRIEVEINODESMSG_H

#include <common/net/message/NetMessage.h>

class RetrieveInodesMsg : public NetMessage
{
   public:
      RetrieveInodesMsg(unsigned hashDirNum, int64_t lastOffset, unsigned maxOutInodes):
         NetMessage(NETMSGTYPE_RetrieveInodes)
      {
         this->hashDirNum = hashDirNum;
         this->lastOffset = lastOffset;
         this->maxOutInodes = maxOutInodes;
      }

      RetrieveInodesMsg() : NetMessage(NETMSGTYPE_RetrieveInodes)
      {
      }

   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
         Serialization::serialLenUInt() + Serialization::serialLenInt64() +
         Serialization::serialLenUInt();
      }


   private:
      unsigned hashDirNum;
      int64_t lastOffset;
      unsigned maxOutInodes;

   public:
      uint getHashDirNum()
      {
         return hashDirNum;
      }

      int64_t getLastOffset()
      {
         return lastOffset;
      }

      uint getMaxOutInodes()
      {
         return maxOutInodes;
      }

      virtual TestingEqualsRes testingEquals(NetMessage* msg)
      {
         RetrieveInodesMsg* msgIn = (RetrieveInodesMsg*) msg;

         if ( this->hashDirNum != msgIn->getHashDirNum() )
            return TestingEqualsRes_FALSE;

         if ( this->lastOffset != msgIn->getLastOffset() )
            return TestingEqualsRes_FALSE;

         if ( this->maxOutInodes != msgIn->getMaxOutInodes() )
            return TestingEqualsRes_FALSE;

         return TestingEqualsRes_TRUE;
      }
};


#endif /*RETRIEVEINODESMSG_H*/

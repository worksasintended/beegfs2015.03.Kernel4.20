#ifndef ADJUSTCHUNKPERMISSIONSRESPMSG_H
#define ADJUSTCHUNKPERMISSIONSRESPMSG_H

#include <common/net/message/NetMessage.h>
#include <common/toolkit/serialization/SerializationFsck.h>

class AdjustChunkPermissionsRespMsg : public NetMessage
{
   public:
      AdjustChunkPermissionsRespMsg(unsigned count, std::string& currentContDirID,
         int64_t newHashDirOffset, int64_t newContDirOffset, unsigned errorCount) :
            NetMessage(NETMSGTYPE_AdjustChunkPermissionsResp)
      {
         this->count = count;
         this->currentContDirID = currentContDirID.c_str();
         this->currentContDirIDLen = currentContDirID.length();
         this->newHashDirOffset = newHashDirOffset;
         this->newContDirOffset = newContDirOffset;
         this->errorCount = errorCount;
      }

      AdjustChunkPermissionsRespMsg() : NetMessage(NETMSGTYPE_AdjustChunkPermissionsResp)
      {
      }

   protected:

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      virtual unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenStrAlign4(currentContDirIDLen) + // currentContDirID
            Serialization::serialLenUInt() + //count
            Serialization::serialLenInt64() + // newHashDirOffset
            Serialization::serialLenInt64() + // newContDirOffset
            Serialization::serialLenUInt(); // errorCount
      }

   private:
      const char* currentContDirID;
      unsigned currentContDirIDLen;
      unsigned count;
      int64_t newHashDirOffset;
      int64_t newContDirOffset;
      unsigned errorCount;

   public:
      // getters & setters
      unsigned getCount() const
      {
         return count;
      }

      int64_t getNewHashDirOffset() const
      {
         return newHashDirOffset;
      }

      int64_t getNewContDirOffset() const
      {
         return newContDirOffset;
      }

      std::string getCurrentContDirID() const
      {
         return currentContDirID;
      }

      bool getErrorCount() const
      {
         return errorCount;
      }

      // inliner
      virtual TestingEqualsRes testingEquals(NetMessage* msg)
      {
         AdjustChunkPermissionsRespMsg* msgIn = (AdjustChunkPermissionsRespMsg*) msg;

         if ( this->count != msgIn->getCount() )
            return TestingEqualsRes_FALSE;

         if ( this->currentContDirID != msgIn->getCurrentContDirID() )
            return TestingEqualsRes_FALSE;

         if ( this->newContDirOffset != msgIn->getNewContDirOffset() )
            return TestingEqualsRes_FALSE;

         if ( this->newHashDirOffset != msgIn->getNewHashDirOffset() )
            return TestingEqualsRes_FALSE;

         if ( this->errorCount != msgIn->getErrorCount() )
            return TestingEqualsRes_FALSE;

         return TestingEqualsRes_TRUE;
      }
};


#endif /*ADJUSTCHUNKPERMISSIONSRESPMSG_H*/

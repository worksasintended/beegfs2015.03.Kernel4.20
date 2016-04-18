#ifndef ADJUSTCHUNKPERMISSIONSMSG_H
#define ADJUSTCHUNKPERMISSIONSMSG_H

#include <common/net/message/NetMessage.h>

class AdjustChunkPermissionsMsg: public NetMessage
{
   public:
      AdjustChunkPermissionsMsg(unsigned hashDirNum, std::string& currentContDirID,
         unsigned maxEntries, int64_t lastHashDirOffset, int64_t lastContDirOffset) :
         NetMessage(NETMSGTYPE_AdjustChunkPermissions)
      {
         this->hashDirNum = hashDirNum;
         this->currentContDirID = currentContDirID.c_str();
         this->currentContDirIDLen = currentContDirID.length();
         this->maxEntries = maxEntries;
         this->lastHashDirOffset = lastHashDirOffset;
         this->lastContDirOffset = lastContDirOffset;
      }

      AdjustChunkPermissionsMsg() : NetMessage(NETMSGTYPE_AdjustChunkPermissions)
      {
      }


      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenStrAlign4(currentContDirIDLen) + // currentContDirID
            Serialization::serialLenUInt() + // hashDirNum
            Serialization::serialLenUInt() + // maxEntries
            Serialization::serialLenInt64() + //lastHashDirOffset
            Serialization::serialLenInt64(); // lastContDirOffset
    }

private:
    unsigned hashDirNum;
    const char *currentContDirID;
    unsigned currentContDirIDLen;
    unsigned maxEntries;
    int64_t lastHashDirOffset;
    int64_t lastContDirOffset;

public:
    std::string getCurrentContDirID() const
    {
        return currentContDirID;
    }

    unsigned getHashDirNum() const
    {
        return hashDirNum;
    }

    int64_t getLastContDirOffset() const
    {
        return lastContDirOffset;
    }

    int64_t getLastHashDirOffset() const
    {
        return lastHashDirOffset;
    }

    unsigned getMaxEntries() const
    {
        return maxEntries;
    }

    // inliner
    virtual TestingEqualsRes testingEquals(NetMessage* msg)
    {
       AdjustChunkPermissionsMsg* msgIn = (AdjustChunkPermissionsMsg*) msg;

       if ( this->currentContDirID != msgIn->getCurrentContDirID() )
          return TestingEqualsRes_FALSE;

       if ( this->hashDirNum != msgIn->getHashDirNum() )
          return TestingEqualsRes_FALSE;

       if ( this->lastContDirOffset != msgIn->getLastContDirOffset() )
          return TestingEqualsRes_FALSE;

       if ( this->lastHashDirOffset != msgIn->getLastHashDirOffset() )
          return TestingEqualsRes_FALSE;

       if ( this->maxEntries != msgIn->getMaxEntries() )
          return TestingEqualsRes_FALSE;

       return TestingEqualsRes_TRUE;
    }

};

#endif /*ADJUSTCHUNKPERMISSIONSMSG_H*/

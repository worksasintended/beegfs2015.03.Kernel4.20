#ifndef RETRIEVEDIRENTRIESMSG_H
#define RETRIEVEDIRENTRIESMSG_H

#include <common/net/message/NetMessage.h>
#include <common/storage/Path.h>
#include <common/storage/StorageDefinitions.h>

class RetrieveDirEntriesMsg : public NetMessage
{
   public:
      RetrieveDirEntriesMsg(unsigned hashDirNum, std::string& currentContDirID,
         unsigned maxOutEntries, int64_t lastHashDirOffset, int64_t lastContDirOffset) :
         NetMessage(NETMSGTYPE_RetrieveDirEntries)
      {
         this->hashDirNum = hashDirNum;
         this->currentContDirID = currentContDirID.c_str();
         this->currentContDirIDLen = currentContDirID.length();
         this->maxOutEntries = maxOutEntries;
         this->lastHashDirOffset = lastHashDirOffset;
         this->lastContDirOffset = lastContDirOffset;
      }

      RetrieveDirEntriesMsg() : NetMessage(NETMSGTYPE_RetrieveDirEntries)
      {
      }

   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenUInt() + // hashDirNum
            Serialization::serialLenStrAlign4(currentContDirIDLen) + // currentContDirID
            Serialization::serialLenUInt() + // maxOutEntries
            Serialization::serialLenInt64() + //lastHashDirOffset
            Serialization::serialLenInt64(); // lastContDirOffset
    }

private:
    unsigned hashDirNum;
    const char *currentContDirID;
    unsigned currentContDirIDLen;
    unsigned maxOutEntries;
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

    unsigned getMaxOutEntries() const
    {
        return maxOutEntries;
    }

    virtual TestingEqualsRes testingEquals(NetMessage* msg)
    {
       RetrieveDirEntriesMsg* msgIn = (RetrieveDirEntriesMsg*) msg;

       if ( this->currentContDirID != msgIn->getCurrentContDirID() )
          return TestingEqualsRes_FALSE;

       if ( this->hashDirNum != msgIn->getHashDirNum() )
          return TestingEqualsRes_FALSE;

       if ( this->lastContDirOffset != msgIn->getLastContDirOffset() )
          return TestingEqualsRes_FALSE;

       if ( this->lastHashDirOffset != msgIn->getLastHashDirOffset() )
          return TestingEqualsRes_FALSE;

       if ( this->maxOutEntries != msgIn->getMaxOutEntries() )
          return TestingEqualsRes_FALSE;

       return TestingEqualsRes_TRUE;
    }
};

#endif /*RETRIEVEDIRENTRIESMSG_H*/

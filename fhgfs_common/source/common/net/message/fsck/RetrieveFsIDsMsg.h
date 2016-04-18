#ifndef RETRIEVEFSIDSMSG_H
#define RETRIEVEFSIDSMSG_H

#include <common/net/message/NetMessage.h>
#include <common/storage/Path.h>
#include <common/storage/StorageDefinitions.h>

class RetrieveFsIDsMsg : public NetMessage
{
   public:
      RetrieveFsIDsMsg(unsigned hashDirNum, std::string& currentContDirID,
         unsigned maxOutIDs, int64_t lastHashDirOffset, int64_t lastContDirOffset) :
         NetMessage(NETMSGTYPE_RetrieveFsIDs)
      {
         this->hashDirNum = hashDirNum;
         this->currentContDirID = currentContDirID.c_str();
         this->currentContDirIDLen = currentContDirID.length();
         this->maxOutIDs = maxOutIDs;
         this->lastHashDirOffset = lastHashDirOffset;
         this->lastContDirOffset = lastContDirOffset;
      }

      RetrieveFsIDsMsg() : NetMessage(NETMSGTYPE_RetrieveFsIDs)
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
            Serialization::serialLenUInt() + // maxOutIDs
            Serialization::serialLenInt64() + //lastHashDirOffset
            Serialization::serialLenInt64(); // lastContDirOffset
    }

private:
    unsigned hashDirNum;
    const char *currentContDirID;
    unsigned currentContDirIDLen;
    unsigned maxOutIDs;
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

    unsigned getMaxOutIDs() const
    {
        return maxOutIDs;
    }

    virtual TestingEqualsRes testingEquals(NetMessage* msg)
    {
       RetrieveFsIDsMsg* msgIn = (RetrieveFsIDsMsg*) msg;

       if ( this->currentContDirID != msgIn->getCurrentContDirID() )
          return TestingEqualsRes_FALSE;

       if ( this->hashDirNum != msgIn->getHashDirNum() )
          return TestingEqualsRes_FALSE;

       if ( this->lastContDirOffset != msgIn->getLastContDirOffset() )
          return TestingEqualsRes_FALSE;

       if ( this->lastHashDirOffset != msgIn->getLastHashDirOffset() )
          return TestingEqualsRes_FALSE;

       if ( this->maxOutIDs != msgIn->getMaxOutIDs() )
          return TestingEqualsRes_FALSE;

       return TestingEqualsRes_TRUE;
    }
      
};

#endif /*RETRIEVEFSIDSMSG_H*/

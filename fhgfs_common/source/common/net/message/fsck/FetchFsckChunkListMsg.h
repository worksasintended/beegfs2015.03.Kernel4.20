#ifndef FETCHFSCKCHUNKLISTMSG_H
#define FETCHFSCKCHUNKLISTMSG_H

#include <common/net/message/NetMessage.h>
#include <common/toolkit/FsckTk.h>
#include <common/toolkit/serialization/Serialization.h>

#define FETCHFSCKCHUNKLISTMSG_COMPAT_FLAG_NO_FORCE_RESTART 1 // = !forceRestart in version 6

class FetchFsckChunkListMsg: public NetMessage
{
   public:
      /*
       * @param maxNumChunks max number of chunks to fetch at once
       */
      FetchFsckChunkListMsg(unsigned maxNumChunks, FetchFsckChunkListStatus lastStatus,
            bool forceRestart = true) :
         NetMessage(NETMSGTYPE_FetchFsckChunkList)
      {
         this->maxNumChunks = maxNumChunks;
         this->lastStatus = lastStatus;

         if (!forceRestart)
            addMsgHeaderCompatFeatureFlag(FETCHFSCKCHUNKLISTMSG_COMPAT_FLAG_NO_FORCE_RESTART);
      }

      // only for deserialization
      FetchFsckChunkListMsg() : NetMessage(NETMSGTYPE_FetchFsckChunkList)
      {
      }

   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenUInt() + // maxNumChunks
            Serialization::serialLenUInt(); // lastStatus
      }

   private:
      unsigned maxNumChunks;
      FetchFsckChunkListStatus lastStatus;

   public:
      // getter
      unsigned getMaxNumChunks()
      {
         return this->maxNumChunks;
      }

      FetchFsckChunkListStatus getLastStatus()
      {
         return this->lastStatus;
      }

      bool getForceRestart()
      {
         return
            !isMsgHeaderCompatFeatureFlagSet(FETCHFSCKCHUNKLISTMSG_COMPAT_FLAG_NO_FORCE_RESTART);
      }

      // inliner
      virtual TestingEqualsRes testingEquals(NetMessage* msg)
      {
         FetchFsckChunkListMsg* msgIn = (FetchFsckChunkListMsg*) msg;

         if ( maxNumChunks != msgIn->getMaxNumChunks() )
            return TestingEqualsRes_FALSE;

         if ( lastStatus != msgIn->getLastStatus() )
            return TestingEqualsRes_FALSE;

         return TestingEqualsRes_TRUE;
      }
};

#endif /*FETCHFSCKCHUNKLISTMSG_H*/

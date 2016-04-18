#ifndef FETCHFSCKCHUNKLISTRESPMSG_H
#define FETCHFSCKCHUNKLISTRESPMSG_H

#include <common/fsck/FsckChunk.h>
#include <common/net/message/NetMessage.h>
#include <common/toolkit/serialization/Serialization.h>
#include <common/toolkit/serialization/SerializationFsck.h>
#include <common/toolkit/FsckTk.h>
#include <common/toolkit/ListTk.h>

class FetchFsckChunkListRespMsg: public NetMessage
{
   public:
      FetchFsckChunkListRespMsg(FsckChunkList* chunkList, FetchFsckChunkListStatus status) :
         NetMessage(NETMSGTYPE_FetchFsckChunkListResp)
      {
         this->chunkList = chunkList;
         this->status = status;
      }

      // only for deserialization
      FetchFsckChunkListRespMsg() : NetMessage(NETMSGTYPE_FetchFsckChunkListResp)
      {
      }

   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            SerializationFsck::serialLenFsckChunkList(chunkList) + // chunkList
            Serialization::serialLenUInt(); // status
      }

   private:
      FsckChunkList* chunkList;
      FetchFsckChunkListStatus status;

      // for deserialization
      const char* chunkListStart;
      unsigned chunkListElemNum;

   public:
      // getter
      FetchFsckChunkListStatus getStatus()
      {
         return this->status;
      }

      void parseChunkList(FsckChunkList* outList)
      {
         SerializationFsck::deserializeFsckChunkList(chunkListElemNum,
            chunkListStart, outList);
      }

      // inliner
      virtual TestingEqualsRes testingEquals(NetMessage* msg)
      {
         FetchFsckChunkListRespMsg* msgIn = (FetchFsckChunkListRespMsg*) msg;

         FsckChunkList chunksIn;

         msgIn->parseChunkList(&chunksIn);

         if ( ! ListTk::listsEqual(this->chunkList, &chunksIn) )
            return TestingEqualsRes_FALSE;

         if ( this->status != msgIn->status )
            return TestingEqualsRes_FALSE;

         return TestingEqualsRes_TRUE;
      }
};

#endif /*FETCHFSCKCHUNKLISTRESPMSG_H*/

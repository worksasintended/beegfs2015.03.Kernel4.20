#ifndef RESYNCLOCALFILEMSG_H_
#define RESYNCLOCALFILEMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/PathInfo.h>

#define RESYNCLOCALFILEMSG_FLAG_SETATTRIBS 1 /* message contains SettableFileAttrib object, which
                                                should be applied to the chunk */
#define RESYNCLOCALFILEMSG_FLAG_NODATA     2 /* do not sync data, i.e. dataBuf, offset, count are
                                                ignored */
#define RESYNCLOCALFILEMSG_FLAG_TRUNC      4 /* truncate after write; cannot be used together with
                                                RESYNCLOCALFILEMSG_FLAG_NODATA */
#define RESYNCLOCALFILEMSG_CHECK_SPARSE    8 /* check if incoming data has sparse areas */

#define RESYNCER_SPARSE_BLOCK_SIZE 4096 //4K

class ResyncLocalFileMsg : public NetMessage
{
   public:
      /*
       * @param dataBuf the actual data to be written;may be NULL if RESYNCLOCALFILEMSG_FLAG_NODATA
       * @param relativePathStr path to chunk, relative to buddy mirror directory
       * @param resyncToTargetID
       * @param offset
       * @param count amount of data to write from dataBuf
       * @param chunkAttribs may be NULL, but only if FLAG_SETATTRIBS is not set
       */
      ResyncLocalFileMsg(const char* dataBuf, std::string& relativePathStr,
         uint16_t resyncToTargetID, int64_t offset, int count, SettableFileAttribs* chunkAttribs =
            NULL) :
         NetMessage(NETMSGTYPE_ResyncLocalFile)
      {
         this->dataBuf = dataBuf;

         this->relativePathStr = relativePathStr;

         this->resyncToTargetID = resyncToTargetID;
         this->offset = offset;
         this->count = count;

         if (chunkAttribs)
            this->chunkAttribs = *chunkAttribs;
      }

   protected:
      /**
       * For deserialization only!
       */
      ResyncLocalFileMsg() : NetMessage(NETMSGTYPE_ResyncLocalFile) {}

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         unsigned retVal = NETMSG_HEADER_LENGTH                         +
            Serialization::serialLenStrAlign4(relativePathStr.length()) + // relativePathStr
            Serialization::serialLenUInt16()                            + // resyncToTargetID
            Serialization::serialLenInt64()                             + // offset
            Serialization::serialLenInt();                                // count

         if ( this->isMsgHeaderFeatureFlagSet(RESYNCLOCALFILEMSG_FLAG_SETATTRIBS) )
         {
            retVal += Serialization::serialLenInt()                     + // mode
               Serialization::serialLenUInt()                           + // userID
               Serialization::serialLenUInt();                            // groupID
         }

         retVal += count;                                                 // dataBuf

         return retVal;
      }

      unsigned getSupportedHeaderFeatureFlagsMask() const
      {
         return RESYNCLOCALFILEMSG_FLAG_SETATTRIBS | RESYNCLOCALFILEMSG_FLAG_NODATA |
            RESYNCLOCALFILEMSG_FLAG_TRUNC | RESYNCLOCALFILEMSG_CHECK_SPARSE;
      }

   private:
      const char* dataBuf;

      std::string relativePathStr;
      uint16_t resyncToTargetID;
      int64_t offset;
      int count;

      SettableFileAttribs chunkAttribs;

   public:
      // getters & setters

      void setChunkAttribs(SettableFileAttribs& chunkAttribs)
      {
         this->chunkAttribs = chunkAttribs;
      }

      const char* getDataBuf() const
      {
         return dataBuf;
      }
      
      uint16_t getResyncToTargetID() const
      {
         return resyncToTargetID;
      }
      
      int64_t getOffset() const
      {
         return offset;
      }
      
      void setOffset(int64_t offset)
      {
         this->offset = offset;
      }

      int getCount() const
      {
         return count;
      }

      std::string getRelativePathStr() const
      {
         return relativePathStr;
      }

      SettableFileAttribs* getChunkAttribs()
      {
         return &chunkAttribs;
      }
};

#endif /*RESYNCLOCALFILEMSG_H_*/

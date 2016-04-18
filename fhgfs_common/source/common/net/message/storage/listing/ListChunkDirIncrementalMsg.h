#ifndef LISTCHUNKDIRINCREMENTALMSG_H_
#define LISTCHUNKDIRINCREMENTALMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/net/message/storage/listing/ListChunkDirIncrementalRespMsg.h>

#define LISTCHUNKDIRINCREMENTALMSG_FLAG_IGNORENOTEXISTS   1 /* ignore it, if requested dir does */
                                                            /*  not exist (not logged as error) */

class ListChunkDirIncrementalMsg : public NetMessage
{
   public:
      
      ListChunkDirIncrementalMsg(uint16_t targetID, bool isMirror, std::string& relativeDir,
         int64_t offset, unsigned maxOutEntries, bool onlyFiles) :
         NetMessage(NETMSGTYPE_ListChunkDirIncremental), targetID(targetID), isMirror(isMirror),
         relativeDir(relativeDir), offset(offset), maxOutEntries(maxOutEntries),
         onlyFiles(onlyFiles)
      {
         // all initialization done in initializer list
      }


   protected:
      /**
       * For deserialization only
       */
      ListChunkDirIncrementalMsg() : NetMessage(NETMSGTYPE_ListChunkDirIncremental)
      {
      }

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned getSupportedHeaderFeatureFlagsMask() const
      {
         return LISTCHUNKDIRINCREMENTALMSG_FLAG_IGNORENOTEXISTS;
      }

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenStrAlign4(relativeDir.length()) + // relativeDir
            Serialization::serialLenUInt16() +                        // targetID
            Serialization::serialLenBool() +                          // isMirror
            Serialization::serialLenInt64() +                         // offset
            Serialization::serialLenUInt()  +                         // maxOutEntries
            Serialization::serialLenBool();                           // onlyFiles
      }

   private:
      uint16_t targetID;
      bool isMirror;
      std::string relativeDir;
      int64_t offset;
      unsigned maxOutEntries;
      bool onlyFiles;

   public:
   
      // inliners   
      
      // getters & setters
      uint16_t getTargetID() const
      {
         return targetID;
      }

      bool getIsMirror() const
      {
         return isMirror;
      }

      int64_t getOffset() const
      {
         return offset;
      }

      unsigned getMaxOutEntries() const
      {
         return maxOutEntries;
      }

      std::string getRelativeDir() const
      {
         return relativeDir;
      }

      bool getOnlyFiles() const
      {
         return onlyFiles;
      }
};


#endif /*LISTCHUNKDIRINCREMENTALMSG_H_*/

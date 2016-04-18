#ifndef LISTDIRFROMPFFSETRESPMSG_H_
#define LISTDIRFROMPFFSETRESPMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/Common.h>

class ListDirFromOffsetRespMsg : public NetMessage
{
   public:
      ListDirFromOffsetRespMsg(FhgfsOpsErr result, StringList* names, UInt8List* entryTypes,
         StringList* entryIDs, Int64List* serverOffsets, int64_t newServerOffset) :
         NetMessage(NETMSGTYPE_ListDirFromOffsetResp)
      {
         this->result = result;
         this->names = names;
         this->entryIDs = entryIDs;
         this->entryTypes = entryTypes;
         this->serverOffsets = serverOffsets;
         this->newServerOffset = newServerOffset;
      }

      ListDirFromOffsetRespMsg() : NetMessage(NETMSGTYPE_ListDirFromOffsetResp)
      {
      }
   
   protected:

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      virtual unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH                                +
            Serialization::serialLenInt()                           + // result
            Serialization::serialLenUInt8List(this->entryTypes)     + // entryTypes
            Serialization::serialLenStringList(this->entryIDs)      + // entryIDs
            Serialization::serialLenStringList(this->names)         + // names
            Serialization::serialLenInt64List(this->serverOffsets)  + // serverOffsets
            Serialization::serialLenInt64();                          // newServerOffset
      }
      
   private:
      int result;
      
      int64_t newServerOffset;

      // for serialization
      StringList* names;    // not owned by this object!
      UInt8List* entryTypes;  // not owned by this object!
      StringList* entryIDs; // not owned by this object!
      Int64List* serverOffsets; // not owned by this object!
      
      // for deserialization
      unsigned namesElemNum;
      const char* namesListStart;
      unsigned namesBufLen;

      unsigned entryTypesElemNum;
      const char* entryTypesListStart;
      unsigned entryTypesBufLen;

      unsigned entryIDsElemNum;
      const char* entryIDsListStart;
      unsigned entryIDsBufLen;

      unsigned serverOffsetsElemNum;
      const char* serverOffsetsListStart;
      unsigned serverOffsetsBufLen;
      
   public:
      // inliners
      bool parseNames(StringList* outNames)
      {
         return Serialization::deserializeStringList(
            this->namesBufLen, this->namesElemNum, this->namesListStart, outNames);
      }
      
      bool parseEntryTypes(UInt8List* outEntryTypes)
      {
         return Serialization::deserializeUInt8List(this->entryTypesBufLen, this->entryTypesElemNum,
            this->entryTypesListStart, outEntryTypes);
      }

      bool parseEntryIDs(StringList* outEntryIDs)
      {
         return Serialization::deserializeStringList(this->entryIDsBufLen, this->entryIDsElemNum,
            this->entryIDsListStart, outEntryIDs);
      }

      bool parseServerOffsets(Int64List* outServerOffsets)
      {
         return Serialization::deserializeInt64List(this->serverOffsetsBufLen,
            this->serverOffsetsElemNum, this->serverOffsetsListStart, outServerOffsets);
      }

      // getters & setters
      FhgfsOpsErr getResult()
      {
         return (FhgfsOpsErr)this->result;
      }
      
      int64_t getNewServerOffset()
      {
         return this->newServerOffset;
      }
   
};

#endif /*LISTDIRFROMPFFSETRESPMSG_H_*/

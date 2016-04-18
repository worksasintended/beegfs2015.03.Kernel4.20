#ifndef LISTCHUNKDIRINCREMENTALRESPMSG_H_
#define LISTCHUNKDIRINCREMENTALRESPMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/Common.h>

class ListChunkDirIncrementalRespMsg : public NetMessage
{
   public:
      ListChunkDirIncrementalRespMsg(FhgfsOpsErr result, StringList* names, IntList* entryTypes,
         int64_t newOffset) : NetMessage(NETMSGTYPE_ListChunkDirIncrementalResp)
      {
         this->result = (int)result;
         this->names = names;
         this->entryTypes = entryTypes;
         this->newOffset = newOffset;
      }

      ListChunkDirIncrementalRespMsg() : NetMessage(NETMSGTYPE_ListChunkDirIncrementalResp)
      {
      }
   
   protected:

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      virtual unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH                           +
            Serialization::serialLenInt()                      + // result
            Serialization::serialLenStringList(names)          + // names
            Serialization::serialLenIntList(entryTypes)        + // entryTypes
            Serialization::serialLenInt64();                     // newOffset
      }
      
   private:
      int result;
      
      int64_t newOffset;

      // for serialization
      StringList* names;            // not owned by this object!
      IntList* entryTypes;          // not owned by this object!
      
      // for deserialization
      unsigned namesElemNum;
      const char* namesListStart;
      unsigned namesBufLen;
      
      // for deserialization
      unsigned entryTypesElemNum;
      const char* entryTypesListStart;
      unsigned entryTypesBufLen;

   public:
      // inliners
      bool parseNames(StringList* outNames)
      {
         return Serialization::deserializeStringList(namesBufLen, namesElemNum, namesListStart,
            outNames);
      }
      
      bool parseEntryTypes(IntList* outEntryTypes)
      {
         return Serialization::deserializeIntList(entryTypesBufLen, entryTypesElemNum,
            entryTypesListStart, outEntryTypes);
      }

      // getters & setters
      FhgfsOpsErr getResult()
      {
         return (FhgfsOpsErr)this->result;
      }
      
      int64_t getNewOffset()
      {
         return this->newOffset;
      }
   
};

#endif /*LISTCHUNKDIRINCREMENTALRESPMSG_H_*/

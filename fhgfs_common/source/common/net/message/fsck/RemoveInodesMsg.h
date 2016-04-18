#ifndef REMOVEINODESMSG_H
#define REMOVEINODESMSG_H

#include <common/net/message/NetMessage.h>
#include <common/toolkit/ListTk.h>

class RemoveInodesMsg : public NetMessage
{
   public:
      RemoveInodesMsg(StringList* entryIDList, DirEntryTypeList* entryTypeList) :
         NetMessage(NETMSGTYPE_RemoveInodes)
      {
         this->entryIDList = entryIDList;
         this->entryTypeList = (IntList*)entryTypeList;
      }

      RemoveInodesMsg() : NetMessage(NETMSGTYPE_RemoveInodes)
      {
      }

   protected:

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH + Serialization::serialLenStringList(entryIDList)
            + Serialization::serialLenIntList(entryTypeList);
      }


   private:
      StringList* entryIDList;
      IntList* entryTypeList;

      // for deserialization
      unsigned entryIDListBufLen;
      unsigned entryIDListElemNum;
      const char* entryIDListStart;

      unsigned entryTypeListBufLen;
      unsigned entryTypeListElemNum;
      const char* entryTypeListStart;

   public:
      void parseEntryIDList(StringList* outEntryIDList)
      {
         Serialization::deserializeStringList(entryIDListBufLen, entryIDListElemNum,
            entryIDListStart, outEntryIDList);
      }

      void parseEntryTypeList(DirEntryTypeList* outEntryTypeList)
      {
         Serialization::deserializeIntList(entryTypeListBufLen, entryTypeListElemNum,
            entryTypeListStart, (IntList*)outEntryTypeList);
      }

      virtual TestingEqualsRes testingEquals(NetMessage* msg)
      {
         RemoveInodesMsg* msgIn = (RemoveInodesMsg*) msg;

         StringList entryIDListIn;
         msgIn->parseEntryIDList(&entryIDListIn);

         DirEntryTypeList entryTypeListIn;
         msgIn->parseEntryTypeList(&entryTypeListIn);

         if (! ListTk::listsEqual(this->entryIDList, &entryIDListIn) )
            return TestingEqualsRes_FALSE;

         if (! ListTk::listsEqual((DirEntryTypeList*)this->entryTypeList, &entryTypeListIn) )
            return TestingEqualsRes_FALSE;

         return TestingEqualsRes_TRUE;
      }
};


#endif /*REMOVEINODESMSG_H*/

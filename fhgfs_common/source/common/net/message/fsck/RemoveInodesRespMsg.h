#ifndef REMOVEINODESRESPMSG_H
#define REMOVEINODESRESPMSG_H

#include <common/net/message/NetMessage.h>
#include <common/toolkit/ListTk.h>

class RemoveInodesRespMsg : public NetMessage
{
   public:
      RemoveInodesRespMsg(StringList* failedEntryIDList, DirEntryTypeList* entryTypeList) :
         NetMessage(NETMSGTYPE_RemoveInodesResp)
      {
         this->failedEntryIDList = failedEntryIDList;
         this->entryTypeList = (IntList*)entryTypeList;
      }

      RemoveInodesRespMsg() : NetMessage(NETMSGTYPE_RemoveInodesResp)
      {
      }

   protected:

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH + Serialization::serialLenStringList(failedEntryIDList)
            + Serialization::serialLenIntList(entryTypeList);
      }


   private:
      StringList* failedEntryIDList;
      IntList* entryTypeList;

      // for deserialization
      unsigned entryIDListBufLen;
      unsigned entryIDListElemNum;
      const char* entryIDListStart;

      unsigned entryTypeListBufLen;
      unsigned entryTypeListElemNum;
      const char* entryTypeListStart;

   public:
      void parseFailedEntryIDList(StringList* outFailedEntryIDList)
      {
         Serialization::deserializeStringList(entryIDListBufLen, entryIDListElemNum,
            entryIDListStart, outFailedEntryIDList);
      }

      void parseEntryTypeList(DirEntryTypeList* outEntryTypeList)
      {
         Serialization::deserializeIntList(entryTypeListBufLen, entryTypeListElemNum,
            entryTypeListStart, (IntList*)outEntryTypeList);
      }

      virtual TestingEqualsRes testingEquals(NetMessage* msg)
      {
         RemoveInodesRespMsg* msgIn = (RemoveInodesRespMsg*) msg;

         StringList entryIDListIn;
         msgIn->parseFailedEntryIDList(&entryIDListIn);

         DirEntryTypeList entryTypeListIn;
         msgIn->parseEntryTypeList(&entryTypeListIn);

         if (! ListTk::listsEqual(this->failedEntryIDList, &entryIDListIn) )
            return TestingEqualsRes_FALSE;

         if (! ListTk::listsEqual((DirEntryTypeList*)this->entryTypeList, &entryTypeListIn) )
            return TestingEqualsRes_FALSE;

         return TestingEqualsRes_TRUE;
      }
};


#endif /*REMOVEINODESRESPMSG_H*/

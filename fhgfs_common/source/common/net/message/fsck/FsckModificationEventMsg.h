#ifndef FSCKMODIFICATIONEVENTMSG_H
#define FSCKMODIFICATIONEVENTMSG_H

#include <common/net/message/AcknowledgeableMsg.h>
#include <common/toolkit/serialization/Serialization.h>
#include <common/toolkit/ListTk.h>

class FsckModificationEventMsg: public AcknowledgeableMsg
{
   public:
      FsckModificationEventMsg(UInt8List* modificationEventTypeList, StringList* entryIDList,
         bool eventsMissed = false) :
         AcknowledgeableMsg(NETMSGTYPE_FsckModificationEvent)
      {
         this->modificationEventTypeList = modificationEventTypeList;
         this->entryIDList = entryIDList;
         this->eventsMissed = eventsMissed;

         this->ackID = "";
         this->ackIDLen = 0;
      }

      // only for deserialization
      FsckModificationEventMsg() : AcknowledgeableMsg(NETMSGTYPE_FsckModificationEvent)
      {
      }

   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenUInt8List(modificationEventTypeList) + // modificationEventTypes
            Serialization::serialLenStringList(entryIDList) +
            Serialization::serialLenBool() + // eventsMissed
            Serialization::serialLenStr(ackIDLen);
      }

      /**
       * @param ackID just a reference
       */
      void setAckIDInternal(const char* ackID)
      {
         this->ackID = ackID;
         this->ackIDLen = strlen(ackID);
      }

   private:
      const char* ackID;
      unsigned ackIDLen;

      UInt8List* modificationEventTypeList;
      StringList* entryIDList;
      bool eventsMissed;

      // for deserialization
      const char* entryIDListStart;
      unsigned entryIDListBufLen;
      unsigned entryIDListElemNum;

      const char* eventTypeListStart;
      unsigned eventTypeListBufLen;
      unsigned eventTypeListElemNum;

   public:
      // getter
      const char* getAckID()
      {
         return this->ackID;
      }

      void parseModificationEventTypeList(UInt8List* outList)
      {
         Serialization::deserializeUInt8List(eventTypeListBufLen, eventTypeListElemNum,
            eventTypeListStart, outList);
      }

      void parseEntryIDList(StringList* outList)
      {
         Serialization::deserializeStringList(entryIDListBufLen, entryIDListElemNum,
            entryIDListStart, outList);
      }

      bool getEventsMissed()
      {
         return this->eventsMissed;
      }

      // inliner
      virtual TestingEqualsRes testingEquals(NetMessage* msg)
      {
         FsckModificationEventMsg* msgIn = (FsckModificationEventMsg*) msg;

         UInt8List eventsIn;
         StringList entryIDsIn;

         msgIn->parseModificationEventTypeList(&eventsIn);
         msgIn->parseEntryIDList(&entryIDsIn);

         if ( ! ListTk::listsEqual(this->modificationEventTypeList, &eventsIn) )
            return TestingEqualsRes_FALSE;

         if ( ! ListTk::listsEqual(this->entryIDList, &entryIDsIn) )
            return TestingEqualsRes_FALSE;

         if ( this->eventsMissed != msgIn->getEventsMissed() )
            return TestingEqualsRes_FALSE;

         return TestingEqualsRes_TRUE;
      }
};

#endif /*FSCKMODIFICATIONEVENTMSG_H*/

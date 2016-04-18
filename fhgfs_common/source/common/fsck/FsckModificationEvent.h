#ifndef FSCKMODIFICATIONEVENT_H_
#define FSCKMODIFICATIONEVENT_H_

#include <common/Common.h>
#include <common/toolkit/MetadataTk.h>

class FsckModificationEvent;

typedef std::list<FsckModificationEvent> FsckModificationEventList;
typedef FsckModificationEventList::iterator FsckModificationEventListIter;

class FsckModificationEvent
{
   friend class TestDatabase;

   public:
      size_t serialize(char* outBuf);
      bool deserialize(const char* buf, size_t bufLen, unsigned* outLen);
      unsigned serialLen();

   private:
      ModificationEventType eventType;
      std::string entryID;

   public:
      /*
       * @param eventType
       * @param entryID
       */
      FsckModificationEvent(ModificationEventType eventType, std::string entryID):
         eventType(eventType), entryID(entryID)
      {
      }

      // only for deserialization!
      FsckModificationEvent()
      {
      }

      // getter/setter
      ModificationEventType getEventType() const
      {
         return this->eventType;
      }

      std::string getEntryID() const
      {
         return this->entryID;
      }

      bool operator<(const FsckModificationEvent& other)
      {
         if ( eventType < other.eventType )
            return true;
         else
         if ( ( eventType == other.eventType ) && ( entryID < other.entryID ))
            return true;
         else
            return false;
      }

      bool operator==(const FsckModificationEvent& other)
      {
         if ( eventType != other.eventType )
            return false;
         else
         if ( entryID.compare(other.entryID) )
            return false;
         else
            return true;
      }

      bool operator!= (const FsckModificationEvent& other)
      {
         return !(operator==( other ) );
      }

      void update(ModificationEventType eventType, std::string entryID)
      {
         this->eventType = eventType;
         this->entryID = entryID;
      }
};

#endif /* FSCKMODIFICATIONEVENT_H_ */

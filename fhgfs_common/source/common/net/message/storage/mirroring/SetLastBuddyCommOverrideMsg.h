#ifndef SETLASTBUDDYCOMMOVERRIDEMSG_H_
#define SETLASTBUDDYCOMMOVERRIDEMSG_H_

#include <common/net/message/NetMessage.h>

class SetLastBuddyCommOverrideMsg : public NetMessage
{
   public:

      /**
       * @param targetID
       * @param timestamp
       * @param abortResync if a resync is already running for that target abort it, so that it
       * restarts with the new timestamp file
       */
      SetLastBuddyCommOverrideMsg(uint16_t targetID, int64_t timestamp, bool abortResync) :
         NetMessage(NETMSGTYPE_SetLastBuddyCommOverride), targetID(targetID), timestamp(timestamp),
         abortResync(abortResync)
      {
         // all initialization done in initializer list
      }

   protected:
      /**
       * For deserialization only!
       */
      SetLastBuddyCommOverrideMsg() : NetMessage(NETMSGTYPE_SetLastBuddyCommOverride) {}

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         unsigned retVal = NETMSG_HEADER_LENGTH +
            Serialization::serialLenUInt16() + // targetID
            Serialization::serialLenInt64() +  // timestamp
            Serialization::serialLenBool();    // abortResync

         return retVal;
      }

   private:
      uint16_t targetID;
      int64_t timestamp;
      bool abortResync;

   public:
      // getters & setters
      uint16_t getTargetID() const
      {
         return targetID;
      }

      int64_t getTimestamp() const
      {
         return timestamp;
      }

      bool getAbortResync() const
      {
         return abortResync;
      }

};

#endif /*SETLASTBUDDYCOMMOVERRIDEMSG_H_*/

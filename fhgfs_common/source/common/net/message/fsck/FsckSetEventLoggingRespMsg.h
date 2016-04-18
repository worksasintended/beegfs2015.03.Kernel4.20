#ifndef FSCKSETEVENTLOGGINGRESPMSG_H
#define FSCKSETEVENTLOGGINGRESPMSG_H

#include <common/net/message/NetMessage.h>
#include <common/toolkit/ListTk.h>
#include <common/toolkit/serialization/Serialization.h>

class FsckSetEventLoggingRespMsg: public NetMessage
{
   public:
      FsckSetEventLoggingRespMsg(bool result, bool missedEvents) :
         NetMessage(NETMSGTYPE_FsckSetEventLoggingResp)
      {
         this->result = result;
         this->missedEvents = missedEvents;
      }

      // only for deserialization
      FsckSetEventLoggingRespMsg() : NetMessage(NETMSGTYPE_FsckSetEventLoggingResp)
      {
      }

   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenBool() + // result
            Serialization::serialLenBool(); // missedEvents
      }

   private:
      bool result;
      bool missedEvents;

   public:
      // getter
      bool getResult() const
      {
         return this->result;
      }

      unsigned getMissedEvents() const
      {
         return this->missedEvents;
      }

      // inliner
      virtual TestingEqualsRes testingEquals(NetMessage* msg)
      {
         FsckSetEventLoggingRespMsg* msgIn = (FsckSetEventLoggingRespMsg*) msg;

         if ( this->result != msgIn->getResult())
            return TestingEqualsRes_FALSE;

         if ( this->missedEvents != msgIn->getMissedEvents())
            return TestingEqualsRes_FALSE;

         return TestingEqualsRes_TRUE;
      }
};

#endif /*FSCKSETEVENTLOGGINGRESPMSG_H*/

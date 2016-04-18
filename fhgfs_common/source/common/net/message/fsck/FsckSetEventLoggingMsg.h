#ifndef FSCKSETEVENTLOGGINGMSG_H
#define FSCKSETEVENTLOGGINGMSG_H

#include <common/net/message/NetMessage.h>
#include <common/toolkit/ListTk.h>
#include <common/toolkit/serialization/Serialization.h>

class FsckSetEventLoggingMsg: public NetMessage
{
   public:
      FsckSetEventLoggingMsg(bool enableLogging, unsigned portUDP, NicAddressList* nicList) :
         NetMessage(NETMSGTYPE_FsckSetEventLogging)
      {
         this->enableLogging = enableLogging;
         this->portUDP = portUDP;
         this->nicList = nicList;
      }

      // only for deserialization
      FsckSetEventLoggingMsg() : NetMessage(NETMSGTYPE_FsckSetEventLogging)
      {
      }

   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenBool() + // enableLogging
            Serialization::serialLenUInt() + // portUDP
            Serialization::serialLenNicList(this->nicList); // nicList
      }

   private:
      bool enableLogging;
      unsigned portUDP;
      NicAddressList* nicList;

      // for (de)-serialization
      unsigned nicListElemNum;
      const char* nicListStart;

   public:
      // getter
      bool getEnableLogging() const
      {
         return this->enableLogging;
      }

      unsigned getPortUDP() const
      {
         return this->portUDP;
      }

      void parseNicList(NicAddressList *outNicList)
      {
         Serialization::deserializeNicList(nicListElemNum, nicListStart, outNicList);
      }

      // inliner
      virtual TestingEqualsRes testingEquals(NetMessage* msg)
      {
         FsckSetEventLoggingMsg* msgIn = (FsckSetEventLoggingMsg*) msg;

         if ( this->enableLogging != msgIn->getEnableLogging())
            return TestingEqualsRes_FALSE;

         if ( this->portUDP != msgIn->getPortUDP())
            return TestingEqualsRes_FALSE;

         return TestingEqualsRes_TRUE;
      }
};

#endif /*FSCKSETEVENTLOGGINGMSG_H*/

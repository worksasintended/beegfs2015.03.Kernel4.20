#ifndef FSCKSETEVENTLOGGINGMSG_H
#define FSCKSETEVENTLOGGINGMSG_H

#include <common/net/message/NetMessage.h>
#include <common/toolkit/ListTk.h>
#include <common/toolkit/serialization/Serialization.h>

#define FSCKSETEVENTLOGGINGMSG_COMPAT_FLAG_NO_FORCE_RESTART 1 // = !forceRestart in version 6

class FsckSetEventLoggingMsg: public NetMessage
{
   public:
      FsckSetEventLoggingMsg(bool enableLogging, unsigned portUDP, NicAddressList* nicList,
            bool forceRestart = true) :
         NetMessage(NETMSGTYPE_FsckSetEventLogging)
      {
         this->enableLogging = enableLogging;
         this->portUDP = portUDP;
         this->nicList = nicList;

         if (!forceRestart)
            addMsgHeaderCompatFeatureFlag(FSCKSETEVENTLOGGINGMSG_COMPAT_FLAG_NO_FORCE_RESTART);
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

      bool getForceRestart()
      {
         return
            !isMsgHeaderCompatFeatureFlagSet(FSCKSETEVENTLOGGINGMSG_COMPAT_FLAG_NO_FORCE_RESTART);
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

#include "TestMsgSerializationBase.h"

#include <common/net/message/nodes/HeartbeatRequestMsg.h>
#include <common/net/message/nodes/HeartbeatMsg.h>
#include <common/net/message/AbstractNetMessageFactory.h>
#include <common/toolkit/MessagingTk.h>

TestMsgSerializationBase::TestMsgSerializationBase()
{
}

TestMsgSerializationBase::~TestMsgSerializationBase()
{
}

void TestMsgSerializationBase::setUp()
{
}

void TestMsgSerializationBase::tearDown()
{
}

bool TestMsgSerializationBase::testMsgSerialization(NetMessage &msg, NetMessage &msgClone)
{
   const char* logContext = "testMsgSerialization";

   bool retVal = true;

   char* buf = NULL;
   size_t bufLen = msg.getMsgLength();
   char* msgPayloadBuf = NULL;
   size_t msgPayloadBufLen = 0;

   TestingEqualsRes testRes = TestingEqualsRes_UNDEFINED;
   bool checkCompatRes = false;
   bool deserRes = false;

   // create buffer and serialize message into it
   buf = MessagingTk::createMsgBuf(&msg);

   // get serialized msg header from buf and check if msgType matches
   NetMessageHeader headerOut;
   NetMessage::deserializeHeader(buf, bufLen, &headerOut);

   if (headerOut.msgType != msg.getMsgType())
   {
      retVal = false;
      goto cleanup_and_exit;
   }

   // apply message feature flags and check compatibility

   msgClone.setMsgHeaderFeatureFlags(headerOut.msgFeatureFlags);

   checkCompatRes = msgClone.checkHeaderFeatureFlagsCompat();

   if(!checkCompatRes)
   {
      retVal = false;
      goto cleanup_and_exit;
   }

   // compare header compat feature flags

   if(headerOut.msgCompatFeatureFlags != msg.getMsgHeaderCompatFeatureFlags() )
   {
      retVal = false;
      goto cleanup_and_exit;
   }

   // deserialize msg from buf
   msgPayloadBuf = buf + NETMSG_HEADER_LENGTH;
   msgPayloadBufLen = bufLen - NETMSG_HEADER_LENGTH;

   deserRes = msgClone.deserializePayload(msgPayloadBuf, msgPayloadBufLen);

   if (!deserRes)
   {
      retVal = false;
      goto cleanup_and_exit;
   }

   testRes = msg.testingEquals(&msgClone);

   if (testRes == TestingEqualsRes_UNDEFINED)
   {
      LogContext(logContext).log(Log_WARNING, "TestingEqual is UNDEFINED; MsgType: " +
         StringTk::intToStr(msg.getMsgType() ) );
   }
   else
   if (testRes == TestingEqualsRes_FALSE)
      retVal = false;

   cleanup_and_exit:
   SAFE_FREE(buf);

   return retVal;
}

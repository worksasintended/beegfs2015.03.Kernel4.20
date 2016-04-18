#ifndef TESTMSGSERIALIZATION_H_
#define TESTMSGSERIALIZATION_H_

#include <common/app/log/LogContext.h>
#include <common/net/message/NetMessage.h>
#include <common/testing/TestMsgSerializationBase.h>
#include <cppunit/TestAssert.h>
#include <cppunit/extensions/HelperMacros.h>

/*
 * This class is intended to hold all tests to check the serialization/deserialization of messages,
 * which are sent in this project. Please note, that each message tested here must implement
 * the method "testingEquals".
 */
class TestMsgSerialization: public TestMsgSerializationBase
{
   CPPUNIT_TEST_SUITE( TestMsgSerialization );
   CPPUNIT_TEST( testRetrieveDirEntriesMsgSerialization );
   CPPUNIT_TEST( testRetrieveDirEntriesRespMsgSerialization );
   CPPUNIT_TEST( testRetrieveFsIDsMsgSerialization );
   CPPUNIT_TEST( testRetrieveFsIDsRespMsgSerialization );
   CPPUNIT_TEST( testRetrieveInodesMsgSerialization );
   CPPUNIT_TEST( testRetrieveInodesRespMsgSerialization );
   CPPUNIT_TEST( testFetchFsckChunkListMsgSerialization );
   CPPUNIT_TEST( testFetchFsckChunkListRespMsgSerialization );
   CPPUNIT_TEST( testChangeStripeTargetMsgSerialization );
   CPPUNIT_TEST( testChangeStripeTargetRespMsgSerialization );
   CPPUNIT_TEST( testFsckSetEventLoggingMsg );
   CPPUNIT_TEST( testFsckSetEventLoggingRespMsg );
   CPPUNIT_TEST( testAdjustChunkPermissionsMsgSerialization );
   CPPUNIT_TEST( testAdjustChunkPermissionsRespMsgSerialization );
   CPPUNIT_TEST( testMoveChunkFileMsg );
   CPPUNIT_TEST( testMoveChunkFileRespMsg );
   CPPUNIT_TEST( testRemoveInodesMsg );
   CPPUNIT_TEST( testRemoveInodesRespMsg );
   CPPUNIT_TEST_SUITE_END();

   public:
      TestMsgSerialization();
      virtual ~TestMsgSerialization();

      void setUp();
      void tearDown();

      void testRetrieveDirEntriesMsgSerialization();
      void testRetrieveDirEntriesRespMsgSerialization();
      void testRetrieveFsIDsMsgSerialization();
      void testRetrieveFsIDsRespMsgSerialization();
      void testRetrieveInodesMsgSerialization();
      void testRetrieveInodesRespMsgSerialization();
      void testFetchFsckChunkListMsgSerialization();
      void testFetchFsckChunkListRespMsgSerialization();

      void testChangeStripeTargetMsgSerialization();
      void testChangeStripeTargetRespMsgSerialization();

      void testFsckSetEventLoggingMsg();
      void testFsckSetEventLoggingRespMsg();

      void testAdjustChunkPermissionsMsgSerialization();
      void testAdjustChunkPermissionsRespMsgSerialization();

      void testMoveChunkFileMsg();
      void testMoveChunkFileRespMsg();

      void testRemoveInodesMsg();
      void testRemoveInodesRespMsg();
   private:
      LogContext log;
};

#endif /* TESTMSGSERIALIZATION_H_ */

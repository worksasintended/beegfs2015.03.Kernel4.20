#ifndef TESTSERIALIZATION_H_
#define TESTSERIALIZATION_H_

#include <common/app/log/LogContext.h>
#include <common/net/message/NetMessage.h>
#include <common/testing/TestMsgSerializationBase.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <session/SessionStore.h>

/*
 * intended to test all kind of serialization/deserialization of objects (not for messages, only
 * for the single objects itself
 */
class TestSerialization: public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE( TestSerialization );
   CPPUNIT_TEST( testEntryInfoListSerialization );
   CPPUNIT_TEST( testStripePatternSerialization );
   CPPUNIT_TEST( testSessionSerialization );
   CPPUNIT_TEST_SUITE_END();

   public:
      TestSerialization();
      virtual ~TestSerialization();

      void setUp();
      void tearDown();

      void testEntryInfoListSerialization();
      void testStripePatternSerialization();
      void testSessionSerialization();


   private:
      LogContext log;

      void initSessionStoreForTests(SessionStore& testSessionStore);
      static void addRandomValuesToUInt16Vector(UInt16Vector* vector, size_t count);
      static void addRandomTargetChunkBlocksToStatData(StatData* statData, size_t count);
};

#endif /* TESTSERIALIZATION_H_ */

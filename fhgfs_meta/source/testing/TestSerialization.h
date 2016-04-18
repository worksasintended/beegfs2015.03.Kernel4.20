#ifndef TESTSERIALIZATION_H_
#define TESTSERIALIZATION_H_

#include <common/net/message/NetMessage.h>
#include <common/testing/TestMsgSerializationBase.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <common/app/log/LogContext.h>

/*
 * intended to test all kind of serialization/deserialization of objects (not for messages, only
 * for the single objects itself
 */
class TestSerialization: public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE( TestSerialization );
   CPPUNIT_TEST( testEntryInfoListSerialization );
   CPPUNIT_TEST( testStripePatternSerialization );
   CPPUNIT_TEST_SUITE_END();

   public:
      TestSerialization();
      virtual ~TestSerialization();

      void setUp();
      void tearDown();

      void testEntryInfoListSerialization();
      void testStripePatternSerialization();
   private:
      LogContext log;
};

#endif /* TESTSERIALIZATION_H_ */

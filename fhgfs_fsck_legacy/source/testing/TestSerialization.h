#ifndef TESTSERIALIZATION_H_
#define TESTSERIALIZATION_H_

#include <common/net/message/NetMessage.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class TestSerialization: public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE( TestSerialization );

   CPPUNIT_TEST(testFsckDirEntrySerialization);
   CPPUNIT_TEST(testFsckDirEntryListSerialization);

   CPPUNIT_TEST(testFsckDirInodeSerialization);
   CPPUNIT_TEST(testFsckDirInodeListSerialization);

   CPPUNIT_TEST(testFsckFileInodeSerialization);
   CPPUNIT_TEST(testFsckFileInodeListSerialization);

   CPPUNIT_TEST(testFsckChunkSerialization);
   CPPUNIT_TEST(testFsckChunkListSerialization);

   CPPUNIT_TEST(testFsckContDirSerialization);
   CPPUNIT_TEST(testFsckContDirListSerialization);

   CPPUNIT_TEST(testFsckFsIDSerialization);
   CPPUNIT_TEST(testFsckFsIDListSerialization);

   CPPUNIT_TEST(testUInt16Serialization);
   CPPUNIT_TEST(testUInt16ListSerialization);
   CPPUNIT_TEST(testUInt16VectorSerialization);
   CPPUNIT_TEST_SUITE_END();

   public:
      TestSerialization();
      virtual ~TestSerialization();

      void setUp();
      void tearDown();

   private:
      void testFsckDirEntrySerialization();
      void testFsckDirEntryListSerialization();

      void testFsckDirInodeSerialization();
      void testFsckDirInodeListSerialization();

      void testFsckFileInodeSerialization();
      void testFsckFileInodeListSerialization();

      void testFsckChunkSerialization();
      void testFsckChunkListSerialization();

      void testFsckContDirSerialization();
      void testFsckContDirListSerialization();

      void testFsckFsIDSerialization();
      void testFsckFsIDListSerialization();

      void testUInt16Serialization();
      void testUInt16ListSerialization();
      void testUInt16VectorSerialization();
};

#endif /* TESTSERIALIZATION_H_ */

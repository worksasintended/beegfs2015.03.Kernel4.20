#ifndef TESTFSCKTK_H_
#define TESTFSCKTK_H_

#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class TestFsckTk: public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE( TestFsckTk );

   CPPUNIT_TEST( testDirEntryTypeConversion );

   CPPUNIT_TEST( testRemoveFromListDirEntries );
   CPPUNIT_TEST( testRemoveFromListDirInodes );
   CPPUNIT_TEST( testRemoveFromListDirInodesByID );
   CPPUNIT_TEST( testRemoveFromListFileInodes );
   CPPUNIT_TEST( testRemoveFromListFileInodesByID );
   CPPUNIT_TEST( testRemoveFromListChunks );
   CPPUNIT_TEST( testRemoveFromListChunksByID );
   CPPUNIT_TEST( testRemoveFromListStrings );

   CPPUNIT_TEST_SUITE_END();

   public:
      TestFsckTk();
      virtual ~TestFsckTk();

      void setUp();
      void tearDown();

      void testDirEntryTypeConversion();
      void testRemoveFromListDirEntries();
      void testRemoveFromListDirInodes();
      void testRemoveFromListDirInodesByID();
      void testRemoveFromListFileInodes();
      void testRemoveFromListFileInodesByID();
      void testRemoveFromListChunks();
      void testRemoveFromListChunksByID();
      void testRemoveFromListStrings();
};

#endif /* TESTFSCKTK_H_ */

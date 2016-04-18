#ifndef TESTDATABASE_H_
#define TESTDATABASE_H_

#include <common/net/message/NetMessage.h>
#include <database/FsckDB.h>

#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class TestDatabase: public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE( TestDatabase );

   CPPUNIT_TEST(testCreateTables);
   CPPUNIT_TEST(testCreateTablesOnExistingDB);

   CPPUNIT_TEST(testInsertAndReadSingleDentry);
   CPPUNIT_TEST(testInsertAndReadDentries);
   CPPUNIT_TEST(testInsertDoubleDentries);

   CPPUNIT_TEST(testUpdateDentries);
   CPPUNIT_TEST(testUpdateDirInodes);
   CPPUNIT_TEST(testUpdateFileInodes);
   CPPUNIT_TEST(testUpdateChunks);

   CPPUNIT_TEST(testInsertAndReadSingleFileInode);
   CPPUNIT_TEST(testInsertAndReadFileInodes);
   CPPUNIT_TEST(testInsertDoubleFileInodes);

   CPPUNIT_TEST(testInsertAndReadSingleDirInode);
   CPPUNIT_TEST(testInsertAndReadDirInodes);
   CPPUNIT_TEST(testInsertDoubleDirInodes);

   CPPUNIT_TEST(testInsertAndReadSingleChunk);
   CPPUNIT_TEST(testInsertAndReadChunks);
   CPPUNIT_TEST(testInsertDoubleChunks);

   CPPUNIT_TEST(testInsertAndReadSingleContDir);
   CPPUNIT_TEST(testInsertAndReadContDirs);
   CPPUNIT_TEST(testInsertDoubleContDirs);

   CPPUNIT_TEST(testInsertAndReadSingleFsID);
   CPPUNIT_TEST(testInsertAndReadFsIDs);
   CPPUNIT_TEST(testInsertDoubleFsIDs);

   CPPUNIT_TEST(testCheckForAndInsertDanglingDirEntries);
   CPPUNIT_TEST(testCheckForAndInsertInodesWithWrongOwner);
   CPPUNIT_TEST(testCheckForAndInsertDirEntriesWithWrongOwner);
   CPPUNIT_TEST(testCheckForAndInsertMissingDentryByIDFile);
   CPPUNIT_TEST(testCheckForAndInsertOrphanedDentryByIDFiles);
   CPPUNIT_TEST(testCheckForAndInsertOrphanedDirInodes);
   CPPUNIT_TEST(testCheckForAndInsertOrphanedFileInodes);
   CPPUNIT_TEST(testCheckForAndInsertOrphanedChunks);
   CPPUNIT_TEST(testCheckForAndInsertInodesWithoutContDir);
   CPPUNIT_TEST(testCheckForAndInsertOrphanedContDirs);
   CPPUNIT_TEST(testCheckForAndInsertFileInodesWithWrongAttribs);
   CPPUNIT_TEST(testCheckForAndInsertDirInodesWithWrongAttribs);
   CPPUNIT_TEST(testCheckForAndInsertMissingStripeTargets);
   CPPUNIT_TEST(testCheckForAndInsertFilesWithMissingStripeTargets);
   CPPUNIT_TEST(testCheckForAndInsertChunksWithWrongPermissions);
   CPPUNIT_TEST(testCheckForAndInsertChunksInWrongPath);

   CPPUNIT_TEST(testUpdateRepairActionDentries);
   CPPUNIT_TEST(testUpdateRepairActionFileInodes);
   CPPUNIT_TEST(testUpdateRepairActionDirInodes);
   CPPUNIT_TEST(testUpdateRepairActionChunks);
   CPPUNIT_TEST(testUpdateRepairActionContDirs);
   CPPUNIT_TEST(testUpdateRepairActionFsIDs);

   CPPUNIT_TEST(testDeleteDentries);
   CPPUNIT_TEST(testDeleteChunks);
   CPPUNIT_TEST(testDeleteFsIDs);

   CPPUNIT_TEST(testDeleteDanglingDentries);
   CPPUNIT_TEST(testDeleteDentriesWithWrongOwner);
   CPPUNIT_TEST(testDeleteInodesWithWrongOwner);
   CPPUNIT_TEST(testDeleteOrphanedDirInodes);
   CPPUNIT_TEST(testDeleteOrphanedFileInodes);
   CPPUNIT_TEST(testDeleteOrphanedChunks);
   CPPUNIT_TEST(testDeleteMissingContDir);
   CPPUNIT_TEST(testDeleteOrphanedContDir);
   CPPUNIT_TEST(testDeleteFileInodesWithWrongAttribs);
   CPPUNIT_TEST(testDeleteDirInodesWithWrongAttribs);
   CPPUNIT_TEST(testDeleteMissingStripeTargets);
   CPPUNIT_TEST(testDeleteFilesWithMissingStripeTargets);
   CPPUNIT_TEST(testDeleteChunksWithWrongPermissions);

   CPPUNIT_TEST(testReplaceStripeTarget);

   CPPUNIT_TEST(testBasicDBCursor);

   CPPUNIT_TEST(testGetRowCount);

   CPPUNIT_TEST(testGetFullPath);

   CPPUNIT_TEST_SUITE_END();

   public:
      TestDatabase();
      virtual ~TestDatabase();

      void setUp();
      void tearDown();

   private:
      std::string databaseFile;
      FsckDB *db;
      LogContext log;

      void testCreateTables();
      void testCreateTablesOnExistingDB();

      void testInsertAndReadSingleDentry();
      void testInsertAndReadDentries();
      void testInsertDoubleDentries();

      void testUpdateDentries();
      void testUpdateDirInodes();
      void testUpdateFileInodes();
      void testUpdateChunks();

      void testInsertAndReadSingleFileInode();
      void testInsertAndReadFileInodes();
      void testInsertDoubleFileInodes();

      void testInsertAndReadSingleDirInode();
      void testInsertAndReadDirInodes();
      void testInsertDoubleDirInodes();

      void testInsertAndReadSingleChunk();
      void testInsertAndReadChunks();
      void testInsertDoubleChunks();

      void testInsertAndReadSingleContDir();
      void testInsertAndReadContDirs();
      void testInsertDoubleContDirs();

      void testInsertAndReadSingleFsID();
      void testInsertAndReadFsIDs();
      void testInsertDoubleFsIDs();

      void testCheckForAndInsertDanglingDirEntries();
      void testCheckForAndInsertInodesWithWrongOwner();
      void testCheckForAndInsertDirEntriesWithWrongOwner();
      void testCheckForAndInsertMissingDentryByIDFile();
      void testCheckForAndInsertOrphanedDentryByIDFiles();
      void testCheckForAndInsertOrphanedDirInodes();
      void testCheckForAndInsertOrphanedFileInodes();
      void testCheckForAndInsertOrphanedChunks();
      void testCheckForAndInsertInodesWithoutContDir();
      void testCheckForAndInsertOrphanedContDirs();
      void testCheckForAndInsertFileInodesWithWrongAttribs();
      void testCheckForAndInsertDirInodesWithWrongAttribs();
      void testCheckForAndInsertMissingStripeTargets();
      void testCheckForAndInsertFilesWithMissingStripeTargets();
      void testCheckForAndInsertChunksWithWrongPermissions();
      void testCheckForAndInsertChunksInWrongPath();

      void testUpdateRepairActionDentries();
      void testUpdateRepairActionFileInodes();
      void testUpdateRepairActionDirInodes();
      void testUpdateRepairActionChunks();
      void testUpdateRepairActionContDirs();
      void testUpdateRepairActionFsIDs();
      void testUpdateRepairActionTargetIDs();

      void testDeleteDentries();
      void testDeleteChunks();
      void testDeleteFsIDs();

      void testDeleteDanglingDentries();
      void testDeleteDentriesWithWrongOwner();
      void testDeleteInodesWithWrongOwner();
      void testDeleteOrphanedDirInodes();
      void testDeleteOrphanedFileInodes();
      void testDeleteOrphanedChunks();
      void testDeleteMissingContDir();
      void testDeleteOrphanedContDir();
      void testDeleteFileInodesWithWrongAttribs();
      void testDeleteDirInodesWithWrongAttribs();
      void testDeleteMissingStripeTargets();
      void testDeleteFilesWithMissingStripeTargets();
      void testDeleteChunksWithWrongPermissions();

      void testReplaceStripeTarget();

      void testBasicDBCursor();

      void testGetRowCount();

      void testGetFullPath();
};

#endif /* TESTDATABASE_H_ */

#include "TestDatabase.h"

#include <common/net/message/nodes/HeartbeatRequestMsg.h>
#include <common/net/message/nodes/HeartbeatMsg.h>
#include <common/storage/striping/Raid0Pattern.h>
#include <common/toolkit/ListTk.h>
#include <database/FsckDBException.h>
#include <net/message/NetMessageFactory.h>
#include <program/Program.h>
#include <toolkit/DatabaseTk.h>
#include <toolkit/FsckTkEx.h>

TestDatabase::TestDatabase()
{
   log.setContext("TestDatabase");
   this->databasePath = Program::getApp()->getConfig()->getTestDatabasePath();
}

TestDatabase::~TestDatabase()
{
}

void TestDatabase::setUp()
{
   // create a new database
   try
   {
      Path dbPath(databasePath);
      bool createRes = StorageTk::createPathOnDisk(dbPath, false);
      if (!createRes)
         throw FsckDBException("Unable to create path to database");

      this->db = new FsckDB(databasePath);
      this->db->init();
   }
   catch (FsckDBException &e)
   {
      std::cerr << "Setup of TestDatabase failed" << std::endl;
      std::cerr << "Exception Msg: " << e.what() << std::endl;
   }
}

void TestDatabase::tearDown()
{
   if (this->db)
   {
      delete(this->db);
   }

   bool keepFile = Program::getApp()->getConfig()->getKeepTestDatabaseFile();

   if (!keepFile)
      DatabaseTk::removeDatabaseFiles(databasePath);
}

void TestDatabase::testCreateTables()
{
   try
   {
      this->db->createTables();
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }
}

/*
 * try to create the tables twice (second time should be ignored)
 */
void TestDatabase::testCreateTablesOnExistingDB()
{
   try
   {
      this->db->createTables();
      this->db->createTables();
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }
}

void TestDatabase::testInsertAndReadSingleDentry()
{
   FsckDirEntryList dentryListIn;
   FsckDirEntryList dentryListOut;

   FsckDirEntry dentry = DatabaseTk::createDummyFsckDirEntry();

   dentryListIn.push_back(dentry);

   try
   {
      // insert the dentry
      FsckDirEntry failedInsert;
      int errorCode;
      if (! this->db->insertDirEntries(dentryListIn, failedInsert, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // read into the outList
      this->db->getDirEntries(&dentryListOut);

      bool compareRes = FsckTkEx::compareFsckDirEntryLists(&dentryListIn, &dentryListOut);
      CPPUNIT_ASSERT_MESSAGE("Comparision of dirEntries failed", compareRes);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }
}

void TestDatabase::testInsertDoubleDentries()
{
   unsigned NUM_ROWS = 5;

   // generate NUM_ROWS rows
   FsckDirEntryList dentryListOne;
   DatabaseTk::createDummyFsckDirEntries(NUM_ROWS, &dentryListOne);

   // duplicate first dentry and put it into second list
   FsckDirEntry duplicate = dentryListOne.front();
   FsckDirEntryList dentryListTwo;
   dentryListTwo.push_back(duplicate);

   try
   {
      FsckDirEntry failedInsert;
      int errorCode;

      // insert the first dentry list
      if (! this->db->insertDirEntries(dentryListOne, failedInsert, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // now insert the second list; there we expect to have a conflict with constraint
      // violation error code

      bool success = this->db->insertDirEntries(dentryListTwo, failedInsert, errorCode);

      CPPUNIT_ASSERT ( success == false );
      CPPUNIT_ASSERT ( failedInsert == duplicate );
      CPPUNIT_ASSERT ( errorCode == SQLITE_CONSTRAINT );
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }
}

void TestDatabase::testInsertAndReadDentries()
{
   unsigned NUM_ROWS = 5;

   FsckDirEntryList dentryListIn;
   FsckDirEntryList dentryListOut;

   DatabaseTk::createDummyFsckDirEntries(NUM_ROWS, &dentryListIn);

   try
   {
      // insert the dentries
      FsckDirEntry failedInsert;
      int errorCode;
      if (! this->db->insertDirEntries(dentryListIn, failedInsert, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // read into the outList
      this->db->getDirEntries(&dentryListOut);

      bool compareRes = FsckTkEx::compareFsckDirEntryLists(&dentryListIn, &dentryListOut);
      CPPUNIT_ASSERT_MESSAGE("Comparision of dirEntries failed", compareRes);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }
}

void TestDatabase::testUpdateDentries()
{
   log.log(Log_DEBUG, "-testUpdateDentries - started");

   unsigned NUM_ROWS = 5;

   FsckDirEntryList dentryListIn;
   FsckDirEntryList dentryListOut;

   DatabaseTk::createDummyFsckDirEntries(NUM_ROWS, &dentryListIn);

   try
   {
      // insert the dentries
      FsckDirEntry failedInsert;
      int errorCode;
      if (! this->db->insertDirEntries(dentryListIn, failedInsert, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // modify the dentries
      for (FsckDirEntryListIter iter = dentryListIn.begin(); iter != dentryListIn.end(); iter++)
      {
         iter->setInodeOwnerNodeID(iter->getInodeOwnerNodeID() + 1000);
      }

      // update the dentries
      if (! this->db->updateDirEntries(dentryListIn, failedInsert, errorCode) )
         CPPUNIT_FAIL("Update returned error");

      // read into the outList
      this->db->getDirEntries(&dentryListOut);

      bool compareRes = FsckTkEx::compareFsckDirEntryLists(&dentryListIn, &dentryListOut);

      CPPUNIT_ASSERT_MESSAGE("Comparision of dirEntries failed", compareRes);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }

   log.log(Log_DEBUG, "-testUpdateDentries - success");
}

void TestDatabase::testUpdateDirInodes()
{
   log.log(Log_DEBUG, "-testUpdateDirInodes - started");

   unsigned NUM_ROWS = 5;

   FsckDirInodeList inodeListIn;
   FsckDirInodeList inodeListOut;

   DatabaseTk::createDummyFsckDirInodes(NUM_ROWS, &inodeListIn);

   try
   {
      // insert the inodes
      FsckDirInode failedInsert;
      int errorCode;
      if (! this->db->insertDirInodes(inodeListIn, failedInsert, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // modify the inodes
      for (FsckDirInodeListIter iter = inodeListIn.begin(); iter != inodeListIn.end(); iter++)
      {
         iter->setParentNodeID(iter->getParentNodeID() + 1000);
      }

      // update the inodes
      if (! this->db->updateDirInodes(inodeListIn, failedInsert, errorCode) )
         CPPUNIT_FAIL("Update returned error");

      // read into the outList
      this->db->getDirInodes(&inodeListOut);

      bool compareRes = FsckTkEx::compareFsckDirInodeLists(&inodeListIn, &inodeListOut);
      CPPUNIT_ASSERT_MESSAGE("Comparision of dir inodes failed", compareRes);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }

   log.log(Log_DEBUG, "-testUpdateDirInodes - success");
}

void TestDatabase::testUpdateFileInodes()
{
   log.log(Log_DEBUG, "-testUpdateFileInodes - started");

   unsigned NUM_ROWS = 5;

   FsckFileInodeList inodeListIn;
   FsckFileInodeList inodeListOut;

   DatabaseTk::createDummyFsckFileInodes(NUM_ROWS, &inodeListIn);

   try
   {
      // insert the inodes
      FsckFileInode failedInsert;
      int errorCode;
      if (! this->db->insertFileInodes(inodeListIn, failedInsert, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // modify the inodes
      for (FsckFileInodeListIter iter = inodeListIn.begin(); iter != inodeListIn.end(); iter++)
      {
         iter->setParentNodeID(iter->getParentNodeID() + 1000);
      }

      // update the inodes
      if (! this->db->updateFileInodes(inodeListIn, failedInsert, errorCode) )
         CPPUNIT_FAIL("Update returned error");

      // read into the outList
      this->db->getFileInodes(&inodeListOut);

      bool compareRes = FsckTkEx::compareFsckFileInodeLists(&inodeListIn, &inodeListOut);
      CPPUNIT_ASSERT_MESSAGE("Comparision of file inodes failed", compareRes);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }

   log.log(Log_DEBUG, "-testUpdateFileInodes - success");
}

void TestDatabase::testUpdateChunks()
{
   log.log(Log_DEBUG, "-testUpdateChunks - started");

   unsigned NUM_ROWS = 5;

   FsckChunkList chunkListIn;
   FsckChunkList chunkListOut;

   DatabaseTk::createDummyFsckChunks(NUM_ROWS, &chunkListIn);

   try
   {
      // insert the chunks
      FsckChunk failedInsert;
      int errorCode;
      if (! this->db->insertChunks(chunkListIn, failedInsert, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // modify the chunkListIn
      for (FsckChunkListIter iter = chunkListIn.begin(); iter != chunkListIn.end(); iter++)
      {
         iter->setUserID(iter->getUserID() + 1000);
      }

      // update the chunkListIn
      if (! this->db->updateChunks(chunkListIn, failedInsert, errorCode))
         CPPUNIT_FAIL("Update returned error");

      // read into the outList
      this->db->getChunks(&chunkListOut);

      bool compareRes = FsckTkEx::compareFsckChunkLists(&chunkListIn, &chunkListOut);
      CPPUNIT_ASSERT_MESSAGE("Comparision of chunks failed", compareRes);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }

   log.log(Log_DEBUG, "-testUpdateChunks - success");
}

void TestDatabase::testInsertAndReadSingleFileInode()
{
   FsckFileInodeList fileInodeListIn;
   FsckFileInodeList fileInodeListOut;

   FsckFileInode fileInode = DatabaseTk::createDummyFsckFileInode();

   fileInodeListIn.push_back(fileInode);

   try
   {
      // insert the inodes
      FsckFileInode failedInsert;
      int errorCode;
      if (! this->db->insertFileInodes(fileInodeListIn, failedInsert, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // read into the outList
      this->db->getFileInodes(&fileInodeListOut);

      bool compareRes = FsckTkEx::compareFsckFileInodeLists(&fileInodeListIn, &fileInodeListOut);
      CPPUNIT_ASSERT_MESSAGE("Comparision of file inodes failed", compareRes);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }
}

void TestDatabase::testInsertAndReadFileInodes()
{
   unsigned NUM_ROWS = 5;

   FsckFileInodeList fileInodeListIn;
   FsckFileInodeList fileInodeListOut;

   DatabaseTk::createDummyFsckFileInodes(NUM_ROWS, &fileInodeListIn);

   try
   {
      // insert the inodes
      FsckFileInode failedInsert;
      int errorCode;
      if (! this->db->insertFileInodes(fileInodeListIn, failedInsert, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // read into the outList
      this->db->getFileInodes(&fileInodeListOut);

      bool compareRes = FsckTkEx::compareFsckFileInodeLists(&fileInodeListIn, &fileInodeListOut);
      CPPUNIT_ASSERT_MESSAGE("Comparision of file inodes failed", compareRes);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }
}

void TestDatabase::testInsertDoubleFileInodes()
{
   unsigned NUM_ROWS = 5;

   // generate NUM_ROWS rows
   FsckFileInodeList listOne;
   DatabaseTk::createDummyFsckFileInodes(NUM_ROWS, &listOne);

   // duplicate first item and put it into second list
   FsckFileInode duplicate = listOne.front();
   FsckFileInodeList listTwo;
   listTwo.push_back(duplicate);

   try
   {
      // insert the first list
      FsckFileInode failedInsert;
      int errorCode;

      if (! this->db->insertFileInodes(listOne, failedInsert, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // now insert the second list; there we expect to have a conflict with constraint
      // violation error code
      bool success = this->db->insertFileInodes(listTwo, failedInsert, errorCode);

      // double inodes will succeed!

      CPPUNIT_ASSERT ( success == true );
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }
}

void TestDatabase::testInsertAndReadSingleDirInode()
{
   log.log(Log_DEBUG, "-testInsertAndReadSingleDirInode - started");

   FsckDirInodeList dirInodeListIn;
   FsckDirInodeList dirInodeListOut;

   FsckDirInode dirInode = DatabaseTk::createDummyFsckDirInode();

   dirInodeListIn.push_back(dirInode);

   try
   {
      // insert the inode
      FsckDirInode failedInsert;
      int errorCode;
      if (! this->db->insertDirInodes(dirInodeListIn, failedInsert, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // read into the outList
      this->db->getDirInodes(&dirInodeListOut);

      FsckDirInode inode = dirInodeListOut.front();

      bool compareRes = FsckTkEx::compareFsckDirInodeLists(&dirInodeListIn, &dirInodeListOut);
      CPPUNIT_ASSERT_MESSAGE("Comparision of dir inodes failed", compareRes);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }

   log.log(Log_DEBUG, "-testInsertAndReadSingleDirInode - success");
}

void TestDatabase::testInsertAndReadDirInodes()
{
   uint NUM_ROWS = 5;

   FsckDirInodeList dirInodeListIn;
   FsckDirInodeList dirInodeListOut;

   DatabaseTk::createDummyFsckDirInodes(NUM_ROWS, &dirInodeListIn);

   try
   {
      // insert the dir inodes
      FsckDirInode failedInsert;
      int errorCode;
      if (! this->db->insertDirInodes(dirInodeListIn, failedInsert, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // read into the outList
      this->db->getDirInodes(&dirInodeListOut);

      bool compareRes = FsckTkEx::compareFsckDirInodeLists(&dirInodeListIn, &dirInodeListOut);
      CPPUNIT_ASSERT_MESSAGE("Comparision of dir inodes failed", compareRes);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }
}

void TestDatabase::testInsertDoubleDirInodes()
{
   unsigned NUM_ROWS = 5;

   // generate NUM_ROWS rows
   FsckDirInodeList listOne;
   DatabaseTk::createDummyFsckDirInodes(NUM_ROWS, &listOne);

   // duplicate first itwm and put it into second list
   FsckDirInode duplicate = listOne.front();
   FsckDirInodeList listTwo;
   listTwo.push_back(duplicate);

   try
   {
      // insert the first list
      FsckDirInode failedInsert;
      int errorCode;

      if (! this->db->insertDirInodes(listOne, failedInsert, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // now insert the second list; there we expect to have a conflict with constraint
      // violation error code
      bool success = this->db->insertDirInodes(listTwo, failedInsert, errorCode);

      CPPUNIT_ASSERT ( success == false );
      CPPUNIT_ASSERT ( failedInsert == duplicate );
      CPPUNIT_ASSERT ( errorCode == SQLITE_CONSTRAINT );
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }
}

void TestDatabase::testInsertAndReadSingleChunk()
{
   FsckChunkList chunkListIn;
   FsckChunkList chunkListOut;

   FsckChunk chunk = DatabaseTk::createDummyFsckChunk();

   chunkListIn.push_back(chunk);

   try
   {
      // insert the chunks
      FsckChunk failedInsert;
      int errorCode;
      if (! this->db->insertChunks(chunkListIn, failedInsert, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // read into the outList
      this->db->getChunks(&chunkListOut);

      // check sizes
      CPPUNIT_ASSERT( chunkListIn.size() == chunkListOut.size() );

      bool compareRes = FsckTkEx::compareFsckChunkLists(&chunkListIn, &chunkListOut);
      CPPUNIT_ASSERT_MESSAGE("Comparision of chunks failed", compareRes);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }
}

void TestDatabase::testInsertAndReadChunks()
{
   uint NUM_ROWS = 5;

   FsckChunkList chunkListIn;
   FsckChunkList chunkListOut;

   DatabaseTk::createDummyFsckChunks(NUM_ROWS, &chunkListIn);

   try
   {
      // insert the chunks
      FsckChunk failedInsert;
      int errorCode;
      if (! this->db->insertChunks(chunkListIn, failedInsert, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // read into the outList
      this->db->getChunks(&chunkListOut);

      bool compareRes = FsckTkEx::compareFsckChunkLists(&chunkListIn, &chunkListOut);
      CPPUNIT_ASSERT_MESSAGE("Comparision of chunks failed", compareRes);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }
}

void TestDatabase::testInsertDoubleChunks()
{
   unsigned NUM_ROWS = 5;

   // generate NUM_ROWS rows
   FsckChunkList listOne;
   DatabaseTk::createDummyFsckChunks(NUM_ROWS, &listOne);

   // duplicate first itwm and put it into second list
   FsckChunk duplicate = listOne.front();
   FsckChunkList listTwo;
   listTwo.push_back(duplicate);

   try
   {
      // insert the first list
      FsckChunk failedInsert;
      int errorCode;

      if (! this->db->insertChunks(listOne, failedInsert, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // now insert the second list; there we expect to have a conflict with constraint
      // violation error code
      bool success = this->db->insertChunks(listTwo, failedInsert, errorCode);

      CPPUNIT_ASSERT ( success == false );
      CPPUNIT_ASSERT ( failedInsert == duplicate );
      CPPUNIT_ASSERT ( errorCode == SQLITE_CONSTRAINT );
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }
}

void TestDatabase::testInsertAndReadSingleContDir()
{
   FsckContDirList contDirListIn;
   FsckContDirList contDirListOut;

   FsckContDir contDir = DatabaseTk::createDummyFsckContDir();

   contDirListIn.push_back(contDir);

   try
   {
      // insert the cont dirs
      FsckContDir failedInsert;
      int errorCode;
      if (! this->db->insertContDirs(contDirListIn, failedInsert, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // read into the outList
      this->db->getContDirs(&contDirListOut);

      bool compareRes = FsckTkEx::compareFsckContDirLists(&contDirListIn, &contDirListOut);
      CPPUNIT_ASSERT_MESSAGE("Comparision of cont dirs failed", compareRes);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }
}

void TestDatabase::testInsertAndReadContDirs()
{
   uint NUM_ROWS = 5;

   FsckContDirList contDirListIn;
   FsckContDirList contDirListOut;

   DatabaseTk::createDummyFsckContDirs(NUM_ROWS, &contDirListIn);

   try
   {
      // insert the cont dirs
      FsckContDir failedInsert;
      int errorCode;
      if (! this->db->insertContDirs(contDirListIn, failedInsert, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // read into the outList
      this->db->getContDirs(&contDirListOut);

      bool compareRes = FsckTkEx::compareFsckContDirLists(&contDirListIn, &contDirListOut);
      CPPUNIT_ASSERT_MESSAGE("Comparision of cont dirs failed", compareRes);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }
}

void TestDatabase::testInsertDoubleContDirs()
{
   unsigned NUM_ROWS = 5;

   // generate NUM_ROWS rows
   FsckContDirList listOne;
   DatabaseTk::createDummyFsckContDirs(NUM_ROWS, &listOne);

   // duplicate first itwm and put it into second list
   FsckContDir duplicate = listOne.front();
   FsckContDirList listTwo;
   listTwo.push_back(duplicate);

   try
   {
      // insert the first list
      FsckContDir failedInsert;
      int errorCode;

      if (! this->db->insertContDirs(listOne, failedInsert, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // now insert the second list; there we expect to have a conflict with constraint
      // violation error code
      bool success = this->db->insertContDirs(listTwo, failedInsert, errorCode);

      CPPUNIT_ASSERT ( success == false );
      CPPUNIT_ASSERT ( failedInsert == duplicate );
      CPPUNIT_ASSERT ( errorCode == SQLITE_CONSTRAINT );
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }
}

void TestDatabase::testInsertAndReadSingleFsID()
{
   FsckFsIDList listIn;
   FsckFsIDList listOut;

   FsckFsID fsID = DatabaseTk::createDummyFsckFsID();

   listIn.push_back(fsID);

   try
   {
      // insert the fsid
      FsckFsID failedInsert;
      int errorCode;
      if (! this->db->insertFsIDs(listIn, failedInsert, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // read into the outList
      this->db->getFsIDs(&listOut);

      bool compareRes = FsckTkEx::compareFsckFsIDLists(&listIn, &listOut);
      CPPUNIT_ASSERT_MESSAGE("Comparision of fs ids failed", compareRes);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }
}

void TestDatabase::testInsertAndReadFsIDs()
{
   uint NUM_ROWS = 5;

   FsckFsIDList listIn;
   FsckFsIDList listOut;

   DatabaseTk::createDummyFsckFsIDs(NUM_ROWS, &listIn);

   try
   {
      // insert the fs ids
      FsckFsID failedInsert;
      int errorCode;
      if (! this->db->insertFsIDs(listIn, failedInsert, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // read into the outList
      this->db->getFsIDs(&listOut);

      bool compareRes = FsckTkEx::compareFsckFsIDLists(&listIn, &listOut);
      CPPUNIT_ASSERT_MESSAGE("Comparision of fs ids failed", compareRes);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }
}

void TestDatabase::testInsertDoubleFsIDs()
{
   unsigned NUM_ROWS = 5;

   // generate NUM_ROWS rows
   FsckFsIDList listOne;
   DatabaseTk::createDummyFsckFsIDs(NUM_ROWS, &listOne);

   // duplicate first itwm and put it into second list
   FsckFsID duplicate = listOne.front();
   FsckFsIDList listTwo;
   listTwo.push_back(duplicate);

   try
   {
      // insert the first list
      FsckFsID failedInsert;
      int errorCode;

      if (! this->db->insertFsIDs(listOne, failedInsert, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // now insert the second list; there we expect to have a conflict with constraint
      // violation error code
      bool success = this->db->insertFsIDs(listTwo, failedInsert, errorCode);

      CPPUNIT_ASSERT ( success == false );
      CPPUNIT_ASSERT ( failedInsert == duplicate );
      CPPUNIT_ASSERT ( errorCode == SQLITE_CONSTRAINT );
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }
}

void TestDatabase::testCheckForAndInsertDanglingDirEntries()
{
   log.log(Log_DEBUG, "-testCheckForAndInsertDanglingDirEntries - started");
   unsigned NUM_DENTRIES_DIRS = 5;
   unsigned NUM_DENTRIES_FILES = 5;

   unsigned MISSING_EACH = 2;

   // create dentries for dirs
   FsckDirEntryList dentriesDirs;
   DatabaseTk::createDummyFsckDirEntries(NUM_DENTRIES_DIRS, &dentriesDirs,
      FsckDirEntryType_DIRECTORY);

   // create dentries for files (start with ID after the NUM_DENTRIES_DIRS)
   FsckDirEntryList dentriesFiles;
   DatabaseTk::createDummyFsckDirEntries(NUM_DENTRIES_DIRS, NUM_DENTRIES_FILES, &dentriesFiles);

   // create dir inodes
   FsckDirInodeList dirInodes;
   DatabaseTk::createDummyFsckDirInodes(NUM_DENTRIES_DIRS-MISSING_EACH, &dirInodes);


   // create file inodes
   FsckFileInodeList fileInodes;
   DatabaseTk::createDummyFsckFileInodes(NUM_DENTRIES_DIRS, NUM_DENTRIES_FILES-MISSING_EACH,
      &fileInodes);

   try
   {
      // insert the dentries
      FsckDirEntry failedDirEntry;
      int errorCode;
      if ( ! this->db->insertDirEntries(dentriesDirs, failedDirEntry, errorCode))
         CPPUNIT_FAIL("Insertion of file dentries returned error");
      if ( ! this->db->insertDirEntries(dentriesFiles, failedDirEntry, errorCode))
         CPPUNIT_FAIL("Insertion of dir dentries returned error");

      // insert the inodes
      FsckFileInode failedFileInode;
      FsckDirInode failedDirInode;
      if (! this->db->insertFileInodes(fileInodes, failedFileInode, errorCode))
         CPPUNIT_FAIL("Insertion of file inodes returned error");
      if (! this->db->insertDirInodes(dirInodes, failedDirInode, errorCode))
         CPPUNIT_FAIL("Insertion of dir inodes returned error");

      // now insert all dentries without an inode into the errors table
      if (!this->db->checkForAndInsertDanglingDirEntries())
         CPPUNIT_FAIL("Check for and insert dentries without inode returned error");

      // get cursor to the entries
      FsckDirEntryList entries;
      DBCursor<FsckDirEntry>* cursor = this->db->getDanglingDirEntries();

      FsckDirEntry* currentEntry = cursor->open();
      while (currentEntry)
      {
         entries.push_back(*currentEntry);
         currentEntry = cursor->step();
      }

      cursor->close();

      SAFE_DELETE(cursor);

      int64_t rowCount = this->db->getRowCount(TABLE_NAME_ERROR(FsckErrorCode_DANGLINGDENTRY));

      CPPUNIT_ASSERT(rowCount == MISSING_EACH*2);
      CPPUNIT_ASSERT(entries.size() == MISSING_EACH*2);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }

   log.log(Log_DEBUG, "- testCheckForAndInsertDanglingDirEntries - success");
}

void TestDatabase::testCheckForAndInsertInodesWithWrongOwner()
{
   log.log(Log_DEBUG, "-testCheckForAndInsertInodesWithWrongOwner - started");

   unsigned NUM_INODES = 10;
   unsigned NUM_WRONG = 4;

   // create the inodes
   FsckDirInodeList dirInodes;
   DatabaseTk::createDummyFsckDirInodes(NUM_INODES, &dirInodes);

   // for all of the inodes set the same ownerNodeID and saveNodeID
   for ( FsckDirInodeListIter dirInodeIter = dirInodes.begin(); dirInodeIter != dirInodes.end();
      dirInodeIter++ )
   {
      dirInodeIter->ownerNodeID = 1000;
      dirInodeIter->saveNodeID = 1000;
   }

   // now set wrong ownerInfo for NUM_WRONG inodes
   unsigned i = 0;
   FsckDirInodeListIter iter = dirInodes.begin();
   while ( (iter != dirInodes.end()) && (i < NUM_WRONG) )
   {
      iter->ownerNodeID = iter->ownerNodeID + 1000;
      iter++;
      i++;
   }

   try
   {
      // save them to DB
      FsckDirInode failedInsert;
      int errorCode;
      if ( ! this->db->insertDirInodes(dirInodes, failedInsert, errorCode))
         CPPUNIT_FAIL("Insertion of file dentries returned error");

      // now check
      if ( !this->db->checkForAndInsertInodesWithWrongOwner() )
         CPPUNIT_FAIL("check for and insert inode with wrong owner returned error");

      // get cursor to the entries
      FsckDirInodeList inodes;
      DBCursor<FsckDirInode>* cursor = this->db->getInodesWithWrongOwner();

      FsckDirInode* currentInode = cursor->open();
      while ( currentInode )
      {
         inodes.push_back(*currentInode);
         currentInode = cursor->step();
      }

      cursor->close();

      SAFE_DELETE(cursor);

      int64_t rowCount = this->db->getRowCount(TABLE_NAME_ERROR(FsckErrorCode_WRONGINODEOWNER));

      CPPUNIT_ASSERT(rowCount == NUM_WRONG);
      CPPUNIT_ASSERT(inodes.size() == NUM_WRONG);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }

   log.log(Log_DEBUG, "-testCheckForAndInsertInodesWithWrongOwner - success");
}

void TestDatabase::testCheckForAndInsertDirEntriesWithWrongOwner()
{
   log.log(Log_DEBUG, "-testCheckForAndInsertDirEntriesWithWrongOwner - started");

   unsigned NUM_DENTRIES_DIRS = 5;
   unsigned NUM_DENTRIES_FILES = 5;

   unsigned WRONG_EACH = 2;

   // create dentries for dirs
   FsckDirEntryList dentriesDirs;
   DatabaseTk::createDummyFsckDirEntries(NUM_DENTRIES_DIRS, &dentriesDirs,
      FsckDirEntryType_DIRECTORY);

   // create dentries for files (start with ID after the NUM_DENTRIES_DIRS)
   FsckDirEntryList dentriesFiles;
   DatabaseTk::createDummyFsckDirEntries(NUM_DENTRIES_DIRS, NUM_DENTRIES_FILES, &dentriesFiles);

   // create dir inodes
   FsckDirInodeList dirInodes;
   DatabaseTk::createDummyFsckDirInodes(NUM_DENTRIES_DIRS, &dirInodes);

   // for all of the inodes set the same saveNodeID
   for (FsckDirInodeListIter dirInodeIter = dirInodes.begin(); dirInodeIter != dirInodes.end();
      dirInodeIter++)
   {
      dirInodeIter->saveNodeID = 1000;
   }

   // create file inodes
   FsckFileInodeList fileInodes;
   DatabaseTk::createDummyFsckFileInodes(NUM_DENTRIES_DIRS, NUM_DENTRIES_FILES,
      &fileInodes);

   // for all of the inodes set the same saveNodeID
   for (FsckFileInodeListIter fileInodeIter = fileInodes.begin(); fileInodeIter != fileInodes.end();
      fileInodeIter++)
   {
      fileInodeIter->saveNodeID = 1000;
   }

   // now manipulate the first WRONG_EACH dir entries and file entries to have wrong owner
   // information and the rest to be correct
   unsigned i = 0;

   FsckDirEntryListIter iter = dentriesDirs.begin();
   while ( ( iter != dentriesDirs.end() ) && (i < WRONG_EACH) )
   {
      iter->inodeOwnerNodeID = iter->inodeOwnerNodeID + 1000;
      iter++;
      i++;
   }

   while ( iter != dentriesDirs.end() )
   {
      iter->inodeOwnerNodeID = 1000;
      iter++;
      i++;
   }

   i = 0;
   iter = dentriesFiles.begin();
   while ( ( iter != dentriesFiles.end() ) && (i < WRONG_EACH) )
   {
      iter->inodeOwnerNodeID = iter->inodeOwnerNodeID + 1000;
      iter++;
      i++;
   }

   while ( iter != dentriesFiles.end() )
   {
       iter->inodeOwnerNodeID = 1000;
       iter++;
       i++;
    }

   try
   {
      // insert the dentries
      FsckDirEntry failedDirEntry;
      int errorCode;
      if (! this->db->insertDirEntries(dentriesDirs, failedDirEntry, errorCode))
         CPPUNIT_FAIL("Insertion of file dentries returned error");
      if (! this->db->insertDirEntries(dentriesFiles, failedDirEntry, errorCode) )
         CPPUNIT_FAIL("Insertion of dir dentries returned error");

      // insert the inodes
      FsckDirInode failedDirInode;
      FsckFileInode failedFileInode;
      if (! this->db->insertFileInodes(fileInodes, failedFileInode, errorCode) )
         CPPUNIT_FAIL("Insertion of file inodes returned error");
      if (! this->db->insertDirInodes(dirInodes, failedDirInode, errorCode) )
         CPPUNIT_FAIL("Insertion of dir inodes returned error");

      // now insert all dentries without wrong inode owner into the errors table
      if (!this->db->checkForAndInsertDirEntriesWithWrongOwner())
         CPPUNIT_FAIL("Check for and insert dentries with wrong owner returned error");

      // get cursor to the entries
      FsckDirEntryList entries;
      DBCursor<FsckDirEntry>* cursor = this->db->getDirEntriesWithWrongOwner();

      FsckDirEntry* currentEntry = cursor->open();
      while (currentEntry)
      {
         entries.push_back(*currentEntry);
         currentEntry = cursor->step();
      }

      cursor->close();

      SAFE_DELETE(cursor);

      int64_t rowCount = this->db->getRowCount(TABLE_NAME_ERROR(FsckErrorCode_WRONGOWNERINDENTRY));

      CPPUNIT_ASSERT(rowCount == WRONG_EACH*2);
      CPPUNIT_ASSERT(entries.size() == WRONG_EACH*2);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }

   log.log(Log_DEBUG, "-testCheckForAndInsertDirEntriesWithWrongOwner - success");
}

void TestDatabase::testCheckForAndInsertMissingDentryByIDFile()
{
   log.log(Log_DEBUG, "-testCheckForAndInsertMissingDentryByIDFile - started");

   unsigned NUM_DENTRIES = 50;
   unsigned NUM_DELETED_IDS = 5;
   unsigned NUM_WRONG_NODE_IDS = 5;
   unsigned NUM_WRONG_DEVICE_IDS = 5;
   unsigned NUM_WRONG_INODE_IDS = 5;

   unsigned ERRORS_TOTAL = NUM_DELETED_IDS + NUM_WRONG_NODE_IDS + NUM_WRONG_DEVICE_IDS
      + NUM_WRONG_INODE_IDS;

   // create dentries
   FsckDirEntryList dentries;
   DatabaseTk::createDummyFsckDirEntries(NUM_DENTRIES, &dentries);

   // create IDs
   FsckFsIDList fsIDs;
   DatabaseTk::createDummyFsckFsIDs(NUM_DENTRIES, &fsIDs);

   // make sure the data is correct
   FsckDirEntryListIter dentryIter = dentries.begin();
   FsckFsIDListIter fsIDIter = fsIDs.begin();
   for (uint i = 0; i< NUM_DENTRIES; i++)
   {
      dentryIter->saveNodeID = i;
      dentryIter->parentDirID = "parent";
      dentryIter->saveDevice = i+1000;
      dentryIter->saveInode = i+2000;

      fsIDIter->saveNodeID = i;
      fsIDIter->parentDirID = "parent";
      fsIDIter->saveDevice = i+1000;
      fsIDIter->saveInode = i+2000;

      dentryIter++;
      fsIDIter++;
   }

   // now manipulate the first NUM_WRONG_NODE_IDS fsids

   fsIDIter = fsIDs.begin();
   for (unsigned i = 0 ; i < NUM_WRONG_NODE_IDS; i++)
   {
      if (fsIDIter == fsIDs.end())
         CPPUNIT_FAIL("End of list reached! Most likely a programmer's error!");

      fsIDIter->saveNodeID = fsIDIter->getSaveNodeID() + 1000;
      fsIDIter++;
   }

   // now manipulate NUM_WRONG_DEVICE_IDS fsids
   for (unsigned i = 0 ; i < NUM_WRONG_DEVICE_IDS; i++)
   {
      if (fsIDIter == fsIDs.end())
         CPPUNIT_FAIL("End of list reached! Most likely a programmer's error!");

      fsIDIter->saveDevice = fsIDIter->getSaveDevice() + 1000;
      fsIDIter++;
   }

   // now manipulate NUM_WRONG_INODE_IDS fsids
   for (unsigned i = 0 ; i < NUM_WRONG_INODE_IDS; i++)
   {
      if (fsIDIter == fsIDs.end())
         CPPUNIT_FAIL("End of list reached! Most likely a programmer's error!");

      fsIDIter->saveInode = fsIDIter->getSaveInode() + 1000;
      fsIDIter++;
   }

   // now delete NUM_DELETE_IDS fsids
   for (unsigned i = 0 ; i < NUM_DELETED_IDS; i++)
   {
      if (fsIDIter == fsIDs.end())
         CPPUNIT_FAIL("End of list reached! Most likely a programmer's error!");

      fsIDIter = fsIDs.erase(fsIDIter);
   }

   try
   {
      // insert the dentries
      FsckDirEntry failedDirEntry;
      int errorCode;
      if (! this->db->insertDirEntries(dentries, failedDirEntry, errorCode) )
         CPPUNIT_FAIL("Insertion of dentries returned error");

      FsckFsID failedFsIDs;
      // insert the IDs
      if (! this->db->insertFsIDs(fsIDs, failedFsIDs, errorCode) )
         CPPUNIT_FAIL("Insertion of IDs returned error");

      // now do the check
      if (!this->db->checkForAndInsertDirEntriesWithBrokenByIDFile())
         CPPUNIT_FAIL("Check for and insert missing dentry by ID file returned error");

      // get cursor to the entries
      FsckDirEntryList entries;
      DBCursor<FsckDirEntry>* cursor = this->db->getDirEntriesWithBrokenByIDFile();

      FsckDirEntry* currentEntry = cursor->open();
      while (currentEntry)
      {
         entries.push_back(*currentEntry);
         currentEntry = cursor->step();
      }

      cursor->close();

      SAFE_DELETE(cursor);

      int64_t rowCount = this->db->getRowCount(TABLE_NAME_ERROR(FsckErrorCode_BROKENFSID));

      CPPUNIT_ASSERT(rowCount == ERRORS_TOTAL);
      CPPUNIT_ASSERT(entries.size() == ERRORS_TOTAL);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }

   log.log(Log_DEBUG, "-testCheckForAndInsertMissingDentryByIDFile - success");
}

void TestDatabase::testCheckForAndInsertOrphanedDentryByIDFiles()
{
   log.log(Log_DEBUG, "-testCheckForAndInsertOrphanedDentryByIDFiles - started");

   unsigned NUM_DENTRIES = 50;
   unsigned NUM_DELETES = 5;

   // create dentries
   FsckDirEntryList dentries;
   DatabaseTk::createDummyFsckDirEntries(NUM_DENTRIES, &dentries);

   // create IDs
   FsckFsIDList fsIDs;
   DatabaseTk::createDummyFsckFsIDs(NUM_DENTRIES, &fsIDs);

   // make sure the data is correct
   FsckDirEntryListIter dentryIter = dentries.begin();
   FsckFsIDListIter fsIDIter = fsIDs.begin();
   for (uint i = 0; i< NUM_DENTRIES; i++)
   {
      dentryIter->saveNodeID = i;
      dentryIter->parentDirID = "parent";
      dentryIter->saveDevice = i+1000;
      dentryIter->saveInode = i+2000;

      fsIDIter->saveNodeID = i;
      fsIDIter->parentDirID = "parent";
      fsIDIter->saveDevice = i+1000;
      fsIDIter->saveInode = i+2000;

      dentryIter++;
      fsIDIter++;
   }

   // now delete NUM_DELETES dentry
   dentryIter = dentries.begin();
   for (unsigned i = 0 ; i < NUM_DELETES; i++)
   {
      if (dentryIter == dentries.end())
         CPPUNIT_FAIL("End of list reached! Most likely a programmer's error!");

      dentryIter = dentries.erase(dentryIter);
   }

   try
   {
      // insert the dentries
      FsckDirEntry failedDirEntry;
      FsckFsID failedFsID;
      int errorCode;
      if (! this->db->insertDirEntries(dentries, failedDirEntry, errorCode) )
         CPPUNIT_FAIL("Insertion of dentries returned error");

      // insert the IDs
      if (! this->db->insertFsIDs(fsIDs, failedFsID, errorCode) )
         CPPUNIT_FAIL("Insertion of IDs returned error");

      // now do the check
      if (!this->db->checkForAndInsertOrphanedDentryByIDFiles())
         CPPUNIT_FAIL("Check for and insert orphaned dentry by ID file returned error");

      // get cursor to the fsIDs
      FsckFsIDList fsIDs;
      DBCursor<FsckFsID>* cursor = this->db->getOrphanedDentryByIDFiles();

      FsckFsID* currentEntry = cursor->open();
      while (currentEntry)
      {
         fsIDs.push_back(*currentEntry);
         currentEntry = cursor->step();
      }

      cursor->close();

      SAFE_DELETE(cursor);

      int64_t rowCount = this->db->getRowCount(TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDFSID));

      CPPUNIT_ASSERT(rowCount == NUM_DELETES);
      CPPUNIT_ASSERT(fsIDs.size() == NUM_DELETES);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }

   log.log(Log_DEBUG, "-testCheckForAndInsertOrphanedDentryByIDFiles - success");
}

void TestDatabase::testCheckForAndInsertOrphanedDirInodes()
{
   log.log(Log_DEBUG, "-testCheckForAndInsertOrphanedDirInodes - started");

   unsigned NUM_INODES = 5;
   unsigned MISSING_DENTRIES = 2;

   // create dir inodes
   FsckDirInodeList dirInodes;
   DatabaseTk::createDummyFsckDirInodes(NUM_INODES, &dirInodes);

   // create dentries for dirs
   FsckDirEntryList dentriesDirs;
   DatabaseTk::createDummyFsckDirEntries(NUM_INODES-MISSING_DENTRIES, &dentriesDirs,
      FsckDirEntryType_DIRECTORY);

   // create some dentries for files (just to see what happens if they are present)
   FsckDirEntryList dentriesFiles;
   DatabaseTk::createDummyFsckDirEntries(NUM_INODES, NUM_INODES+NUM_INODES, &dentriesFiles);

   try
   {
      // insert the dentries
      FsckDirEntry failedDirEntry;
      FsckDirInode failedDirInode;
      int errorCode;
      if (! this->db->insertDirEntries(dentriesDirs, failedDirEntry, errorCode) )
         CPPUNIT_FAIL("Insertion of dir dentries returned error");

      if (! this->db->insertDirEntries(dentriesFiles, failedDirEntry, errorCode) )
         CPPUNIT_FAIL("Insertion of file dentries returned error");

      if (! this->db->insertDirInodes(dirInodes, failedDirInode, errorCode) )
         CPPUNIT_FAIL("Insertion of dir inodes returned error");

      // now do the check
      if ( !this->db->checkForAndInsertOrphanedDirInodes() )
         CPPUNIT_FAIL("Check for and insert orphaned dir inodes returned error");

      // get cursor to the entries
      FsckDirInodeList inodes;
      DBCursor<FsckDirInode>* cursor = this->db->getOrphanedDirInodes();

      FsckDirInode* currentEntry = cursor->open();
      while ( currentEntry )
      {
         inodes.push_back(*currentEntry);
         currentEntry = cursor->step();
      }

      cursor->close();

      SAFE_DELETE(cursor);

      int64_t rowCount = this->db->getRowCount(TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDDIRINODE));
      CPPUNIT_ASSERT(rowCount == MISSING_DENTRIES);
      CPPUNIT_ASSERT(inodes.size() == MISSING_DENTRIES);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }

   log.log(Log_DEBUG, "-testCheckForAndInsertOrphanedDirInodes - success");
}

void TestDatabase::testCheckForAndInsertOrphanedFileInodes()
{
   log.log(Log_DEBUG, "-testCheckForAndInsertOrphanedFileInodes - started");

   unsigned NUM_INODES = 5;
   unsigned MISSING_DENTRIES = 2;

   // create file inodes
   FsckFileInodeList fileInodes;
   DatabaseTk::createDummyFsckFileInodes(NUM_INODES, &fileInodes);

   // create dentries for files
   FsckDirEntryList dentriesFiles;
   DatabaseTk::createDummyFsckDirEntries(NUM_INODES-MISSING_DENTRIES, &dentriesFiles);

   // create some dentries for dirs (just to see what happens if they are present)
   FsckDirEntryList dentriesDirs;
   DatabaseTk::createDummyFsckDirEntries(NUM_INODES, NUM_INODES+NUM_INODES, &dentriesDirs,
      FsckDirEntryType_DIRECTORY);

   try
   {
      FsckDirEntry failedDirEntry;
      FsckFileInode failedFileInode;
      int errorCode;
      // insert the dentries
      if (! this->db->insertDirEntries(dentriesDirs, failedDirEntry, errorCode) )
         CPPUNIT_FAIL("Insertion of dir dentries returned error");

      if (! this->db->insertDirEntries(dentriesFiles, failedDirEntry, errorCode))
         CPPUNIT_FAIL("Insertion of file dentries returned error");

      if (! this->db->insertFileInodes(fileInodes, failedFileInode, errorCode) )
         CPPUNIT_FAIL("Insertion of file inodes returned error");

      // now do the check
      if ( !this->db->checkForAndInsertOrphanedFileInodes() )
         CPPUNIT_FAIL("Check for and insert orphaned file inodes returned error");

      // get cursor to the entries
      FsckFileInodeList inodes;
      DBCursor<FsckFileInode>* cursor = this->db->getOrphanedFileInodes();

      FsckFileInode* currentEntry = cursor->open();
      while ( currentEntry )
      {
         inodes.push_back(*currentEntry);
         currentEntry = cursor->step();
      }

      cursor->close();

      SAFE_DELETE(cursor);

      int64_t rowCount = this->db->getRowCount(TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDFILEINODE));
      CPPUNIT_ASSERT(rowCount == MISSING_DENTRIES);
      CPPUNIT_ASSERT(inodes.size() == MISSING_DENTRIES);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }

   log.log(Log_DEBUG, "-testCheckForAndInsertOrphanedFileInodes - success");
}

void TestDatabase::testCheckForAndInsertOrphanedChunks()
{
   log.log(Log_DEBUG, "-testCheckForAndInsertOrphanedChunks - started");

   unsigned NUM_CHUNKS = 5;
   unsigned MISSING_INODES = 2;
   unsigned NUM_TARGETS = 2;

   // create chunks
   FsckChunkList chunks;
   DatabaseTk::createDummyFsckChunks(NUM_CHUNKS, NUM_TARGETS, &chunks);

   // create file inodes
   FsckFileInodeList fileInodes;
   DatabaseTk::createDummyFsckFileInodes(NUM_CHUNKS-MISSING_INODES, &fileInodes);

   try
   {
      FsckChunk failedChunk;
      FsckFileInode failedFileInode;
      int errorCode;
      // insert the entries
      if (! this->db->insertChunks(chunks, failedChunk, errorCode) )
         CPPUNIT_FAIL("Insertion of chunks returned error");

      if (! this->db->insertFileInodes(fileInodes, failedFileInode, errorCode) )
         CPPUNIT_FAIL("Insertion of file inodes returned error");

      // now do the check
      if ( !this->db->checkForAndInsertOrphanedChunks() )
         CPPUNIT_FAIL("Check for and insert orphaned chunks returned error");

      FsckChunkList chunks;
      DBCursor<FsckChunk>* cursor = this->db->getOrphanedChunks();

      FsckChunk* currentEntry = cursor->open();
      while ( currentEntry )
      {
         chunks.push_back(*currentEntry);
         currentEntry = cursor->step();
      }

      cursor->close();

      SAFE_DELETE(cursor);

      int64_t rowCount = this->db->getRowCount(TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDCHUNK));

      CPPUNIT_ASSERT(rowCount == MISSING_INODES*NUM_TARGETS);

      // in the list, the chunks will be together with targetIDs
      CPPUNIT_ASSERT(chunks.size() == MISSING_INODES*NUM_TARGETS);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }

   log.log(Log_DEBUG, "-testCheckForAndInsertOrphanedChunks - success");
}

void TestDatabase::testCheckForAndInsertInodesWithoutContDir()
{
   log.log(Log_DEBUG, "-testCheckForAndInsertInodesWithoutContDir - started");

   unsigned NUM_INODES = 5;
   unsigned MISSING_CONTDIRS = 2;

   // create dir inodes
   FsckDirInodeList dirInodes;
   DatabaseTk::createDummyFsckDirInodes(NUM_INODES, &dirInodes);

   // create cont dirs
   FsckContDirList contDirs;
   DatabaseTk::createDummyFsckContDirs(NUM_INODES-MISSING_CONTDIRS, &contDirs);

   try
   {
      FsckDirInode failedDirInode;
      int errorCode;
      if (! this->db->insertDirInodes(dirInodes, failedDirInode, errorCode) )
         CPPUNIT_FAIL("Insertion of dir inodes returned error");

      FsckContDir failedContDir;
      if (! this->db->insertContDirs(contDirs, failedContDir, errorCode) )
         CPPUNIT_FAIL("Insertion of cont dirs returned error");

      // now do the check
      if ( !this->db->checkForAndInsertInodesWithoutContDir() )
         CPPUNIT_FAIL("Check for and insert inodes without cont dir returned error");

      FsckDirInodeList inodes;
      DBCursor<FsckDirInode>* cursor = this->db->getInodesWithoutContDir();

      FsckDirInode* currentEntry = cursor->open();
      while ( currentEntry )
      {
         inodes.push_back(*currentEntry);
         currentEntry = cursor->step();
      }

      cursor->close();

      SAFE_DELETE(cursor);

      int64_t rowCount = this->db->getRowCount(TABLE_NAME_ERROR(FsckErrorCode_MISSINGCONTDIR));
      CPPUNIT_ASSERT(rowCount == MISSING_CONTDIRS);

      CPPUNIT_ASSERT(inodes.size() == MISSING_CONTDIRS);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }

   log.log(Log_DEBUG, "-testCheckForAndInsertInodesWithoutContDir - success");
}

void TestDatabase::testCheckForAndInsertOrphanedContDirs()
{
   log.log(Log_DEBUG, "-testCheckForAndInsertOrphanedContDirs - started");

   unsigned NUM_CONTDIRS = 5;
   unsigned MISSING_INODES = 2;

   // create cont dirs
   FsckContDirList contDirs;
   DatabaseTk::createDummyFsckContDirs(NUM_CONTDIRS, &contDirs);

   // create dir inodes
   FsckDirInodeList dirInodes;
   DatabaseTk::createDummyFsckDirInodes(NUM_CONTDIRS-MISSING_INODES, &dirInodes);

   try
   {
      FsckContDir failedContDir;
      FsckDirInode failedDirInode;
      int errorCode;
      if (! this->db->insertContDirs(contDirs, failedContDir, errorCode))
         CPPUNIT_FAIL("Insertion of cont dirs returned error");

      if (! this->db->insertDirInodes(dirInodes, failedDirInode, errorCode) )
         CPPUNIT_FAIL("Insertion of dir inodes returned error");

      // now do the check
      if ( !this->db->checkForAndInsertOrphanedContDirs() )
         CPPUNIT_FAIL("Check for and insert orphaned cont dirs returned error");

      FsckContDirList contDirs;
      DBCursor<FsckContDir>* cursor = this->db->getOrphanedContDirs();

      FsckContDir* currentEntry = cursor->open();
      while ( currentEntry )
      {
         contDirs.push_back(*currentEntry);
         currentEntry = cursor->step();
      }

      cursor->close();

      SAFE_DELETE(cursor);

      int64_t rowCount = this->db->getRowCount(TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDCONTDIR));
      CPPUNIT_ASSERT(rowCount == MISSING_INODES);

      CPPUNIT_ASSERT(contDirs.size() == MISSING_INODES);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }

   log.log(Log_DEBUG, "-testCheckForAndInsertOrphanedContDirs - started");
}

void TestDatabase::testCheckForAndInsertFileInodesWithWrongAttribs()
{
   log.log(Log_DEBUG, "-testCheckForAndInsertFileInodesWithWrongAttribs - started");

   unsigned NUM_INODES = 10;
   unsigned NUM_CHUNKS_PER_INODE = 2;

   unsigned WRONG_DATA_INODES_SIZE = 2;
   unsigned WRONG_DATA_INODES_HARDLINKS = 2;

   unsigned WRONG_DATA_INODES = WRONG_DATA_INODES_SIZE + WRONG_DATA_INODES_HARDLINKS;

   int64_t EXPECTED_FILE_SIZE = 100000; // take a value that can be divided by NUM_CHUNKS_PER_INODE
   unsigned EXPECTED_HARDLINKS = 1;

   // create file inodes
   FsckFileInodeList inodesIn;
   DatabaseTk::createDummyFsckFileInodes(NUM_INODES, &inodesIn);

   // create chunks
   FsckChunkList chunks;
   DatabaseTk::createDummyFsckChunks(NUM_INODES, NUM_CHUNKS_PER_INODE, &chunks);

   // create dentries
   FsckDirEntryList dentries;
   DatabaseTk::createDummyFsckDirEntries(NUM_INODES, &dentries);

   // add another dentry with the ID of the last dentry to simulate a hardlink
   FsckDirEntry dentry = dentries.back();
   dentry.name = "hardlink";

   dentries.push_back(dentry);

   // make sure stat attrs do match
   for ( FsckFileInodeListIter iter = inodesIn.begin(); iter != inodesIn.end(); iter++ )
   {
      iter->setFileSize(EXPECTED_FILE_SIZE);
      iter->setNumHardLinks(EXPECTED_HARDLINKS);

      if (iter->getID() == dentry.getID())
         iter->setNumHardLinks(iter->getNumHardLinks()+1);
   }

   for ( FsckChunkListIter iter = chunks.begin(); iter != chunks.end(); iter++ )
   {
      FsckChunk chunk = *iter;
      iter->fileSize = EXPECTED_FILE_SIZE/NUM_CHUNKS_PER_INODE;
   }

   // we hold a list of modifed IDs; each time we modify attributes for a chunk we add its ID
   // we do this to make sure, that if we modify x inodes, they are x different ones
   StringList modifiedIDs;

   // now manipulate data of WRONG_DATA_INODES_SIZE chunks
   FsckChunkListIter chunkIter = chunks.begin();
   // only manipulate chunks from one target
   uint16_t targetID = chunkIter->getTargetID();
   unsigned i = 0;
   while (i < WRONG_DATA_INODES_SIZE)
   {
      if (chunkIter == chunks.end())
         break;

      if (chunkIter->getTargetID() == targetID)
      {
         StringListIter tmpIter = std::find(modifiedIDs.begin(), modifiedIDs.end(),
            chunkIter->getID());

         if (tmpIter == modifiedIDs.end())
         {
            modifiedIDs.push_back(chunkIter->getID());
            chunkIter->fileSize = chunkIter->fileSize*2;
            i++;
         }
      }

      chunkIter++;
   }

   // now manipulate data for WRONG_DATA_INODES_HARDLINKS inodes
   FsckFileInodeListIter inodeIter = inodesIn.begin();

   i = 0;
   while ( i < WRONG_DATA_INODES_HARDLINKS )
   {
      if ( inodeIter == inodesIn.end() )
         break;

      StringListIter tmpIter = std::find(modifiedIDs.begin(), modifiedIDs.end(),
         inodeIter->getID());

      if ( tmpIter == modifiedIDs.end() )
      {
         modifiedIDs.push_back(inodeIter->getID());
         inodeIter->setNumHardLinks(100);
         i++;
      }

      inodeIter++;
   }

   try
   {
      FsckFileInode failedFileInode;
      FsckChunk failedChunk;
      FsckDirEntry failedDentry;
      int errorCode;
      if (! this->db->insertFileInodes(inodesIn, failedFileInode, errorCode) )
         CPPUNIT_FAIL("Insertion of file inodes returned error");

      if (! this->db->insertChunks(chunks, failedChunk, errorCode) )
         CPPUNIT_FAIL("Insertion of chunks returned error");

      if (! this->db->insertDirEntries(dentries, failedDentry, errorCode) )
         CPPUNIT_FAIL("Insertion of dentries returned error");

      // now do the check
      if ( !this->db->checkForAndInsertWrongInodeFileAttribs() )
         CPPUNIT_FAIL("Check for and insert wrong inode attribs returned error");

      FsckFileInodeList inodesOut;
      DBCursor<FsckFileInode>* cursor = this->db->getInodesWithWrongFileAttribs();

      FsckFileInode* currentEntry = cursor->open();
      while ( currentEntry )
      {
         inodesOut.push_back(*currentEntry);
         currentEntry = cursor->step();
      }

      cursor->close();

      SAFE_DELETE(cursor);

      int64_t rowCount = this->db->getRowCount(TABLE_NAME_ERROR(FsckErrorCode_WRONGFILEATTRIBS));

      CPPUNIT_ASSERT(rowCount == WRONG_DATA_INODES);

      CPPUNIT_ASSERT(inodesOut.size() == WRONG_DATA_INODES);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }

   log.log(Log_DEBUG, "-testCheckForAndInsertFileInodesWithWrongAttribs - success");
}

void TestDatabase::testCheckForAndInsertDirInodesWithWrongAttribs()
{
   log.log(Log_DEBUG, "-testCheckForAndInsertDirInodesWithWrongAttribs - started");

   unsigned NUM_INODES = 10;

   unsigned WRONG_DATA_SIZE = 2;
   unsigned WRONG_DATA_LINKS = 2;

   unsigned WRONG_DATA_INODES = WRONG_DATA_SIZE + WRONG_DATA_LINKS;

   int64_t EXPECTED_SIZE = 2;
   unsigned EXPECTED_LINKS = 2; // no subdirs present, so 2 links expected

   // create dir inodes
   FsckDirInodeList inodes;
   DatabaseTk::createDummyFsckDirInodes(NUM_INODES, &inodes);

   // create dentries
   FsckDirEntryList dentries;
   DatabaseTk::createDummyFsckDirEntries(NUM_INODES*EXPECTED_LINKS, &dentries);

   // set parent info, so that it matches
   FsckDirEntryListIter dentryIter = dentries.begin();
   FsckDirInodeListIter inodeIter = inodes.begin();

   while ( inodeIter != inodes.end() )
   {
      inodeIter->setSize(EXPECTED_SIZE);
      inodeIter->setNumHardLinks(EXPECTED_LINKS);

      for ( int i = 0; i < EXPECTED_SIZE; i++ )
      {
         dentryIter->setParentDirID(inodeIter->getID());
         dentryIter++;
      }
      inodeIter++;
   }

   // we hold a list of modifed IDs; each time we modify attributes we add the ID of the inode
   // we do this to make sure, that if we modify x inodes, they are x different ones
   StringList modifiedIDs;

   // now manipulate the size
   inodeIter = inodes.begin();

   unsigned i = 0;
   while ( i < WRONG_DATA_SIZE )
   {
      if ( inodeIter == inodes.end() )
         break;

      StringListIter tmpIter = std::find(modifiedIDs.begin(), modifiedIDs.end(),
         inodeIter->getID());

      if ( tmpIter == modifiedIDs.end() )
      {
         modifiedIDs.push_back(inodeIter->getID());
         // set new size
         inodeIter->setSize(inodeIter->getSize()*10);
         i++;
      }

      inodeIter++;
   }

   // now manipulate link count
   i = 0;
   while ( i < WRONG_DATA_LINKS )
   {
      if ( inodeIter == inodes.end() )
         break;

      StringListIter tmpIter = std::find(modifiedIDs.begin(), modifiedIDs.end(),
         inodeIter->getID());

      if ( tmpIter == modifiedIDs.end() )
      {
         modifiedIDs.push_back(inodeIter->getID());
         // set new numLinks
         inodeIter->setNumHardLinks(inodeIter->getNumHardLinks()*10);
         i++;
      }

      inodeIter++;
   }

   try
   {
      FsckDirInode failedInode;
      FsckDirEntry failedDentry;
      int errorCode;
      if (! this->db->insertDirInodes(inodes, failedInode, errorCode) )
         CPPUNIT_FAIL("Insertion of dir inodes returned error");

      if (! this->db->insertDirEntries(dentries, failedDentry, errorCode) )
         CPPUNIT_FAIL("Insertion of dentries returned error");

      // now do the check
      if ( !this->db->checkForAndInsertWrongInodeDirAttribs() )
         CPPUNIT_FAIL("Check for and insert wrong inode attribs returned error");

      FsckDirInodeList inodesOut;
      DBCursor<FsckDirInode>* cursor = this->db->getInodesWithWrongDirAttribs();

      FsckDirInode* currentEntry = cursor->open();
      while ( currentEntry )
      {
         inodesOut.push_back(*currentEntry);
         currentEntry = cursor->step();
      }

      cursor->close();

      SAFE_DELETE(cursor);

      int64_t rowCount = this->db->getRowCount(TABLE_NAME_ERROR(FsckErrorCode_WRONGDIRATTRIBS));

      CPPUNIT_ASSERT(rowCount == WRONG_DATA_INODES);

      CPPUNIT_ASSERT(inodesOut.size() == WRONG_DATA_INODES);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }

   log.log(Log_DEBUG, "-testCheckForAndInsertDirInodesWithWrongAttribs - success");
}

void TestDatabase::testCheckForAndInsertMissingStripeTargets()
{
   log.log(Log_DEBUG, "-testCheckForAndInsertMissingStripeTargets - started");

   unsigned EXISTING_TARGETS = 4;
   unsigned USED_TARGETS = EXISTING_TARGETS*2;

   // we need a TargetMapper and a MirrorBuddyGroupMapper to test this
   TargetMapper* targetMapper = new TargetMapper();
   MirrorBuddyGroupMapper* buddyGroupMapper = new MirrorBuddyGroupMapper(targetMapper);
   FsckTargetIDList usedTargets;

   uint16_t targetID = 1;

   for (; targetID<=EXISTING_TARGETS; targetID++)
   {
      targetMapper->mapTarget(targetID,1);
      usedTargets.push_back(FsckTargetID(targetID, FsckTargetIDType_TARGET));
   }

   for (; targetID<=USED_TARGETS; targetID++)
   {
      usedTargets.push_back(FsckTargetID(targetID, FsckTargetIDType_TARGET));
   }

   try
   {
      FsckTargetID failedTarget;
      int errorCode;
      if (! this->db->insertUsedTargetIDs(usedTargets, failedTarget, errorCode) )
         CPPUNIT_FAIL("Insertion of used targets returned error");

      // now do the check
      if ( !this->db->checkForAndInsertMissingStripeTargets(targetMapper, buddyGroupMapper) )
         CPPUNIT_FAIL("Check for and insert missing stripe targets returned error");

      FsckTargetIDList missingStripeTargets;
      DBCursor<FsckTargetID>* cursor = this->db->getMissingStripeTargets();

      FsckTargetID* currentEntry = cursor->open();
      while ( currentEntry )
      {
         missingStripeTargets.push_back(*currentEntry);
         currentEntry = cursor->step();
      }

      cursor->close();

      SAFE_DELETE(cursor);

      int64_t rowCount = this->db->getRowCount(TABLE_NAME_ERROR(FsckErrorCode_MISSINGTARGET));

      CPPUNIT_ASSERT(rowCount == USED_TARGETS-EXISTING_TARGETS);

      CPPUNIT_ASSERT(missingStripeTargets.size() == USED_TARGETS-EXISTING_TARGETS);

   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }

   log.log(Log_DEBUG, "-testCheckForAndInsertMissingStripeTargets - success");
}

void TestDatabase::testCheckForAndInsertFilesWithMissingStripeTargets()
{
   log.log(Log_DEBUG, "-testCheckForAndInsertFilesWithMissingStripeTargets - started");

   unsigned EXISTING_TARGETS = 4;
   unsigned USED_TARGETS = EXISTING_TARGETS*2;
   unsigned NUM_FILES = 20;
   unsigned WRONG_FILES = 10;

   // we need a TargetMapper and a MirrorBuddyGroupMapper to test this
   TargetMapper* targetMapper = new TargetMapper();
   MirrorBuddyGroupMapper* buddyGroupMapper = new MirrorBuddyGroupMapper(targetMapper);
   FsckTargetIDList usedTargets;

   uint16_t targetID = 1;

   for (; targetID<=EXISTING_TARGETS; targetID++)
   {
      targetMapper->mapTarget(targetID,1);
      usedTargets.push_back(FsckTargetID(targetID, FsckTargetIDType_TARGET));
   }

   for (; targetID<=USED_TARGETS; targetID++)
   {
      usedTargets.push_back(FsckTargetID(targetID, FsckTargetIDType_TARGET));
   }

   try
   {
      // insert the used targets
      FsckTargetID failedTarget;
      int errorCode;
      if (! this->db->insertUsedTargetIDs(usedTargets, failedTarget, errorCode) )
         CPPUNIT_FAIL("Insertion of used targets returned error");

      // now do the check for missing targets
      if ( !this->db->checkForAndInsertMissingStripeTargets(targetMapper, buddyGroupMapper) )
         CPPUNIT_FAIL("Check for and insert missing stripe targets returned error");

      FsckTargetIDList missingStripeTargets;
      this->db->getMissingStripeTargets(&missingStripeTargets);

      // create some files
      FsckFileInodeList inodes;
      DatabaseTk::createDummyFsckFileInodes(NUM_FILES, &inodes);

      FsckDirEntryList dentries;

      unsigned i = 0;
      for (FsckFileInodeListIter iter = inodes.begin(); iter != inodes.end(); iter++, i++)
      {
         // set stripe pattern
         UInt16Vector stripeTargets;
         uint16_t targetID = 1;
         for (; targetID<=EXISTING_TARGETS; targetID++)
         {
            stripeTargets.push_back(targetID);
         }
         if (i<WRONG_FILES)
         {
            for (FsckTargetIDListIter missingTargetsIter = missingStripeTargets.begin(); missingTargetsIter != missingStripeTargets.end(); missingTargetsIter++)
            {
               stripeTargets.push_back(missingTargetsIter->getID());
            }
         }
         iter->setStripeTargets(stripeTargets);

         // create dentry
         FsckDirEntry dentry(iter->getID(), "file_" + iter->getID(), iter->getParentDirID(),
            iter->getSaveNodeID(), iter->getSaveNodeID(), FsckDirEntryType_REGULARFILE,
            false, iter->getSaveNodeID(), 1, 1);
         dentries.push_back(dentry);
      }

      FsckFileInode failedFileInode;
      FsckDirEntry failedDentry;
      if (! this->db->insertFileInodes(inodes, failedFileInode, errorCode))
         CPPUNIT_FAIL("Insertion of file inodes returned error");
      if (! this->db->insertDirEntries(dentries, failedDentry, errorCode))
         CPPUNIT_FAIL("Insertion of dentries returned error");

      if ( !this->db->checkForAndInsertFilesWithMissingStripeTargets() )
         CPPUNIT_FAIL("Check for and insert files with missing stripe targets returned error");

      FsckDirEntryList badFiles;
      DBCursor<FsckDirEntry>* cursor = this->db->getFilesWithMissingStripeTargets();

      FsckDirEntry* currentEntry = cursor->open();
      while ( currentEntry )
      {
         badFiles.push_back(*currentEntry);
         currentEntry = cursor->step();
      }

      cursor->close();

      SAFE_DELETE(cursor);

      int64_t rowCount = this->db->getRowCount(
         TABLE_NAME_ERROR(FsckErrorCode_FILEWITHMISSINGTARGET));

      CPPUNIT_ASSERT(rowCount == WRONG_FILES);

      CPPUNIT_ASSERT(badFiles.size() == WRONG_FILES);

   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }

   log.log(Log_DEBUG, "-testCheckForAndInsertFilesWithMissingStripeTargets - success");
}

void TestDatabase::testCheckForAndInsertChunksWithWrongPermissions()
{
   log.log(Log_DEBUG, "-testCheckForAndInsertChunksWithWrongPermissions - started");

   unsigned NUMBER_CHUNKS = 500;
   unsigned DIFF_UID = 50;
   unsigned DIFF_GID = 50;
   unsigned DIFF_TOTAL = DIFF_UID + DIFF_GID;

   CPPUNIT_ASSERT_MESSAGE("Programming error in test!", NUMBER_CHUNKS > DIFF_TOTAL);

   FsckFileInodeList fileInodes;
   DatabaseTk::createDummyFsckFileInodes(NUMBER_CHUNKS, &fileInodes);

   FsckChunkList chunks;
   DatabaseTk::createDummyFsckChunks(NUMBER_CHUNKS, &chunks);

   FsckFileInodeListIter iter = fileInodes.begin();

   for (unsigned i = 0; i < DIFF_UID; i++)
   {
      iter->setUserID(1000);
      iter++;
   }

   for (unsigned i = 0; i < DIFF_GID; i++)
   {
      iter->setGroupID(1000);
      iter++;
   }

   //rest is owned by root
   while (iter != fileInodes.end())
   {
      iter->setUserID(0);
      iter->setGroupID(0);
      iter++;
   }

   try
   {
      FsckFileInode failedFileInode;
      FsckChunk failedChunk;
      int errorCode;
      if (! this->db->insertFileInodes(fileInodes, failedFileInode, errorCode) )
         CPPUNIT_FAIL("Insertion of file inodes returned error");

      if (! this->db->insertChunks(chunks, failedChunk, errorCode) )
         CPPUNIT_FAIL("Insertion of primary chunks returned error");

      // now do the check
      if ( !this->db->checkForAndInsertChunksWithWrongPermissions() )
         CPPUNIT_FAIL("Check for and insert chunks with wrong permissions returned error");

      FsckChunkList differingChunks;
      DBCursor<FsckChunk>* cursor = this->db->getChunksWithWrongPermissions();

      FsckChunk* currentEntry = cursor->open();
      while ( currentEntry )
      {
         differingChunks.push_back(*currentEntry);
         currentEntry = cursor->step();
      }

      cursor->close();

      SAFE_DELETE(cursor);

      int64_t rowCount =
         this->db->getRowCount(TABLE_NAME_ERROR(FsckErrorCode_CHUNKWITHWRONGPERM));

      CPPUNIT_ASSERT(rowCount == DIFF_TOTAL);

      CPPUNIT_ASSERT(differingChunks.size() == DIFF_TOTAL);

   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }

   log.log(Log_DEBUG, "-testCheckForAndInsertChunksWithWrongPermissions - success");
}

void TestDatabase::testCheckForAndInsertChunksInWrongPath()
{
   log.log(Log_DEBUG, "-testCheckForAndInsertChunksInWrongPath - started");

   unsigned NUM_TARGETS = 2;
   unsigned NUM_CHUNKS_PER_TARGET = 5;
   unsigned NUM_WRONG_PATHS = 3;
   uint16_t nodeID = 1;

   FsckFileInodeList fileInodeList;
   FsckChunkList chunkList;

   CPPUNIT_ASSERT_MESSAGE("Programming error in test!", NUM_CHUNKS_PER_TARGET*NUM_TARGETS >
      NUM_WRONG_PATHS);

   for (unsigned i = 0; i < NUM_CHUNKS_PER_TARGET; i++)
   {
      std::string fileID = StorageTk::generateFileID(nodeID);
      std::string parentDirID = StorageTk::generateFileID(nodeID);

      // create a file inode for that
      unsigned origParentUID = i;
      PathInfo pathInfo(origParentUID, parentDirID, PATHINFO_FEATURE_ORIG);

      FsckFileInode fileInode = DatabaseTk::createDummyFsckFileInode();

      fileInode.id = fileID;
      fileInode.parentDirID = parentDirID;
      fileInode.parentNodeID = nodeID;
      fileInode.pathInfo = pathInfo;
      UInt16Vector stripeTargets;
      for (unsigned j = 1;j <= NUM_TARGETS; j++)
         stripeTargets.push_back(j);
      fileInode.stripeTargets = stripeTargets;

      fileInodeList.push_back(fileInode);

      // create chunks
      for (unsigned j = 1;j <= NUM_TARGETS; j++)
      {
         FsckChunk chunk = DatabaseTk::createDummyFsckChunk();
         chunk.id = fileID;
         chunk.targetID = j;
         PathVec chunkDirPath;
         std::string chunkFilePath; // will be ignored
         StorageTk::getChunkDirChunkFilePath(&pathInfo, fileID, true, chunkDirPath, chunkFilePath);

         chunk.savedPath = Path(chunkDirPath.getPathAsStr());

         chunkList.push_back(chunk);
      }
   }

   // take the first NUM_WRONG_PATHS in the chunkList and set them to something stupid
   FsckChunkListIter iter = chunkList.begin();
   for (unsigned i=0; i<NUM_WRONG_PATHS; i++)
   {
      CPPUNIT_ASSERT_MESSAGE("Programming error in test!", iter != chunkList.end());
      iter->savedPath = Path("/this/path/is/wrong");
      iter++;
   }

   try
   {
      FsckFileInode failedFileInode;
      FsckChunk failedChunk;
      int errorCode;
      if (! this->db->insertFileInodes(fileInodeList, failedFileInode, errorCode) )
         CPPUNIT_FAIL("Insertion of file inodes returned error");

      if (! this->db->insertChunks(chunkList, failedChunk, errorCode) )
         CPPUNIT_FAIL("Insertion of primary chunks returned error");

      // now do the check
      if ( !this->db->checkForAndInsertChunksInWrongPath() )
         CPPUNIT_FAIL("Check for and insert chunks in wrong path returned error");

      FsckChunkList differingChunks;
      DBCursor<FsckChunk>* cursor = this->db->getChunksInWrongPath();

      FsckChunk* currentEntry = cursor->open();
      while ( currentEntry )
      {
         differingChunks.push_back(*currentEntry);
         currentEntry = cursor->step();
      }

      cursor->close();

      SAFE_DELETE(cursor);

      int64_t rowCount =
         this->db->getRowCount(TABLE_NAME_ERROR(FsckErrorCode_CHUNKINWRONGPATH));

      CPPUNIT_ASSERT(rowCount == NUM_WRONG_PATHS);

      CPPUNIT_ASSERT(differingChunks.size() == NUM_WRONG_PATHS);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }

   log.log(Log_DEBUG, "-testCheckForAndInsertChunksInWrongPath - success");
}

void TestDatabase::testUpdateRepairActionDentries()
{
   log.log(Log_DEBUG, "-testUpdateRepairActionDentries - started");

   // test for each error code that is relevant for dentries
   {
      // create some dangling dentries
      this->testCheckForAndInsertDanglingDirEntries();

      // read them in
      FsckDirEntryList dentriesIn;
      this->db->getDanglingDirEntries(&dentriesIn);

      // update the repair action
      FsckErrorCode errorCode = FsckErrorCode_DANGLINGDENTRY;

      FsckRepairAction repairAction = FsckRepairAction_DELETEDENTRY;

      if ( this->db->setRepairAction(dentriesIn, errorCode, repairAction, NULL, NULL) > 0 )
      {
         CPPUNIT_FAIL( "Setting repair action for error code "
            + FsckTkEx::getErrorDesc(errorCode, true) + " failed.");
      }

      // get all entries from DB with given repairAction and see if they really match the input
      FsckDirEntryList dentriesOut;
      DBCursor<FsckDirEntry>* cursor = this->db->getDanglingDirEntries(repairAction);

      FsckDirEntry* element = cursor->open();

      while ( element != NULL )
      {
         dentriesOut.push_back(*element);
         element = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if ( ! FsckTkEx::compareFsckDirEntryLists(&dentriesIn, &dentriesOut) )
         CPPUNIT_FAIL("Generated and stored lists do not match");
   }

   // reset DB
   delete(this->db);

   try
   {
      this->db = new FsckDB(databasePath);
      this->db->init();
   }
   catch (FsckDBException &e)
   {
      CPPUNIT_FAIL("Recreation of DB failed");
   }

   {
      // create some dentries with wrong inode owner
      testCheckForAndInsertDirEntriesWithWrongOwner();

      // read them in
      FsckDirEntryList dentriesIn;
      this->db->getDirEntriesWithWrongOwner(&dentriesIn);

      // update the repair action
      FsckErrorCode errorCode = FsckErrorCode_WRONGOWNERINDENTRY;

      FsckRepairAction repairAction = FsckRepairAction_DELETEDENTRY;

      if ( this->db->setRepairAction(dentriesIn, errorCode, repairAction, NULL, NULL) > 0 )
      {
         CPPUNIT_FAIL( "Setting repair action for error code "
            + FsckTkEx::getErrorDesc(errorCode, true) + " failed.");
      }

      // get all entries from DB with given repairAction and see if the really math the input
      FsckDirEntryList dentriesOut;
      DBCursor<FsckDirEntry>* cursor = this->db->getDirEntriesWithWrongOwner(repairAction);

      FsckDirEntry* element = cursor->open();

      while ( element != NULL )
      {
         dentriesOut.push_back(*element);
         element = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if ( ! FsckTkEx::compareFsckDirEntryLists(&dentriesIn, &dentriesOut) )
         CPPUNIT_FAIL("Generated and stored lists do not match");
   }

   // reset DB
   delete(this->db);

   try
   {
      this->db = new FsckDB(databasePath);
      this->db->init();
   }
   catch (FsckDBException &e)
   {
      CPPUNIT_FAIL("Recreation of DB failed");
   }

   {
      // create some files with wrong target IDs
      this->testCheckForAndInsertFilesWithMissingStripeTargets();

      // read them in
      FsckDirEntryList dentriesIn;
      this->db->getFilesWithMissingStripeTargets(&dentriesIn);

      // update the repair action
      FsckErrorCode errorCode = FsckErrorCode_FILEWITHMISSINGTARGET;

      FsckRepairAction repairAction = FsckRepairAction_DELETEFILE;

      if ( this->db->setRepairAction(dentriesIn, errorCode, repairAction, NULL, NULL) > 0 )
      {
         CPPUNIT_FAIL( "Setting repair action for error code "
            + FsckTkEx::getErrorDesc(errorCode, true) + " failed.");
      }

      // get all entries from DB with given repairAction and see if they really match the input
      FsckDirEntryList dentriesOut;
      DBCursor<FsckDirEntry>* cursor = this->db->getFilesWithMissingStripeTargets(repairAction);

      FsckDirEntry* element = cursor->open();

      while ( element != NULL )
      {
         dentriesOut.push_back(*element);
         element = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if ( ! FsckTkEx::compareFsckDirEntryLists(&dentriesIn, &dentriesOut) )
         CPPUNIT_FAIL("Generated and stored lists do not match");
   }

   // reset DB
   delete(this->db);

   try
   {
      this->db = new FsckDB(databasePath);
      this->db->init();
   }
   catch (FsckDBException &e)
   {
      CPPUNIT_FAIL("Recreation of DB failed");
   }

   {
      // create some files with broke dentry-by-ID file
      this->testCheckForAndInsertMissingDentryByIDFile();

      // read them in
      FsckDirEntryList dentriesIn;
      this->db->getDirEntriesWithBrokenByIDFile(&dentriesIn);

      // update the repair action
      FsckErrorCode errorCode = FsckErrorCode_BROKENFSID;

      FsckRepairAction repairAction = FsckRepairAction_RECREATEFSID;

      if ( this->db->setRepairAction(dentriesIn, errorCode, repairAction, NULL, NULL) > 0 )
      {
         CPPUNIT_FAIL( "Setting repair action for error code "
            + FsckTkEx::getErrorDesc(errorCode, true) + " failed.");
      }

      // get all entries from DB with given repairAction and see if they really match the input
      FsckDirEntryList dentriesOut;
      DBCursor<FsckDirEntry>* cursor = this->db->getDirEntriesWithBrokenByIDFile(repairAction);

      FsckDirEntry* element = cursor->open();

      while ( element != NULL )
      {
         dentriesOut.push_back(*element);
         element = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if ( ! FsckTkEx::compareFsckDirEntryLists(&dentriesIn, &dentriesOut) )
         CPPUNIT_FAIL("Generated and stored lists do not match");
   }

   log.log(Log_DEBUG, "-testUpdateRepairActionDentries - success");
}

void TestDatabase::testUpdateRepairActionFileInodes()
{
   log.log(Log_DEBUG, "-testUpdateRepairActionFileInodes - started");

   // test for each error code that is relevant for file inodes
   {
      // create some orphaned file inodes
      this->testCheckForAndInsertOrphanedFileInodes();

      // read them in
      FsckFileInodeList inodesIn;
      this->db->getOrphanedFileInodes(&inodesIn);

      // update the repair action
      FsckErrorCode errorCode = FsckErrorCode_ORPHANEDFILEINODE;

      FsckRepairAction repairAction = FsckRepairAction_LOSTANDFOUND;

      if ( this->db->setRepairAction(inodesIn, errorCode, repairAction, NULL, NULL) > 0 )
      {
         CPPUNIT_FAIL( "Setting repair action for error code "
            + FsckTkEx::getErrorDesc(errorCode, true) + " failed.");
      }

      // get all entries from DB with given repairAction and see if the really math the input
      FsckFileInodeList inodesOut;
      DBCursor<FsckFileInode>* cursor = this->db->getOrphanedFileInodes(repairAction);

      FsckFileInode* element = cursor->open();

      while ( element != NULL )
      {
         inodesOut.push_back(*element);
         element = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if ( !FsckTkEx::compareFsckFileInodeLists(&inodesIn, &inodesOut) )
         CPPUNIT_FAIL("Generated and stored lists do not match");
   }

   // reset DB
   delete (this->db);

   try
   {
      this->db = new FsckDB(databasePath);
      this->db->init();
   }
   catch (FsckDBException &e)
   {
      CPPUNIT_FAIL("Recreation of DB failed");
   }

   {
      // create some inodes with wrong stat data
      this->testCheckForAndInsertFileInodesWithWrongAttribs();

      // read them in
      FsckFileInodeList inodesIn;
      this->db->getInodesWithWrongFileAttribs(&inodesIn);

      // update the repair action
      FsckErrorCode errorCode = FsckErrorCode_WRONGFILEATTRIBS;

      FsckRepairAction repairAction = FsckRepairAction_UPDATEATTRIBS;

      if ( this->db->setRepairAction(inodesIn, errorCode, repairAction, NULL, NULL) > 0 )
      {
         CPPUNIT_FAIL("Setting repair action for error code " +
            FsckTkEx::getErrorDesc(errorCode, true) + " failed.");
      }

      // get all entries from DB with given repairAction and see if the really math the input
      FsckFileInodeList inodesOut;
      DBCursor<FsckFileInode>* cursor = this->db->getInodesWithWrongFileAttribs(repairAction);

      FsckFileInode* element = cursor->open();

      while ( element != NULL )
      {
         inodesOut.push_back(*element);
         element = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if ( !FsckTkEx::compareFsckFileInodeLists(&inodesIn, &inodesOut) )
         CPPUNIT_FAIL("Generated and stored lists do not match");
   }

   log.log(Log_DEBUG, "-testUpdateRepairActionFileInodes - success");
}

void TestDatabase::testUpdateRepairActionDirInodes()
{
   log.log(Log_DEBUG, "-testUpdateRepairActionDirInodes - started");

   // test for each error code that is relevant for dir inodes
   {
      // create some orphaned dir inodes
      this->testCheckForAndInsertOrphanedDirInodes();

      // read them in
      FsckDirInodeList inodesIn;
      this->db->getOrphanedDirInodes(&inodesIn);

      // update the repair action
      FsckErrorCode errorCode = FsckErrorCode_ORPHANEDDIRINODE;

      FsckRepairAction repairAction = FsckRepairAction_LOSTANDFOUND;

      if ( this->db->setRepairAction(inodesIn, errorCode, repairAction, NULL, NULL) > 0 )
      {
         CPPUNIT_FAIL(
            "Setting repair action for error code " + FsckTkEx::getErrorDesc(errorCode, true) +
            " failed.");
      }

      // get all entries from DB with given repairAction and see if the really math the input
      FsckDirInodeList inodesOut;
      DBCursor<FsckDirInode>* cursor = this->db->getOrphanedDirInodes(repairAction);

      FsckDirInode* element = cursor->open();

      while ( element != NULL )
      {
         inodesOut.push_back(*element);
         element = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if ( !FsckTkEx::compareFsckDirInodeLists(&inodesIn, &inodesOut) )
         CPPUNIT_FAIL("Generated and stored lists do not match");
   }

   // reset DB
   delete (this->db);

   try
   {
      this->db = new FsckDB(databasePath);
      this->db->init();
   }
   catch (FsckDBException &e)
   {
      CPPUNIT_FAIL("Recreation of DB failed");
   }

   {
      // create some orphaned dir inodes
      this->testCheckForAndInsertInodesWithoutContDir();

      // read them in
      FsckDirInodeList inodesIn;
      this->db->getInodesWithoutContDir(&inodesIn);

      // update the repair action
      FsckErrorCode errorCode = FsckErrorCode_MISSINGCONTDIR;

      FsckRepairAction repairAction = FsckRepairAction_CREATECONTDIR;

      if ( this->db->setRepairAction(inodesIn, errorCode, repairAction, NULL, NULL) > 0 )
      {
         CPPUNIT_FAIL(
            "Setting repair action for error code " + FsckTkEx::getErrorDesc(errorCode, true) +
            " failed.");
      }

      // get all entries from DB with given repairAction and see if the really math the input
      FsckDirInodeList inodesOut;
      DBCursor<FsckDirInode>* cursor = this->db->getInodesWithoutContDir(repairAction);

      FsckDirInode* element = cursor->open();

      while ( element != NULL )
      {
         inodesOut.push_back(*element);
         element = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if ( !FsckTkEx::compareFsckDirInodeLists(&inodesIn, &inodesOut) )
         CPPUNIT_FAIL("Generated and stored lists do not match");
   }

   // reset DB
   delete (this->db);

   try
   {
      this->db = new FsckDB(databasePath);
      this->db->init();
   }
   catch (FsckDBException &e)
   {
      CPPUNIT_FAIL("Recreation of DB failed");
   }

   {
      // create some orphaned dir inodes
      this->testCheckForAndInsertInodesWithoutContDir();

      // read them in
      FsckDirInodeList inodesIn;
      this->db->getInodesWithWrongOwner(&inodesIn);

      // update the repair action
      FsckErrorCode errorCode = FsckErrorCode_WRONGINODEOWNER;

      FsckRepairAction repairAction = FsckRepairAction_CORRECTOWNER;

      if ( this->db->setRepairAction(inodesIn, errorCode, repairAction, NULL, NULL) > 0 )
      {
         CPPUNIT_FAIL(
            "Setting repair action for error code " + FsckTkEx::getErrorDesc(errorCode, true) +
            " failed.");
      }

      // get all entries from DB with given repairAction and see if the really math the input
      FsckDirInodeList inodesOut;
      DBCursor<FsckDirInode>* cursor = this->db->getInodesWithWrongOwner(repairAction);

      FsckDirInode* element = cursor->open();

      while ( element != NULL )
      {
         inodesOut.push_back(*element);
         element = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if ( !FsckTkEx::compareFsckDirInodeLists(&inodesIn, &inodesOut) )
         CPPUNIT_FAIL("Generated and stored lists do not match");
   }

   log.log(Log_DEBUG, "-testUpdateRepairActionDirInodes - success");
}

void TestDatabase::testUpdateRepairActionChunks()
{
   log.log(Log_DEBUG, "-testUpdateRepairActionChunks - started");

   // test for each error code that is relevant for chunks
   {
      // create some orphaned chunks
      this->testCheckForAndInsertOrphanedChunks();

      // read them in
      FsckChunkList chunksIn;
      this->db->getOrphanedChunks(&chunksIn);

      // update the repair action
      FsckErrorCode errorCode = FsckErrorCode_ORPHANEDCHUNK;

      FsckRepairAction repairAction = FsckRepairAction_DELETECHUNK;

      if ( this->db->setRepairAction(chunksIn, errorCode, repairAction, NULL, NULL) > 0 )
      {
         CPPUNIT_FAIL( "Setting repair action for error code "
            + FsckTkEx::getErrorDesc(errorCode, true) + " failed.");
      }

      // get all entries from DB with given repairAction and see if the really math the input
      FsckChunkList chunksOut;
      DBCursor<FsckChunk>* cursor = this->db->getOrphanedChunks(repairAction);

      FsckChunk* element = cursor->open();

      while ( element != NULL )
      {
         chunksOut.push_back(*element);
         element = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if ( ! FsckTkEx::compareFsckChunkLists(&chunksIn, &chunksOut) )
         CPPUNIT_FAIL("Generated and stored lists do not match");
   }

   log.log(Log_DEBUG, "-testUpdateRepairActionChunks - success");
}

void TestDatabase::testUpdateRepairActionContDirs()
{
   log.log(Log_DEBUG, "-testUpdateRepairActionContDirs - started");

   // test for each error code that is relevant for content directories
   {
      // create some orphaned content directories
      this->testCheckForAndInsertOrphanedContDirs();

      // read them in
      FsckContDirList contDirsIn;
      this->db->getOrphanedContDirs(&contDirsIn);

      // update the repair action
      FsckErrorCode errorCode = FsckErrorCode_ORPHANEDCONTDIR;

      FsckRepairAction repairAction = FsckRepairAction_DELETECONTDIR;

      if ( this->db->setRepairAction(contDirsIn, errorCode, repairAction, NULL, NULL) > 0 )
      {
         CPPUNIT_FAIL( "Setting repair action for error code "
            + FsckTkEx::getErrorDesc(errorCode, true) + " failed.");
      }

      // get all entries from DB with given repairAction and see if the really math the input
      FsckContDirList contDirsOut;
      DBCursor<FsckContDir>* cursor = this->db->getOrphanedContDirs(repairAction);

      FsckContDir* element = cursor->open();

      while ( element != NULL )
      {
         contDirsOut.push_back(*element);
         element = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if ( ! FsckTkEx::compareFsckContDirLists(&contDirsIn, &contDirsOut) )
         CPPUNIT_FAIL("Generated and stored lists do not match");
   }

   log.log(Log_DEBUG, "-testUpdateRepairActionContDirs - success");
}

void TestDatabase::testUpdateRepairActionFsIDs()
{
   log.log(Log_DEBUG, "-testUpdateRepairActionFsIDs - started");

   // test for each error code that is relevant for content directories
   {
      // create some missing dentries
      this->testCheckForAndInsertOrphanedDentryByIDFiles();

      // read them in
      FsckFsIDList fsIDsIn;
      this->db->getOrphanedDentryByIDFiles(&fsIDsIn);

      // update the repair action
      FsckErrorCode errorCode = FsckErrorCode_ORPHANEDFSID;

      FsckRepairAction repairAction = FsckRepairAction_RECREATEDENTRY;

      if ( this->db->setRepairAction(fsIDsIn, errorCode, repairAction, NULL, NULL) > 0 )
      {
         CPPUNIT_FAIL( "Setting repair action for error code "
            + FsckTkEx::getErrorDesc(errorCode, true) + " failed.");
      }

      // get all entries from DB with given repairAction and see if they really match the input
      FsckFsIDList fsIDsOut;
      DBCursor<FsckFsID>* cursor = this->db->getOrphanedDentryByIDFiles(repairAction);

      FsckFsID* element = cursor->open();

      while ( element != NULL )
      {
         fsIDsOut.push_back(*element);
         element = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if ( ! FsckTkEx::compareFsckFsIDLists(&fsIDsIn, &fsIDsOut) )
         CPPUNIT_FAIL("Generated and stored lists do not match");
   }

   log.log(Log_DEBUG, "-testUpdateRepairActionFsIDs - success");
}

void TestDatabase::testUpdateRepairActionTargetIDs()
{
   log.log(Log_DEBUG, "-testUpdateRepairActionTargetIDs - started");

   // test for each error code that is relevant for target IDs
   {
      // create some orphaned missing target IDs
      this->testCheckForAndInsertMissingStripeTargets();

      // read them in
      FsckTargetIDList targetsIn;
      this->db->getMissingStripeTargets(&targetsIn);

      // update the repair action
      FsckErrorCode errorCode = FsckErrorCode_MISSINGTARGET;

      FsckRepairAction repairAction = FsckRepairAction_NOTHING;

      if ( this->db->setRepairAction(targetsIn, errorCode, repairAction, NULL, NULL) > 0 )
      {
         CPPUNIT_FAIL( "Setting repair action for error code "
            + FsckTkEx::getErrorDesc(errorCode, true) + " failed.");
      }

      // get all entries from DB with given repairAction and see if the really math the input
      FsckTargetIDList targetsOut;
      DBCursor<FsckTargetID>* cursor = this->db->getMissingStripeTargets(repairAction);

      FsckTargetID* element = cursor->open();

      while ( element != NULL )
      {
         targetsOut.push_back(*element);
         element = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if ( ! ListTk::listsEqual(&targetsIn, &targetsOut) )
         CPPUNIT_FAIL("Generated and stored lists do not match");
   }

   log.log(Log_DEBUG, "-testUpdateRepairActionTargetIDs - success");
}


void TestDatabase::testDeleteDentries()
{
   log.log(Log_DEBUG, "-testDeleteDentries - started");

   unsigned NUM_ELEMENTS = 10;
   unsigned NUM_DELETES = 4;

   // create some dentries
   FsckDirEntryList dentries;
   DatabaseTk::createDummyFsckDirEntries(NUM_ELEMENTS, &dentries);

   // insert them into the DB
   FsckDirEntry failedDentry;
   int errorCode;
   if (! this->db->insertDirEntries(dentries, failedDentry, errorCode) )
      CPPUNIT_FAIL("Insert returned error");

   // take the first NUM_DELETES and delete them in DB
   FsckDirEntryList dentriesDelete;
   FsckDirEntryListIter dentriesIter = dentries.begin();

   for (unsigned i = 0; i < NUM_DELETES; i++)
   {
      if (dentriesIter != dentries.end())
      {
         dentriesDelete.push_back(*dentriesIter);
         dentriesIter++;
      }
   }

   if (! this->db->deleteDirEntries(dentriesDelete, failedDentry, errorCode) )
      CPPUNIT_FAIL("Deleting dir entries failed");

   CPPUNIT_ASSERT(this->db->countDirEntries() == int64_t(NUM_ELEMENTS - NUM_DELETES));

   log.log(Log_DEBUG, "-testDeleteDentries - success");
}

void TestDatabase::testDeleteChunks()
{
   log.log(Log_DEBUG, "-testDeleteChunks - started");

   unsigned NUM_ELEMENTS = 10;
   unsigned NUM_DELETES = 4;

   // create some dentries
   FsckChunkList chunks;
   DatabaseTk::createDummyFsckChunks(NUM_ELEMENTS, &chunks);

   //insert them into the DB
   FsckChunk failedChunk;
   int errorCode;
   if (! this->db->insertChunks(chunks, failedChunk, errorCode) )
      CPPUNIT_FAIL("Insert returned error");

   // take the first NUM_DELETES and delete them in DB
   FsckChunkList chunksDelete;
   FsckChunkListIter chunksIter = chunks.begin();

   for (unsigned i = 0; i < NUM_DELETES; i++)
   {
      if (chunksIter != chunks.end())
      {
         chunksDelete.push_back(*chunksIter);
         chunksIter++;
      }
   }

   if (! this->db->deleteChunks(chunksDelete, failedChunk, errorCode))
      CPPUNIT_FAIL("Deleting chunks failed");

   CPPUNIT_ASSERT(this->db->countChunks() == int64_t(NUM_ELEMENTS - NUM_DELETES));

   log.log(Log_DEBUG, "-testDeleteChunks - success");
}

void TestDatabase::testDeleteFsIDs()
{
   log.log(Log_DEBUG, "-testDeleteFsIDs - started");

   unsigned NUM_ELEMENTS = 10;
   unsigned NUM_DELETES = 4;

   // create some fsIDs
   FsckFsIDList fsIDs;
   DatabaseTk::createDummyFsckFsIDs(NUM_ELEMENTS, &fsIDs);

   //insert them into the DB
   FsckFsID failedFsID;
   int errorCode;
   if (! this->db->insertFsIDs(fsIDs, failedFsID, errorCode) )
      CPPUNIT_FAIL("Insert returned error");

   // take the first NUM_DELETES and delete them in DB
   FsckFsIDList elemsDelete;
   FsckFsIDListIter iter = fsIDs.begin();

   for (unsigned i = 0; i < NUM_DELETES; i++)
   {
      if (iter != fsIDs.end())
      {
         elemsDelete.push_back(*iter);
         iter++;
      }
   }

   if (! this->db->deleteFsIDs(elemsDelete, failedFsID, errorCode) )
      CPPUNIT_FAIL("Deleting FS-IDs failed");

   CPPUNIT_ASSERT(this->db->countFsIDs() == int64_t(NUM_ELEMENTS - NUM_DELETES));

   log.log(Log_DEBUG, "-testDeleteFsIDs - success");
}

void TestDatabase::testDeleteDanglingDentries()
{
   log.log(Log_DEBUG, "-testDeleteDanglingDentries - started");

   unsigned NUM_DELETES = 2;

   // create some dangling dentries
   this->testCheckForAndInsertDanglingDirEntries();

   // read them in
   FsckDirEntryList dentries;
   this->db->getDanglingDirEntries(&dentries);

   size_t numDentries = dentries.size();

   // take the first NUM_DELETES and delete them in DB
   FsckDirEntryList dentriesDelete;
   FsckDirEntryListIter dentriesIter = dentries.begin();

   for (unsigned i = 0; i < NUM_DELETES; i++)
   {
      if (dentriesIter != dentries.end())
      {
         dentriesDelete.push_back(*dentriesIter);
         dentriesIter++;
      }
   }

   FsckDirEntry failedDelete;
   int errorCode;
   if (!this->db->deleteDanglingDirEntries(dentriesDelete, failedDelete, errorCode))
      CPPUNIT_FAIL("Deleting dangling dir entries failed");

   CPPUNIT_ASSERT(this->db->countErrors(FsckErrorCode_DANGLINGDENTRY) ==
      int64_t(numDentries - NUM_DELETES));

   log.log(Log_DEBUG, "-testDeleteDanglingDentries - success");
}

void TestDatabase::testDeleteDentriesWithWrongOwner()
{
   log.log(Log_DEBUG, "-testDeleteDentriesWithWrongOwner - started");

   unsigned NUM_DELETES = 2;

   // create some dangling inodes
   this->testCheckForAndInsertDirEntriesWithWrongOwner();

   // read them in
   FsckDirEntryList dentries;
   this->db->getDirEntriesWithWrongOwner(&dentries);

   size_t numDentries = dentries.size();

   // take the first NUM_DELETES and delete them in DB
   FsckDirEntryList dentriesDelete;
   FsckDirEntryListIter dentriesIter = dentries.begin();

   for (unsigned i = 0; i < NUM_DELETES; i++)
   {
      if (dentriesIter != dentries.end())
      {
         dentriesDelete.push_back(*dentriesIter);
         dentriesIter++;
      }
   }

   FsckDirEntry failedDelete;
   int errorCode;
   if (! this->db->deleteDirEntriesWithWrongOwner(dentriesDelete, failedDelete, errorCode))
      CPPUNIT_FAIL("Deleting dentries with wrong owner failed");

   CPPUNIT_ASSERT(this->db->countErrors(FsckErrorCode_WRONGOWNERINDENTRY) ==
      int64_t(numDentries - NUM_DELETES));

   log.log(Log_DEBUG, "-testDeleteDentriesWithWrongOwner - success");
}

void TestDatabase::testDeleteInodesWithWrongOwner()
{
   log.log(Log_DEBUG, "-testDeleteInodesWithWrongOwner - started");

   unsigned NUM_DELETES = 2;

   this->testCheckForAndInsertInodesWithWrongOwner();

   // read them in
   FsckDirInodeList inodes;
   this->db->getInodesWithWrongOwner(&inodes);

   size_t numInodes = inodes.size();

   // take the first NUM_DELETES and delete them in DB
   FsckDirInodeList inodesDelete;
   FsckDirInodeListIter inodesIter = inodes.begin();

   for (unsigned i = 0; i < NUM_DELETES; i++)
   {
      if (inodesIter != inodes.end())
      {
         inodesDelete.push_back(*inodesIter);
         inodesIter++;
      }
   }

   FsckDirInode failedInode;
   int errorCode;
   if (! this->db->deleteInodesWithWrongOwner(inodesDelete, failedInode, errorCode))
      CPPUNIT_FAIL("Deleting inodes with wrong owner failed");

   CPPUNIT_ASSERT(this->db->countErrors(FsckErrorCode_WRONGINODEOWNER) ==
      int64_t(numInodes - NUM_DELETES));

   log.log(Log_DEBUG, "-testDeleteInodesWithWrongOwner - success");
}

void TestDatabase::testDeleteOrphanedDirInodes()
{
   log.log(Log_DEBUG, "-testDeleteOrphanedDirInodes - started");

   unsigned NUM_DELETES = 2;

   this->testCheckForAndInsertOrphanedDirInodes();

   // read them in
   FsckDirInodeList inodes;
   this->db->getOrphanedDirInodes(&inodes);

   size_t numInodes = inodes.size();

   // take the first NUM_DELETES and delete them in DB
   FsckDirInodeList inodesDelete;
   FsckDirInodeListIter inodesIter = inodes.begin();

   for (unsigned i = 0; i < NUM_DELETES; i++)
   {
      if (inodesIter != inodes.end())
      {
         inodesDelete.push_back(*inodesIter);
         inodesIter++;
      }
   }

   FsckDirInode failedInode;
   int errorCode;
   if (! this->db->deleteOrphanedInodes(inodesDelete, failedInode, errorCode))
      CPPUNIT_FAIL("Deleting orphaned dir inodes");

   CPPUNIT_ASSERT(this->db->countErrors(FsckErrorCode_ORPHANEDDIRINODE) ==
      int64_t(numInodes - NUM_DELETES));

   log.log(Log_DEBUG, "-testDeleteOrphanedDirInodes - success");
}

void TestDatabase::testDeleteOrphanedFileInodes()
{
   log.log(Log_DEBUG, "-testDeleteOrphanedFileInodes - started");

   unsigned NUM_DELETES = 2;

   this->testCheckForAndInsertOrphanedFileInodes();

   // read them in
   FsckFileInodeList inodes;
   this->db->getOrphanedFileInodes(&inodes);

   size_t numInodes = inodes.size();

   // take the first NUM_DELETES and delete them in DB
   FsckFileInodeList inodesDelete;
   FsckFileInodeListIter inodesIter = inodes.begin();

   for (unsigned i = 0; i < NUM_DELETES; i++)
   {
      if (inodesIter != inodes.end())
      {
         inodesDelete.push_back(*inodesIter);
         inodesIter++;
      }
   }

   FsckFileInode failedInode;
   int errorCode;
   if (! this->db->deleteOrphanedInodes(inodesDelete, failedInode, errorCode))
      CPPUNIT_FAIL("Deleting orphaned file inodes");

   CPPUNIT_ASSERT(this->db->countErrors(FsckErrorCode_ORPHANEDFILEINODE) ==
      int64_t(numInodes - NUM_DELETES));

   log.log(Log_DEBUG, "-testDeleteOrphanedFileInodes - success");
}

void TestDatabase::testDeleteOrphanedChunks()
{
   log.log(Log_DEBUG, "-testDeleteOrphanedChunks - started");

   unsigned NUM_DELETES = 4;

   this->testCheckForAndInsertOrphanedChunks();

   // read them in
   FsckChunkList chunks;
   this->db->getOrphanedChunks(&chunks);

   size_t numChunks = chunks.size();

   // take the first NUM_DELETES and delete them in DB
   FsckChunkList chunksDelete;
   FsckChunkListIter chunksIter = chunks.begin();

   for (unsigned i = 0; i < NUM_DELETES; i++)
   {
      if (chunksIter != chunks.end())
      {
         chunksDelete.push_back(*chunksIter);
         chunksIter++;
      }
   }

   FsckChunk failedEntry;
   int errorCode;
   if (! this->db->deleteOrphanedChunks(chunksDelete, failedEntry, errorCode))
      CPPUNIT_FAIL("Deleting orphaned chunks");

   CPPUNIT_ASSERT(this->db->countErrors(FsckErrorCode_ORPHANEDCHUNK) ==
      int64_t(numChunks - NUM_DELETES));

   log.log(Log_DEBUG, "-testDeleteOrphanedChunks - success");
}

void TestDatabase::testDeleteMissingContDir()
{
   log.log(Log_DEBUG, "-testDeleteMissingContDir - started");

   unsigned NUM_DELETES = 2;

   this->testCheckForAndInsertInodesWithoutContDir();

   // read them in
   FsckDirInodeList inodes;
   this->db->getInodesWithoutContDir(&inodes);

   size_t numInodes = inodes.size();

   // take the first NUM_DELETES and delete them in DB
   FsckDirInodeList inodesDelete;
   FsckDirInodeListIter inodesIter = inodes.begin();

   for ( unsigned i = 0; i < NUM_DELETES; i++ )
   {
      if ( inodesIter != inodes.end() )
      {
         inodesDelete.push_back(*inodesIter);
         inodesIter++;
      }
   }

   FsckDirInode failedEntry;
   int errorCode;
   if (! this->db->deleteMissingContDirs(inodesDelete, failedEntry, errorCode) )
      CPPUNIT_FAIL("Deleting dir inodes without .cont dir");

   CPPUNIT_ASSERT(
      this->db->countErrors(FsckErrorCode_MISSINGCONTDIR) == int64_t(numInodes - NUM_DELETES));

   log.log(Log_DEBUG, "-testDeleteMissingContDir - success");
}

void TestDatabase::testDeleteOrphanedContDir()
{
   log.log(Log_DEBUG, "-testDeleteOrphanedContDir - started");

   unsigned NUM_DELETES = 2;

   this->testCheckForAndInsertOrphanedContDirs();

   // read them in
   FsckContDirList contDirs;
   this->db->getOrphanedContDirs(&contDirs);

   size_t numContDirs = contDirs.size();

   // take the first NUM_DELETES and delete them in DB
   FsckContDirList contDirsDelete;
   FsckContDirListIter contDirsIter = contDirs.begin();

   for ( unsigned i = 0; i < NUM_DELETES; i++ )
   {
      if ( contDirsIter != contDirs.end() )
      {
         contDirsDelete.push_back(*contDirsIter);
         contDirsIter++;
      }
   }

   FsckContDir failedEntry;
   int errorCode;
   if (! this->db->deleteOrphanedContDirs(contDirsDelete, failedEntry, errorCode) )
      CPPUNIT_FAIL("Deleting orphaned cont dirs failed");

   CPPUNIT_ASSERT(
      this->db->countErrors(FsckErrorCode_ORPHANEDCONTDIR) == int64_t(numContDirs - NUM_DELETES));

   log.log(Log_DEBUG, "-testDeleteOrphanedContDir - success");
}

void TestDatabase::testDeleteFileInodesWithWrongAttribs()
{
   log.log(Log_DEBUG, "-testDeleteFileInodesWithWrongAttribs - started");

   unsigned NUM_DELETES = 2;

   // create some file inodes with wrong data
   this->testCheckForAndInsertFileInodesWithWrongAttribs();

   // read them in
   FsckFileInodeList inodes;
   this->db->getInodesWithWrongFileAttribs(&inodes);

   size_t numInodes = inodes.size();

   // take the first NUM_DELETES and delete them in DB
   FsckFileInodeList inodesDelete;
   FsckFileInodeListIter inodesIter = inodes.begin();

   for ( unsigned i = 0; i < NUM_DELETES; i++ )
   {
      if ( inodesIter != inodes.end() )
      {
         inodesDelete.push_back(*inodesIter);
         inodesIter++;
      }
   }

   FsckFileInode failedEntry;
   int errorCode;
   if (! this->db->deleteInodesWithWrongAttribs(inodesDelete, failedEntry, errorCode) )
      CPPUNIT_FAIL("Deleting file inodes with wrong attributes failed");

   CPPUNIT_ASSERT(
      this->db->countErrors(FsckErrorCode_WRONGFILEATTRIBS) == int64_t(numInodes - NUM_DELETES));

   log.log(Log_DEBUG, "-testDeleteFileInodesWithWrongAttribs - success");
}

void TestDatabase::testDeleteDirInodesWithWrongAttribs()
{
   log.log(Log_DEBUG, "-testDeleteDirInodesWithWrongAttribs - started");

   unsigned NUM_DELETES = 2;

   // create some file inodes with wrong data
   this->testCheckForAndInsertDirInodesWithWrongAttribs();

   // read them in
   FsckDirInodeList inodes;
   this->db->getInodesWithWrongDirAttribs(&inodes);

   size_t numInodes = inodes.size();

   // take the first NUM_DELETES and delete them in DB
   FsckDirInodeList inodesDelete;
   FsckDirInodeListIter inodesIter = inodes.begin();

   for ( unsigned i = 0; i < NUM_DELETES; i++ )
   {
      if ( inodesIter != inodes.end() )
      {
         inodesDelete.push_back(*inodesIter);
         inodesIter++;
      }
   }

   FsckDirInode failedEntry;
   int errorCode;
   if (! this->db->deleteInodesWithWrongAttribs(inodesDelete, failedEntry, errorCode) )
      CPPUNIT_FAIL("Deleting dir inodes with wrong attributes failed");

   CPPUNIT_ASSERT(
      this->db->countErrors(FsckErrorCode_WRONGDIRATTRIBS) == int64_t(numInodes - NUM_DELETES));

   log.log(Log_DEBUG, "-testDeleteDirInodesWithWrongAttribs - success");
}

void TestDatabase::testDeleteMissingStripeTargets()
{
   log.log(Log_DEBUG, "-testDeleteMissingStripeTargets - started");

   unsigned NUM_DELETES = 2;

   // create some file inodes with wrong data
   this->testCheckForAndInsertMissingStripeTargets();

   // read them in
   FsckTargetIDList targetIDs;
   this->db->getMissingStripeTargets(&targetIDs);

   size_t numTargets = targetIDs.size();

   // take the first NUM_DELETES and delete them in DB
   FsckTargetIDList targetsDelete;
   FsckTargetIDListIter targetsIter = targetIDs.begin();

   for ( unsigned i = 0; i < NUM_DELETES; i++ )
   {
      if ( targetsIter != targetIDs.end() )
      {
         targetsDelete.push_back(*targetsIter);
         targetsIter++;
      }
   }

   FsckTargetID failedEntry;
   int errorCode;
   if (! this->db->deleteMissingStripeTargets(targetsDelete, failedEntry, errorCode) )
      CPPUNIT_FAIL("Deleting missing stripe targets failed");

   CPPUNIT_ASSERT(
      this->db->countErrors(FsckErrorCode_MISSINGTARGET) == int64_t(numTargets - NUM_DELETES));

   log.log(Log_DEBUG, "-testDeleteMissingStripeTargets - success");
}

void TestDatabase::testDeleteFilesWithMissingStripeTargets()
{
   log.log(Log_DEBUG, "-testDeleteFilesWithMissingStripeTargets - started");

   unsigned NUM_DELETES = 2;

   // create some file inodes with wrong data
   this->testCheckForAndInsertFilesWithMissingStripeTargets();

   // read them in
   FsckDirEntryList files;
   this->db->getFilesWithMissingStripeTargets(&files);

   size_t numFiles = files.size();

   // take the first NUM_DELETES and delete them in DB
   FsckDirEntryList filesDelete;
   FsckDirEntryListIter filesIter = files.begin();

   for ( unsigned i = 0; i < NUM_DELETES; i++ )
   {
      if ( filesIter != files.end() )
      {
         filesDelete.push_back(*filesIter);
         filesIter++;
      }
   }

   FsckDirEntry failedDelete;
   int errorCode;
   if (! this->db->deleteFilesWithMissingStripeTargets(filesDelete, failedDelete, errorCode))
      CPPUNIT_FAIL("Deleting files with missing stripe targets failed");

   CPPUNIT_ASSERT(
      this->db->countErrors(FsckErrorCode_FILEWITHMISSINGTARGET) ==
         int64_t(numFiles - NUM_DELETES));

   log.log(Log_DEBUG, "-testDeleteFilesWithMissingStripeTargets - success");
}

void TestDatabase::testDeleteChunksWithWrongPermissions()
{
   log.log(Log_DEBUG, "-testDeleteChunksWithWrongPermissions - started");

   unsigned NUM_DELETES = 2;

   // create some chunks with wrong data
   this->testCheckForAndInsertChunksWithWrongPermissions();

   // read them in
   FsckChunkList chunks;
   this->db->getChunksWithWrongPermissions(&chunks);

   size_t numChunks = chunks.size();

   // take the first NUM_DELETES and delete them in DB
   FsckChunkList chunksDelete;
   FsckChunkListIter chunksIter = chunks.begin();

   for ( unsigned i = 0; i < NUM_DELETES; i++ )
   {
      if ( chunksIter != chunks.end() )
      {
         chunksDelete.push_back(*chunksIter);
         chunksIter++;
      }
   }

   FsckChunk failedEntry;
   int errorCode;
   if (! this->db->deleteChunksWithWrongPermissions(chunksDelete, failedEntry, errorCode))
      CPPUNIT_FAIL("Deleting chunks with wrong permissions failed");

   CPPUNIT_ASSERT(
      this->db->countErrors(FsckErrorCode_CHUNKWITHWRONGPERM) ==
         int64_t(numChunks - NUM_DELETES));

   log.log(Log_DEBUG, "-testDeleteChunksWithWrongPermissions - success");
}

void TestDatabase::testReplaceStripeTarget()
{
   log.log(Log_DEBUG, "-testReplaceStripeTarget - started");

   unsigned NUM_INODES = 10;

   FsckFileInodeList fileInodeListIn;
   FsckFileInodeList fileInodeListOut;

   DatabaseTk::createDummyFsckFileInodes(NUM_INODES, &fileInodeListIn);

   try
   {
      // set defined stripe pattern for inodes
      UInt16Vector stripeTargets;
      stripeTargets.push_back(1);
      stripeTargets.push_back(2);
      stripeTargets.push_back(3);
      stripeTargets.push_back(4);

      UInt16Vector expectedStripeTargets;
      expectedStripeTargets.push_back(1);
      expectedStripeTargets.push_back(20);
      expectedStripeTargets.push_back(3);
      expectedStripeTargets.push_back(4);
      for (FsckFileInodeListIter iter = fileInodeListIn.begin(); iter != fileInodeListIn.end();
         iter++)
      {
         iter->setStripeTargets(stripeTargets);
      }

      // insert the inodes
      FsckFileInode failedInsert;
      int errorCode;
      if (! this->db->insertFileInodes(fileInodeListIn, failedInsert, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // change the stripe targets
      bool replaceRes = this->db->replaceStripeTarget(2,20,&fileInodeListIn,
         this->db->getHandlePool()->acquireHandle(true));

      CPPUNIT_ASSERT_MESSAGE("Target replacement failed", replaceRes);

      // read into the outList
      this->db->getFileInodes(&fileInodeListOut);

      for (FsckFileInodeListIter iter = fileInodeListOut.begin(); iter != fileInodeListOut.end();
         iter++)
      {
         UInt16Vector* savedStripeTargets = iter->getStripeTargets();

         CPPUNIT_ASSERT_MESSAGE("Comparision of stripe targets failed",
            *savedStripeTargets == expectedStripeTargets);
      }
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }

   log.log(Log_DEBUG, "-testReplaceStripeTarget - success");
}

void TestDatabase::testBasicDBCursor()
{
   unsigned NUM_ROWS = 5;
   bool compareRes;

   DBHandlePool* hPool = this->db->getHandlePool();

   {
      // Dentries
      FsckDirEntryList listIn;
      FsckDirEntryList listOut;

      DatabaseTk::createDummyFsckDirEntries(NUM_ROWS, &listIn, FsckDirEntryType_DIRECTORY);

      FsckDirEntry failedDentry;
      int errorCode;
      if (! this->db->insertDirEntries(listIn, failedDentry, errorCode) )
         CPPUNIT_FAIL("Insertion of file dentries returned error");

      DBCursor<FsckDirEntry> cursor(hPool, "SELECT * FROM dirEntries");

      FsckDirEntry* currentObject = cursor.open();

      while ( currentObject )
      {
         listOut.push_back(*currentObject);
         currentObject = cursor.step();
      }

      cursor.close();

      compareRes = FsckTkEx::compareFsckDirEntryLists(&listIn, &listOut);
      CPPUNIT_ASSERT_MESSAGE("Comparision of dirEntries failed", compareRes);

   }

   {
      // FileInodes
      FsckFileInodeList listIn;
      FsckFileInodeList listOut;

      DatabaseTk::createDummyFsckFileInodes(NUM_ROWS, &listIn);

      FsckFileInode failedInode;
      int errorCode;
      if (! this->db->insertFileInodes(listIn, failedInode, errorCode) )
         CPPUNIT_FAIL("Insertion of file file inodes returned error");

      DBCursor<FsckFileInode> cursor(hPool, "SELECT * FROM fileInodes");

      FsckFileInode* currentObject = cursor.open();

      while ( currentObject )
      {
         listOut.push_back(*currentObject);
         currentObject = cursor.step();
      }

      cursor.close();

      compareRes = FsckTkEx::compareFsckFileInodeLists(&listIn, &listOut);
      CPPUNIT_ASSERT_MESSAGE("Comparision of file inodes failed", compareRes);

   }

   {
      // DirInodes
      FsckDirInodeList listIn;
      FsckDirInodeList listOut;

      DatabaseTk::createDummyFsckDirInodes(NUM_ROWS, &listIn);

      FsckDirInode failedInode;
      int errorCode;
      if (! this->db->insertDirInodes(listIn, failedInode, errorCode) )
         CPPUNIT_FAIL("Insertion of dir inodes returned error");

      DBCursor<FsckDirInode> cursor(hPool, "SELECT * FROM dirInodes");

      FsckDirInode* currentObject = cursor.open();

      while ( currentObject )
      {
         listOut.push_back(*currentObject);
         currentObject = cursor.step();
      }

      cursor.close();

      compareRes = FsckTkEx::compareFsckDirInodeLists(&listIn, &listOut);
      CPPUNIT_ASSERT_MESSAGE("Comparision of dir inodes failed", compareRes);
   }

   {
      // chunks
      FsckChunkList listIn;
      FsckChunkList listOut;

      DatabaseTk::createDummyFsckChunks(NUM_ROWS, &listIn);

      FsckChunk failedChunk;
      int errorCode;
      if (! this->db->insertChunks(listIn, failedChunk, errorCode) )
         CPPUNIT_FAIL("Insertion of chunks returned error");

      DBCursor<FsckChunk> cursor(hPool, "SELECT * FROM chunks");

      FsckChunk* currentObject = cursor.open();

      while ( currentObject )
      {
         listOut.push_back(*currentObject);
         currentObject = cursor.step();
      }

      cursor.close();

      compareRes = FsckTkEx::compareFsckChunkLists(&listIn, &listOut);
      CPPUNIT_ASSERT_MESSAGE("Comparision of chunks failed", compareRes);
   }

   {
      // chunks
      FsckContDirList listIn;
      FsckContDirList listOut;

      DatabaseTk::createDummyFsckContDirs(NUM_ROWS, &listIn);

      FsckContDir failedContDir;
      int errorCode;
      if (! this->db->insertContDirs(listIn, failedContDir, errorCode) )
         CPPUNIT_FAIL("Insertion of chunks returned error");

      DBCursor<FsckContDir> cursor(hPool, "SELECT * FROM contDirs");

      FsckContDir* currentObject = cursor.open();

      while ( currentObject )
      {
         listOut.push_back(*currentObject);
         currentObject = cursor.step();
      }

      cursor.close();

      compareRes = FsckTkEx::compareFsckContDirLists(&listIn, &listOut);
      CPPUNIT_ASSERT_MESSAGE("Comparision of cont dirs failed", compareRes);
   }
}

void TestDatabase::testGetRowCount()
{
   // test it by using dentries
   unsigned NUM_ROWS = 5;
   std::string tableName = TABLE_NAME_DIR_ENTRIES;

   FsckDirEntryList dentryList;

   DatabaseTk::createDummyFsckDirEntries(NUM_ROWS, &dentryList);

   try
   {
      // insert the dentries
      FsckDirEntry failedDentry;
      int errorCode;
      if (! this->db->insertDirEntries(dentryList, failedDentry, errorCode) )
         CPPUNIT_FAIL("Insert returned error");

      // get the row count
      int64_t rowCount = this->db->getRowCount(tableName);

      // check row count
      CPPUNIT_ASSERT( rowCount == NUM_ROWS);
   }
   catch (FsckDBException &e) //explicitely catch exception to get error message
   {
      std::string errorMsg = std::string("FsckDBException thrown: ") + std::string(e.what());
      CPPUNIT_FAIL(errorMsg);
   }
}

void TestDatabase::testGetFullPath()
{
   // first test => PASS OK
   FsckDirEntryList dirEntries;

   FsckDirEntry fileDirEntry = DatabaseTk::createDummyFsckDirEntry();
   fileDirEntry.setName("file");
   fileDirEntry.setID("fileID");
   fileDirEntry.setParentDirID(META_ROOTDIR_ID_STR);
   dirEntries.push_back(fileDirEntry);

   FsckDirEntry failedDentry;
   int errorCode;
   if (! this->db->insertDirEntries(dirEntries, failedDentry, errorCode) )
      CPPUNIT_FAIL("Insertion of file dentries returned error");

   std::string outPath;
   bool retVal = DatabaseTk::getFullPath(this->db, &fileDirEntry, outPath);

   CPPUNIT_ASSERT( retVal == true );
   CPPUNIT_ASSERT( outPath == "/file" );

   // same test with ID
   outPath = "";
   retVal = DatabaseTk::getFullPath(this->db, fileDirEntry.getID(), outPath);

   CPPUNIT_ASSERT( retVal == true);
   CPPUNIT_ASSERT( outPath == "/file");

   // second test => PASS OK
   if (! this->db->deleteDirEntries(dirEntries, failedDentry, errorCode) )
      CPPUNIT_FAIL("Deletion of file dentries returned error");

   dirEntries.clear();

   fileDirEntry.setName("file");
   fileDirEntry.setID("fileID");
   fileDirEntry.setParentDirID("id_D");
   dirEntries.push_back(fileDirEntry);

   FsckDirEntry dirEntry = DatabaseTk::createDummyFsckDirEntry(FsckDirEntryType_DIRECTORY);

   dirEntry.setName("D");
   dirEntry.setID("id_D");
   dirEntry.setParentDirID("id_C");
   dirEntries.push_back(dirEntry);

   dirEntry.setName("C");
   dirEntry.setID("id_C");
   dirEntry.setParentDirID("id_B");
   dirEntries.push_back(dirEntry);

   dirEntry.setName("B");
   dirEntry.setID("id_B");
   dirEntry.setParentDirID("id_A");
   dirEntries.push_back(dirEntry);

   dirEntry.setName("A");
   dirEntry.setID("id_A");
   dirEntry.setParentDirID(META_ROOTDIR_ID_STR);
   dirEntries.push_back(dirEntry);

   if (! this->db->insertDirEntries(dirEntries, failedDentry, errorCode) )
      CPPUNIT_FAIL("Insertion of file dentries returned error");

   outPath = "";
   retVal = DatabaseTk::getFullPath(this->db, &fileDirEntry, outPath);

   CPPUNIT_ASSERT( retVal == true );
   CPPUNIT_ASSERT( outPath == "/A/B/C/D/file" );

   // same test with ID
   outPath = "";
   retVal = DatabaseTk::getFullPath(this->db, fileDirEntry.getID(), outPath);

   CPPUNIT_ASSERT( retVal == true);
   CPPUNIT_ASSERT( outPath == "/A/B/C/D/file");

   //==========//
   // third test => create a path which could not be fully resolved
   //=========//
   if (! this->db->deleteDirEntries(dirEntries, failedDentry, errorCode) )
      CPPUNIT_FAIL("Deletion of file dentries returned error");

   dirEntries.clear();

   dirEntries.push_back(fileDirEntry);

   dirEntry.setName("D");
   dirEntry.setID("id_D");
   dirEntry.setParentDirID("id_C");
   dirEntries.push_back(dirEntry);

   dirEntry.setName("C");
   dirEntry.setID("id_C");
   dirEntry.setParentDirID("id_B");
   dirEntries.push_back(dirEntry);

   dirEntry.setName("A");
   dirEntry.setID("id_A");
   dirEntry.setParentDirID(META_ROOTDIR_ID_STR);
   dirEntries.push_back(dirEntry);

   if (! this->db->insertDirEntries(dirEntries, failedDentry, errorCode) )
      CPPUNIT_FAIL("Insertion of file dentries returned error");

   outPath = "";
   retVal = DatabaseTk::getFullPath(this->db, &fileDirEntry, outPath);

   CPPUNIT_ASSERT( retVal == false );
   CPPUNIT_ASSERT( outPath == "[<unresolved>]/C/D/file" );

   // same test with ID
   outPath = "";
   retVal = DatabaseTk::getFullPath(this->db, fileDirEntry.getID(), outPath);

   CPPUNIT_ASSERT( retVal == false );
   CPPUNIT_ASSERT( outPath == "[<unresolved>]/C/D/file" );

   //==========//
   // fourth test => create a loop
   //=========//
   if (! this->db->deleteDirEntries(dirEntries, failedDentry, errorCode) )
      CPPUNIT_FAIL("Deletion of file dentries returned error");

   dirEntries.clear();

   dirEntries.push_back(fileDirEntry);

   dirEntry.setName("D");
   dirEntry.setID("id_D");
   dirEntry.setParentDirID("id_C");
   dirEntries.push_back(dirEntry);

   dirEntry.setName("C");
   dirEntry.setID("id_C");
   dirEntry.setParentDirID("id_B");
   dirEntries.push_back(dirEntry);

   dirEntry.setName("B");
   dirEntry.setID("id_B");
   dirEntry.setParentDirID("id_A");
   dirEntries.push_back(dirEntry);

   dirEntry.setName("A");
   dirEntry.setID("id_A");
   dirEntry.setParentDirID("id_D");
   dirEntries.push_back(dirEntry);

   if (! this->db->insertDirEntries(dirEntries, failedDentry, errorCode) )
      CPPUNIT_FAIL("Insertion of file dentries returned error");

   outPath = "";
   retVal = DatabaseTk::getFullPath(this->db, &fileDirEntry, outPath);

   CPPUNIT_ASSERT( retVal == false );

   // same test with ID
   outPath = "";
   retVal = DatabaseTk::getFullPath(this->db, fileDirEntry.getID(), outPath);

   CPPUNIT_ASSERT( retVal == false );
}


#include "TestFsckTk.h"

#include <common/toolkit/FsckTk.h>
#include <common/toolkit/StringTk.h>
#include <toolkit/DatabaseTk.h>
#include <toolkit/FsckTkEx.h>

TestFsckTk::TestFsckTk()
{
}

TestFsckTk::~TestFsckTk()
{
}

void TestFsckTk::setUp()
{
}

void TestFsckTk::tearDown()
{
}

void TestFsckTk::testDirEntryTypeConversion()
{
   FsckDirEntryType entryTypeOut;

   entryTypeOut = FsckTk::DirEntryTypeToFsckDirEntryType(DirEntryType_INVALID);
   CPPUNIT_ASSERT_MESSAGE("DirEntryType_INVALID does not convert correctly",
      entryTypeOut == FsckDirEntryType_INVALID);

   entryTypeOut = FsckTk::DirEntryTypeToFsckDirEntryType(DirEntryType_REGULARFILE);
   CPPUNIT_ASSERT_MESSAGE("DirEntryType_REGULARFILE does not convert correctly",
      entryTypeOut == FsckDirEntryType_REGULARFILE);

   entryTypeOut = FsckTk::DirEntryTypeToFsckDirEntryType(DirEntryType_SYMLINK);
   CPPUNIT_ASSERT_MESSAGE("DirEntryType_SYMLINK does not convert correctly",
      entryTypeOut == FsckDirEntryType_SYMLINK);

   entryTypeOut = FsckTk::DirEntryTypeToFsckDirEntryType(DirEntryType_DIRECTORY);
   CPPUNIT_ASSERT_MESSAGE("DirEntryType_DIRECTORY does not convert correctly",
      entryTypeOut == FsckDirEntryType_DIRECTORY);

   entryTypeOut = FsckTk::DirEntryTypeToFsckDirEntryType(DirEntryType_BLOCKDEV);
   CPPUNIT_ASSERT_MESSAGE("DirEntryType_BLOCKDEV does not convert correctly",
      entryTypeOut == FsckDirEntryType_SPECIAL);

   entryTypeOut = FsckTk::DirEntryTypeToFsckDirEntryType(DirEntryType_CHARDEV);
   CPPUNIT_ASSERT_MESSAGE("DirEntryType_CHARDEV does not convert correctly",
      entryTypeOut == FsckDirEntryType_SPECIAL);

   entryTypeOut = FsckTk::DirEntryTypeToFsckDirEntryType(DirEntryType_FIFO);
   CPPUNIT_ASSERT_MESSAGE("DirEntryType_FIFO does not convert correctly",
      entryTypeOut == FsckDirEntryType_SPECIAL);

   entryTypeOut = FsckTk::DirEntryTypeToFsckDirEntryType(DirEntryType_SOCKET);
   CPPUNIT_ASSERT_MESSAGE("DirEntryType_SOCKET does not convert correctly",
      entryTypeOut == FsckDirEntryType_SPECIAL);
}

void TestFsckTk::testRemoveFromListDirEntries()
{
   unsigned NUM_DENTRIES = 10;
   unsigned NUM_DENTRIES_DELETE = 2;

   // create a list
   FsckDirEntryList dentries;
   DatabaseTk::createDummyFsckDirEntries(NUM_DENTRIES, &dentries);

   // create the delete list
   FsckDirEntryList dentriesDelete;
   DatabaseTk::createDummyFsckDirEntries(NUM_DENTRIES_DELETE, &dentriesDelete);

   FsckTkEx::removeFromList(dentries, dentriesDelete);

   CPPUNIT_ASSERT(dentries.size() == NUM_DENTRIES - NUM_DENTRIES_DELETE);
}

void TestFsckTk::testRemoveFromListDirInodes()
{
   unsigned NUM_INODES = 10;
   unsigned NUM_INODES_DELETE = 2;

   // create a list
   FsckDirInodeList inodes;
   DatabaseTk::createDummyFsckDirInodes(NUM_INODES, &inodes);

   // create the delete list
   FsckDirInodeList inodesDelete;
   DatabaseTk::createDummyFsckDirInodes(NUM_INODES_DELETE, &inodesDelete);

   FsckTkEx::removeFromList(inodes, inodesDelete);

   CPPUNIT_ASSERT(inodes.size() == NUM_INODES - NUM_INODES_DELETE);
}

void TestFsckTk::testRemoveFromListDirInodesByID()
{
   unsigned NUM_INODES = 10;
   unsigned NUM_INODES_DELETE = 2;

   // create a list
   FsckDirInodeList inodes;
   DatabaseTk::createDummyFsckDirInodes(NUM_INODES, &inodes);

   // create the delete list
   FsckDirInodeList inodesDelete;
   DatabaseTk::createDummyFsckDirInodes(NUM_INODES_DELETE, &inodesDelete);

   StringList idsDelete;

   for (FsckDirInodeListIter iter = inodesDelete.begin(); iter != inodesDelete.end(); iter++)
   {
      idsDelete.push_back(iter->getID());
   }

   FsckTkEx::removeFromList(inodes, idsDelete);

   CPPUNIT_ASSERT(inodes.size() == NUM_INODES - NUM_INODES_DELETE);
}

void TestFsckTk::testRemoveFromListFileInodes()
{
   unsigned NUM_INODES = 10;
   unsigned NUM_INODES_DELETE = 2;

   // create a list
   FsckFileInodeList inodes;
   DatabaseTk::createDummyFsckFileInodes(NUM_INODES, &inodes);

   // create the delete list
   FsckFileInodeList inodesDelete;
   DatabaseTk::createDummyFsckFileInodes(NUM_INODES_DELETE, &inodesDelete);

   FsckTkEx::removeFromList(inodes, inodesDelete);

   CPPUNIT_ASSERT(inodes.size() == NUM_INODES - NUM_INODES_DELETE);
}

void TestFsckTk::testRemoveFromListFileInodesByID()
{
   unsigned NUM_INODES = 10;
   unsigned NUM_INODES_DELETE = 2;

   // create a list
   FsckFileInodeList inodes;
   DatabaseTk::createDummyFsckFileInodes(NUM_INODES, &inodes);

   // create the delete list
   FsckFileInodeList inodesDelete;
   DatabaseTk::createDummyFsckFileInodes(NUM_INODES_DELETE, &inodesDelete);

   StringList idsDelete;

   for (FsckFileInodeListIter iter = inodesDelete.begin(); iter != inodesDelete.end(); iter++)
   {
      idsDelete.push_back(iter->getID());
   }

   FsckTkEx::removeFromList(inodes, idsDelete);

   CPPUNIT_ASSERT(inodes.size() == NUM_INODES - NUM_INODES_DELETE);
}

void TestFsckTk::testRemoveFromListChunks()
{
   unsigned NUM_CHUNKS = 10;
   unsigned NUM_CHUNKS_DELETE = 2;

   // create a list
   FsckChunkList chunks;
   DatabaseTk::createDummyFsckChunks(NUM_CHUNKS, &chunks);

   // create the delete list
   FsckChunkList chunksDelete;
   DatabaseTk::createDummyFsckChunks(NUM_CHUNKS_DELETE, &chunksDelete);

   FsckTkEx::removeFromList(chunks, chunksDelete);

   CPPUNIT_ASSERT(chunks.size() == NUM_CHUNKS - NUM_CHUNKS_DELETE);
}

void TestFsckTk::testRemoveFromListChunksByID()
{
   unsigned NUM_CHUNKS = 10;
   unsigned NUM_CHUNKS_DELETE = 2;

   // create a list
   FsckChunkList chunks;
   DatabaseTk::createDummyFsckChunks(NUM_CHUNKS, &chunks);

   // create the delete list
   FsckChunkList chunksDelete;
   DatabaseTk::createDummyFsckChunks(NUM_CHUNKS_DELETE, &chunksDelete);

   StringList idsDelete;

   for (FsckChunkListIter iter = chunksDelete.begin(); iter != chunksDelete.end(); iter++)
   {
      idsDelete.push_back(iter->getID());
   }

   FsckTkEx::removeFromList(chunks, idsDelete);

   CPPUNIT_ASSERT(chunks.size() == NUM_CHUNKS - NUM_CHUNKS_DELETE);
}

void TestFsckTk::testRemoveFromListStrings()
{
   unsigned NUM_STRINGS = 10;
   unsigned NUM_STRINGS_DELETE = 2;

   // create a list
   StringList list;
   StringList deleteList;

   for (unsigned i=0; i<NUM_STRINGS; i++)
   {
      std::string s = "string" + StringTk::uintToStr(i);
      list.push_back(s);
   }

   for (unsigned i=0; i<NUM_STRINGS_DELETE; i++)
   {
      std::string s = "string" + StringTk::uintToStr(i);
      deleteList.push_back(s);
   }

   FsckTkEx::removeFromList(list, deleteList);

   CPPUNIT_ASSERT(list.size() == NUM_STRINGS - NUM_STRINGS_DELETE);
}

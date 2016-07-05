#include "TestMsgSerialization.h"

#include <common/net/message/fsck/AdjustChunkPermissionsMsg.h>
#include <common/net/message/fsck/AdjustChunkPermissionsRespMsg.h>
#include <common/net/message/fsck/ChangeStripeTargetMsg.h>
#include <common/net/message/fsck/ChangeStripeTargetRespMsg.h>
#include <common/net/message/fsck/FetchFsckChunkListMsg.h>
#include <common/net/message/fsck/FetchFsckChunkListRespMsg.h>
#include <common/net/message/fsck/FsckSetEventLoggingMsg.h>
#include <common/net/message/fsck/FsckSetEventLoggingRespMsg.h>
#include <common/net/message/fsck/MoveChunkFileMsg.h>
#include <common/net/message/fsck/MoveChunkFileRespMsg.h>
#include <common/net/message/fsck/RemoveInodesMsg.h>
#include <common/net/message/fsck/RemoveInodesRespMsg.h>
#include <common/net/message/fsck/RetrieveDirEntriesMsg.h>
#include <common/net/message/fsck/RetrieveDirEntriesRespMsg.h>
#include <common/net/message/fsck/RetrieveFsIDsMsg.h>
#include <common/net/message/fsck/RetrieveFsIDsRespMsg.h>
#include <common/net/message/fsck/RetrieveInodesRespMsg.h>
#include <common/net/message/fsck/RetrieveInodesMsg.h>
#include <toolkit/DatabaseTk.h>


TestMsgSerialization::TestMsgSerialization()
   : log("TestMsgSerialization")
{
}

TestMsgSerialization::~TestMsgSerialization()
{
}

void TestMsgSerialization::setUp()
{
}

void TestMsgSerialization::tearDown()
{
}

void TestMsgSerialization::testRetrieveDirEntriesMsgSerialization()
{
   log.log(Log_DEBUG, "testRetrieveDirEntriesMsgSerialization started");

   unsigned hashDirNum = 55;
   std::string currentContDirID = "currentContDir";
   unsigned maxOutEntries = 150;
   int64_t lastHashDirOffset = 123456789;
   int64_t lastContDirOffset = 987654321;

   RetrieveDirEntriesMsg msg(hashDirNum, currentContDirID, maxOutEntries, lastHashDirOffset,
      lastContDirOffset);
   RetrieveDirEntriesMsg msgClone;

   bool testRes = this->testMsgSerialization(msg, msgClone);

   if (!testRes)
      CPPUNIT_FAIL("Failed to serialize/deserialize RetrieveDirEntriesMsg");

   log.log(Log_DEBUG, "testRetrieveDirEntriesMsgSerialization finished");
}

void TestMsgSerialization::testRetrieveDirEntriesRespMsgSerialization()
{
   log.log(Log_DEBUG, "testRetrieveDirEntriesRespMsgSerialization started");

   FsckContDirList fsckContDirs;
   DatabaseTk::createDummyFsckContDirs(100, &fsckContDirs);

   FsckDirEntryList fsckDirEntries;
   DatabaseTk::createDummyFsckDirEntries(100, &fsckDirEntries);

   FsckFileInodeList inlinedFileInodes;
   DatabaseTk::createDummyFsckFileInodes(100, &inlinedFileInodes);

   std::string currentContDirID = "currentContDir";
   int64_t newHashDirOffset = 123456789;
   int64_t newContDirOffset = 987654321;

   RetrieveDirEntriesRespMsg msg(&fsckContDirs, &fsckDirEntries, &inlinedFileInodes,
      currentContDirID, newHashDirOffset, newContDirOffset);
   RetrieveDirEntriesRespMsg msgClone;

   bool testRes = this->testMsgSerialization(msg, msgClone);

   if (!testRes)
      CPPUNIT_FAIL("Failed to serialize/deserialize RetrieveDirEntriesRespMsg");

   log.log(Log_DEBUG, "testRetrieveDirEntriesRespMsgSerialization finished");
}

void TestMsgSerialization::testRetrieveFsIDsMsgSerialization()
{
   log.log(Log_DEBUG, "testRetrieveFsIDsMsgSerialization started");

   unsigned hashDirNum = 55;
   std::string currentContDirID = "currentContDir";
   unsigned maxOutEntries = 150;
   int64_t lastHashDirOffset = 123456789;
   int64_t lastContDirOffset = 987654321;

   RetrieveFsIDsMsg msg(hashDirNum, currentContDirID, maxOutEntries, lastHashDirOffset,
      lastContDirOffset);
   RetrieveFsIDsMsg msgClone;

   bool testRes = this->testMsgSerialization(msg, msgClone);

   if (!testRes)
      CPPUNIT_FAIL("Failed to serialize/deserialize RetrieveFsIDsMsg");

   log.log(Log_DEBUG, "testRetrieveFsIDsMsgSerialization finished");
}

void TestMsgSerialization::testRetrieveFsIDsRespMsgSerialization()
{
   log.log(Log_DEBUG, "testRetrieveFsIDsRespMsgSerialization started");

   FsckFsIDList fsIDs;
   DatabaseTk::createDummyFsckFsIDs(100, &fsIDs);

   std::string currentContDirID = "currentContDir";
   int64_t newHashDirOffset = 123456789;
   int64_t newContDirOffset = 987654321;

   RetrieveFsIDsRespMsg msg(&fsIDs, currentContDirID, newHashDirOffset, newContDirOffset);
   RetrieveFsIDsRespMsg msgClone;

   bool testRes = this->testMsgSerialization(msg, msgClone);

   if (!testRes)
      CPPUNIT_FAIL("Failed to serialize/deserialize RetrieveFsIDsRespMsg");

   log.log(Log_DEBUG, "testRetrieveFsIDsRespMsgSerialization finished");
}

void TestMsgSerialization::testRetrieveInodesMsgSerialization()
{
   log.log(Log_DEBUG, "testRetrieveInodesMsgSerialization started");

   unsigned hashDirNum = 55;
   unsigned maxOutInodes = 150;
   int64_t lastOffset = 123456789;

   RetrieveInodesMsg msg(hashDirNum, lastOffset, maxOutInodes);
   RetrieveInodesMsg msgClone;

   bool testRes = this->testMsgSerialization(msg, msgClone);

   if (!testRes)
      CPPUNIT_FAIL("Failed to serialize/deserialize RetrieveInodesMsg");

   log.log(Log_DEBUG, "testRetrieveInodesMsgSerialization finished");
}

void TestMsgSerialization::testRetrieveInodesRespMsgSerialization()
{
   log.log(Log_DEBUG, "testRetrieveInodesRespMsgSerialization started");

   FsckFileInodeList fileInodes;
   DatabaseTk::createDummyFsckFileInodes(100, &fileInodes);

   FsckDirInodeList dirInodes;
   DatabaseTk::createDummyFsckDirInodes(100, &dirInodes);

   int64_t lastOffset = 123456789;

   RetrieveInodesRespMsg msg(&fileInodes, &dirInodes, lastOffset);
   RetrieveInodesRespMsg msgClone;

   bool testRes = this->testMsgSerialization(msg, msgClone);

   if (!testRes)
      CPPUNIT_FAIL("Failed to serialize/deserialize RetrieveInodesRespMsg");

   log.log(Log_DEBUG, "testRetrieveInodesRespMsgSerialization finished");
}

void TestMsgSerialization::testFetchFsckChunkListMsgSerialization()
{
   log.log(Log_DEBUG, "testFetchFsckChunkListMsgSerialization started");

   FsckChunkList chunkList;
   DatabaseTk::createDummyFsckChunks(100, &chunkList);

   unsigned maxNum = 100;
   FetchFsckChunkListStatus lastStatus = (FetchFsckChunkListStatus)3;
   bool forceRestart = false;

   FetchFsckChunkListMsg msg(maxNum, lastStatus, forceRestart);
   FetchFsckChunkListMsg msgClone;

   bool testRes = this->testMsgSerialization(msg, msgClone);

   if (!testRes)
      CPPUNIT_FAIL("Failed to serialize/deserialize FetchFsckChunkListMsg");

   log.log(Log_DEBUG, "testFetchFsckChunkListMsgSerialization finished");
}

void TestMsgSerialization::testFetchFsckChunkListRespMsgSerialization()
{
   log.log(Log_DEBUG, "testFetchFsckChunkListRespMsgSerialization started");

   FsckChunkList chunkList;
   DatabaseTk::createDummyFsckChunks(100, &chunkList);

   FetchFsckChunkListStatus status = (FetchFsckChunkListStatus)3;

   FetchFsckChunkListRespMsg msg(&chunkList, status);
   FetchFsckChunkListRespMsg msgClone;

   bool testRes = this->testMsgSerialization(msg, msgClone);

   if (!testRes)
      CPPUNIT_FAIL("Failed to serialize/deserialize FetchFsckChunkListRespMsg");

   log.log(Log_DEBUG, "testFetchFsckChunkListRespMsgSerialization finished");
}

void TestMsgSerialization::testChangeStripeTargetMsgSerialization()
{
   log.log(Log_DEBUG, "testChangeStripeTargetMsgSerialization started");

   FsckFileInodeList inodes;
   DatabaseTk::createDummyFsckFileInodes(10,&inodes);
   uint16_t targetID = 1234;
   uint16_t newTargetID = 4321;

   ChangeStripeTargetMsg msg(&inodes, targetID, newTargetID);
   ChangeStripeTargetMsg msgClone;

   bool testRes = this->testMsgSerialization(msg, msgClone);

   if (!testRes)
      CPPUNIT_FAIL("Failed to serialize/deserialize ChangeStripeTargetMsg");

   log.log(Log_DEBUG, "testChangeStripeTargetMsgSerialization finished");
}

void TestMsgSerialization::testChangeStripeTargetRespMsgSerialization()
{
   log.log(Log_DEBUG, "testChangeStripeTargetRespMsgSerialization started");

   FsckFileInodeList inodes;
   DatabaseTk::createDummyFsckFileInodes(10,&inodes);

   ChangeStripeTargetRespMsg msg(&inodes);
   ChangeStripeTargetRespMsg msgClone;

   bool testRes = this->testMsgSerialization(msg, msgClone);

   if (!testRes)
      CPPUNIT_FAIL("Failed to serialize/deserialize ChangeStripeTargetRespMsg");

   log.log(Log_DEBUG, "testChangeStripeTargetRespMsgSerialization finished");
}

void TestMsgSerialization::testFsckSetEventLoggingMsg()
{
   log.log(Log_DEBUG, "testFsckSetEventLoggingMsg started");

   NicAddressList nicList;
   FsckSetEventLoggingMsg msg(true, 1024, &nicList, false);

   FsckSetEventLoggingMsg msgClone;

   bool testRes = this->testMsgSerialization(msg, msgClone);

   if (!testRes)
      CPPUNIT_FAIL("Failed to serialize/deserialize FsckSetEventLoggingMsg");

   log.log(Log_DEBUG, "testFsckSetEventLoggingMsg finished");
}

void TestMsgSerialization::testFsckSetEventLoggingRespMsg()
{
   log.log(Log_DEBUG, "testFsckSetEventLoggingRespMsg started");

   FsckSetEventLoggingRespMsg msg(true, true, false);

   FsckSetEventLoggingRespMsg msgClone;

   bool testRes = this->testMsgSerialization(msg, msgClone);

   if (!testRes)
      CPPUNIT_FAIL("Failed to serialize/deserialize FsckSetEventLoggingRespMsg");

   log.log(Log_DEBUG, "testFsckSetEventLoggingRespMsg finished");
}

void TestMsgSerialization::testAdjustChunkPermissionsMsgSerialization()
{
   log.log(Log_DEBUG, "testAdjustChunkPermissionsMsgSerialization started");

   std::string currentContDirID = "currentContDirID";
   AdjustChunkPermissionsMsg msg(1000, currentContDirID, 2000, 3000, 4000);

   AdjustChunkPermissionsMsg msgClone;

   bool testRes = this->testMsgSerialization(msg, msgClone);

   if (!testRes)
      CPPUNIT_FAIL("Failed to serialize/deserialize AdjustChunkPermissionsMsg");

   log.log(Log_DEBUG, "testAdjustChunkPermissionsMsgSerialization finished");
}

void TestMsgSerialization::testAdjustChunkPermissionsRespMsgSerialization()
{
   log.log(Log_DEBUG, "testAdjustChunkPermissionsRespMsgSerialization started");

   std::string currentContDirID = "currentContDirID";
   AdjustChunkPermissionsRespMsg msg(1000, currentContDirID, 2000, 3000, true);

   AdjustChunkPermissionsRespMsg msgClone;

   bool testRes = this->testMsgSerialization(msg, msgClone);

   if (!testRes)
      CPPUNIT_FAIL("Failed to serialize/deserialize AdjustChunkPermissionsRespMsg");

   log.log(Log_DEBUG, "testAdjustChunkPermissionsRespMsgSerialization finished");
}

void TestMsgSerialization::testMoveChunkFileMsg()
{
   log.log(Log_DEBUG, "testMoveChunkFileMsg started");

   std::string chunkName = "chunkName";
   uint16_t targetID = 1000;
   uint16_t mirroredFromTargetID = 2000;
   std::string oldPath = "path/to/old/path";
   std::string newPath = "path/to/new/path";
   MoveChunkFileMsg msg(chunkName, targetID, mirroredFromTargetID, oldPath, newPath);

   MoveChunkFileMsg msgClone;

   bool testRes = this->testMsgSerialization(msg, msgClone);

   if (!testRes)
      CPPUNIT_FAIL("Failed to serialize/deserialize MoveChunkFileMsg");

   log.log(Log_DEBUG, "testMoveChunkFileMsg finished");
}

void TestMsgSerialization::testMoveChunkFileRespMsg()
{
   log.log(Log_DEBUG, "testMoveChunkFileRespMsg started");

   int result = 1000;
   MoveChunkFileRespMsg msg(result);

   MoveChunkFileRespMsg msgClone;

   bool testRes = this->testMsgSerialization(msg, msgClone);

   if (!testRes)
      CPPUNIT_FAIL("Failed to serialize/deserialize MoveChunkFileRespMsg");

   log.log(Log_DEBUG, "testMoveChunkFileRespMsg finished");
}

void TestMsgSerialization::testRemoveInodesMsg()
{
   log.log(Log_DEBUG, "testRemoveInodesMsg started");

   StringList entryIDList;
   DirEntryTypeList entryTypeList;

   for (int i=0; i<100; i++)
   {
      entryIDList.push_back("entry " + StringTk::intToStr(i));
      entryTypeList.push_back(DirEntryType(1));
   }

   RemoveInodesMsg msg(&entryIDList, &entryTypeList);

   RemoveInodesMsg msgClone;

   bool testRes = this->testMsgSerialization(msg, msgClone);

   if (!testRes)
      CPPUNIT_FAIL("Failed to serialize/deserialize RemoveInodesMsg");

   log.log(Log_DEBUG, "testRemoveInodesMsg finished");
}

void TestMsgSerialization::testRemoveInodesRespMsg()
{
   log.log(Log_DEBUG, "testRemoveInodesRespMsg started");

   StringList entryIDList;
   DirEntryTypeList entryTypeList;

   for (int i=0; i<100; i++)
   {
      entryIDList.push_back("entry " + StringTk::intToStr(i));
      entryTypeList.push_back(DirEntryType(1));
   }

   RemoveInodesRespMsg msg(&entryIDList, &entryTypeList);

   RemoveInodesRespMsg msgClone;

   bool testRes = this->testMsgSerialization(msg, msgClone);

   if (!testRes)
      CPPUNIT_FAIL("Failed to serialize/deserialize RemoveInodesRespMsg");

   log.log(Log_DEBUG, "testRemoveInodesRespMsg finished");
}

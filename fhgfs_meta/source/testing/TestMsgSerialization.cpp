#include "TestMsgSerialization.h"

#include <common/net/message/fsck/FsckModificationEventMsg.h>
#include <common/net/message/nodes/HeartbeatRequestMsg.h>
#include <common/net/message/nodes/HeartbeatMsg.h>
#include <common/net/message/storage/creating/HardlinkMsg.h>
#include <common/net/message/storage/creating/MkDirMsg.h>
#include <common/net/message/session/opening/CloseChunkFileMsg.h>

TestMsgSerialization::TestMsgSerialization()
{
	log.setContext("TestMsgSerialization");
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

void TestMsgSerialization::testMkDirMsgSerialization()
{
   log.log(Log_DEBUG, "testMkDirMsgSerialization started");

   uint16_t ownerNodeID            = 123;
   std::string parentParentEntryID = "parentOfParent";
   std::string parentEntryID       = "parentID";
   std::string fileName            = "exampleFile";
   DirEntryType parentType         = DirEntryType_DIRECTORY;
   int flags                       = 0;

   EntryInfo parentInfo(ownerNodeID, parentParentEntryID, parentEntryID, fileName,
      parentType, flags);

   unsigned int uid = 2345;
   unsigned int gid = 8794;
   int mode = 0777;
   int umask = 0022;
   std::string newDirName = "test";
   UInt16List preferredNodes;

   MkDirMsg msg(&parentInfo, newDirName, uid, gid, mode, umask, &preferredNodes);
   MkDirMsg msgClone;

   bool testRes = this->testMsgSerialization(msg, msgClone);

   if (!testRes)
	   CPPUNIT_FAIL("Failed to serialize/deserialize MkDirMsgSerialization");

   log.log(Log_DEBUG, "testMkDirMsgSerialization finished");
}

void TestMsgSerialization::testFsckModificationEventMsgSerialization()
{
   log.log(Log_DEBUG, "testFsckModificationEventMsgSerialization started");

   UInt8List eventTypes;
   StringList entryIDs;

   for (unsigned i=0; i<9; i++)
   {
      eventTypes.push_back(i);
      entryIDs.push_back("entryID" + StringTk::uintToStr(i));
   }

   FsckModificationEventMsg msg(&eventTypes, &entryIDs, true);
   FsckModificationEventMsg msgClone;

   bool testRes = this->testMsgSerialization(msg, msgClone);

   if (!testRes)
      CPPUNIT_FAIL("Failed to serialize/deserialize FsckModificationEventMsg");

   log.log(Log_DEBUG, "testFsckModificationEventMsgSerialization finished");
}

void TestMsgSerialization::testCloseChunkFileMsgSerialization()
{
   log.log(Log_DEBUG, "testCloseChunkFileMsgSerialization started");

   std::string sessionID = "SessionID";
   std::string fileHandleID = "fileHandleID";
   uint16_t targetID = 1234;

   unsigned origParentUID = 2346;
   std::string origParentEntryID("someID");
   unsigned flags   = PATHINFO_FEATURE_ORIG;
   PathInfo pathInfo(origParentUID, origParentEntryID, flags);

   CloseChunkFileMsg msg(sessionID, fileHandleID, targetID, &pathInfo);
   CloseChunkFileMsg msgClone;

   bool testRes = this->testMsgSerialization(msg, msgClone);

   if (!testRes)
      CPPUNIT_FAIL("Failed to serialize/deserialize CloseChunkFileMsg");

   log.log(Log_DEBUG, "testCloseChunkFileMsgSerialization finished");
}

void TestMsgSerialization::testCloseChunkFileMsgSerializationHsm()
{
   log.log(Log_DEBUG, "testCloseChunkFileMsgSerializationHsm started.");

   std::string sessionID = "SessionID";
   std::string fileHandleID = "fileHandleID";
   uint16_t targetID = 1234;

   PathInfo pathInfo; // pathInfo with origFeature

   const char* entryInfoBuf = "abcdefghijklmnopqrstuvwxyz";
   unsigned entryInfoBufLen = strlen(entryInfoBuf);

   CloseChunkFileMsg msg(sessionID, fileHandleID, targetID, &pathInfo);
   CloseChunkFileMsg msgClone;

   msg.setHsmCollocationID(1111);

   msg.setEntryInfoBuf(entryInfoBuf, entryInfoBufLen);

   bool testRes = this->testMsgSerialization(msg, msgClone);

   if (!testRes)
      CPPUNIT_FAIL("Failed to serialize/deserialize CloseChunkFileMsg with HSM support.");

   log.log(Log_DEBUG, "testCloseChunkFileMsgSerializationHsm finished.");
}

void TestMsgSerialization::testHardlinkMsgSerialization()
{
   log.log(Log_DEBUG, "testHardlinkMsgSerialization started");

   std::string fromName = "fromName";
   std::string entryID  = "entryID";

   std::string toName   = "toName";

   uint16_t fromOwnerNodeID        = 123;
   uint16_t toOwnerNodeID          = 456;

   std::string fromGrandParentID   = "fromGrandParentID";
   std::string toGrandParentID     = "toGrandParentID";

   std::string fromParentID        = "parentID";
   std::string toParentID          = "toParentID";

   std::string fromDirName         = "fromDir";
   std::string toDirName           = "toDir";

   DirEntryType dirType            = DirEntryType_DIRECTORY;
   int flags                       = 0;

   EntryInfo fromDirInfo(fromOwnerNodeID, fromGrandParentID, fromParentID, fromDirName,
      dirType, flags);
   EntryInfo toDirInfo(toOwnerNodeID, toGrandParentID, toParentID, toDirName, dirType, flags);

   DirEntryType fileType           = DirEntryType_REGULARFILE;
   EntryInfo fromFileInfo(fromOwnerNodeID, fromParentID, entryID, fromName, fileType, 0);


   // test client/server serialization/deserialization
   HardlinkMsg linkMsg(&fromDirInfo, fromName, &fromFileInfo, &toDirInfo, toName);
   HardlinkMsg linkCloneMsg;

   bool testRes = this->testMsgSerialization(linkMsg, linkCloneMsg);
   if (!testRes)
      CPPUNIT_FAIL("Failed to serialize/deserialize HardlinkMsgSerialization!");


   // test meta-to-meta link-dentry creation
   HardlinkMsg linkDentryMsg(&fromFileInfo, &toDirInfo, toName);
   HardlinkMsg linkDentryCloneMsg;

   bool testLinkDentryRes = this->testMsgSerialization(linkDentryMsg, linkDentryCloneMsg);
   if (!testLinkDentryRes)
      CPPUNIT_FAIL("Failed to serialize/deserialize (link dentry) HardlinkMsgSerialization!");


   log.log(Log_DEBUG, "testHardlinkMsgSerialization finished.");



}


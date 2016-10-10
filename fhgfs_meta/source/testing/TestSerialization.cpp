#include "TestSerialization.h"

#include <common/storage/EntryInfo.h>
#include <common/storage/striping/BuddyMirrorPattern.h>
#include <common/storage/striping/Raid0Pattern.h>
#include <common/storage/striping/Raid10Pattern.h>
#include <session/Session.h>



TestSerialization::TestSerialization()
{
   log.setContext("TestSerialization");
}

TestSerialization::~TestSerialization()
{
}

void TestSerialization::setUp()
{
}

void TestSerialization::tearDown()
{
}

void TestSerialization::testEntryInfoListSerialization()
{
   log.log(Log_DEBUG, "testEntryInfoListSerialization started");

   EntryInfoList listIn;
   EntryInfoList listOut;

   for (int i = 0; i < 5; i++)
   {
      uint16_t ownerNodeID = 1 + i; // "1+" to avoid invalid ID '0'
      std::string parentEntryID = "parentEntryID" + StringTk::intToStr(i);
      std::string entryID = "entryID" + StringTk::intToStr(i);
      std::string fileName = "fileName" + StringTk::intToStr(i);
      DirEntryType entryType = DirEntryType_REGULARFILE;
      int statFlags = 0;

      EntryInfo entryInfo(ownerNodeID, parentEntryID, entryID, fileName, entryType, statFlags);
      listIn.push_back(entryInfo);
   }

   size_t bufLen = Serialization::serialLenEntryInfoList(&listIn);
   char* buf = (char*) malloc(bufLen);

   Serialization::serializeEntryInfoList(buf, &listIn);

   unsigned elemNum;
   const char *infoStart;
   unsigned infoLen;

   bool preprocRes = Serialization::deserializeEntryInfoListPreprocess(buf, bufLen, &elemNum,
      &infoStart, &infoLen);

   if (!preprocRes)
   {
      CPPUNIT_FAIL("Deserialization preprocessing failed");
   }

   Serialization::deserializeEntryInfoList(infoLen, elemNum, infoStart, &listOut);

   free(buf);

   CPPUNIT_ASSERT(listIn.size() == listOut.size());

   EntryInfoListIter listInIter = listIn.begin();
   EntryInfoListIter listOutIter = listOut.begin();

   while (listInIter != listIn.end())
   {
      EntryInfo entryIn = *listInIter;
      EntryInfo entryOut = *listOutIter;

      CPPUNIT_ASSERT (entryIn.getEntryID().compare(entryOut.getEntryID()) == 0);
      CPPUNIT_ASSERT (entryIn.getFileName().compare(entryOut.getFileName()) == 0);
      CPPUNIT_ASSERT (entryIn.getOwnerNodeID() == entryOut.getOwnerNodeID());
      CPPUNIT_ASSERT (entryIn.getParentEntryID().compare(entryOut.getParentEntryID()) == 0);

      CPPUNIT_ASSERT (entryIn.getEntryType() == entryOut.getEntryType());
      CPPUNIT_ASSERT (entryIn.getFlags() == entryOut.getFlags());

      listInIter++;
      listOutIter++;
   }

   log.log(Log_DEBUG, "testEntryInfoListSerialization finished");
}

void TestSerialization::testStripePatternSerialization()
{
   log.log(Log_DEBUG, "testStripePatternSerialization started");

   bool failure = false;
   std::string failureMsg = "";

   UInt16Vector hostVec;
   UInt16VectorConstIter iterThis;
   UInt16VectorConstIter iterOther;
   UInt16Vector hostVecClone;

   hostVec.push_back(1);
   hostVec.push_back(2);
   hostVec.push_back(3);

   Raid0Pattern* pattern = new Raid0Pattern(1234, hostVec);

   size_t bufLen = pattern->getSerialPatternLength();
   char* buf = (char*)malloc(bufLen);

   if(pattern->serialize(buf) != bufLen)
      CPPUNIT_FAIL("Serialization failed");

   StripePatternHeader headerClone;
   const char* patternStartClone;
   unsigned patternLengthClone = ~0;
   StripePattern::deserializePatternPreprocess(buf, bufLen,
      &headerClone, &patternStartClone, &patternLengthClone);

   StripePattern* patternClone = StripePattern::createFromBuf(buf, headerClone);

   if( (patternLengthClone != bufLen) )
   {
	   failure = true;
	   failureMsg = "Pattern-lengths do not match";
	   goto cleanup;
   }

   if( (patternClone->getPatternType() != pattern->getPatternType() ) )
   {
	   failure = true;
	   failureMsg = "PatternTypes do not match";
	   goto cleanup;
   }

   if( (patternClone->getChunkSize() != pattern->getChunkSize() ) )
   {
	   failure = true;
	   failureMsg = "Chunk-sizes do not match";
	   goto cleanup;
   }

   patternClone->getStripeTargetIDs(&hostVecClone);

   if(hostVec.size() != hostVecClone.size() )
	   failure = true;
	   failureMsg = "Number of stripe targets does not match";
	   goto cleanup;

   iterThis = hostVec.begin();
   iterOther = hostVecClone.begin();
   for( ; iterThis != hostVec.end(); iterThis++, iterOther++)
   {
      if(*iterThis != *iterOther)
      {
   	   failure = true;
   	   failureMsg = "Stripe targets do not match";
   	   goto cleanup;
      }
   }

cleanup:
   // clean up
   free(buf);
   delete(patternClone);

   if (failure)
	   CPPUNIT_FAIL(failureMsg);

   log.log(Log_DEBUG, "testStripePatternSerialization finished");
}

void TestSerialization::testSessionSerialization()
{
   log.log(Log_DEBUG, "testSessionSerialization started");

   SessionStore sessionStore;
   SessionStore sessionStoreClone;

   initSessionStoreForTests(sessionStore);

   size_t bufLen = sessionStore.serialLen();
   size_t bufPos = 0;
   unsigned filesLen = 0;

   char* buf = (char*)malloc(bufLen);

   if(sessionStore.serialize(buf) != bufLen)
   {
      SAFE_FREE(buf);
      CPPUNIT_FAIL("Serialization failed!");
   }

   if(!sessionStoreClone.deserialize(&buf[bufPos], bufLen-bufPos, &filesLen) )
   {
      SAFE_FREE(buf);
      CPPUNIT_FAIL("Deserialization failed!");
   }

   // Can be enabled when the MetaStore is provided in the test. The we can enable the inode check
   // in sessionStoreMetaEquals(...) method
   //if(!sessionStoreClone.deserializeLockStates(buf, filesLen, bufLen) )
   //{
   //   SAFE_FREE(buf);
   //   CPPUNIT_FAIL("Deserialization of lock states failed!");
   //}
   //

   if(!sessionStoreMetaEquals(sessionStore, sessionStoreClone, true) )
   {
      SAFE_FREE(buf);
      CPPUNIT_FAIL("Sessions is not equal after serialization and deserialization!");
   }

   log.log(Log_DEBUG, "testSessionSerialization finished");
}

void TestSerialization::initSessionStoreForTests(SessionStore& testSessionStore)
{
   SettableFileAttribs* settableFileAttribs10 = new SettableFileAttribs();
   settableFileAttribs10->groupID = UINT_MAX;
   settableFileAttribs10->lastAccessTimeSecs = std::numeric_limits<int64_t>::max();
   settableFileAttribs10->mode = INT_MAX;
   settableFileAttribs10->modificationTimeSecs = std::numeric_limits<int64_t>::max();
   settableFileAttribs10->userID = UINT_MAX;
   size_t numTargets10 = 25;
   StatData* statData10 = new StatData(std::numeric_limits<int64_t>::min(), settableFileAttribs10,
      std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min(), 0, 0);
   statData10->setSparseFlag();
   UInt16Vector* targets10 = new UInt16Vector(numTargets10);
   TestSerialization::addRandomValuesToUInt16Vector(targets10, numTargets10);
   UInt16Vector* mirrorTargets10 = new UInt16Vector(numTargets10);
   TestSerialization::addRandomValuesToUInt16Vector(mirrorTargets10, numTargets10);
   TestSerialization::addRandomTargetChunkBlocksToStatData(statData10, numTargets10);
   Raid10Pattern* stripePattern10 = new Raid10Pattern(UINT_MAX, *targets10, *mirrorTargets10,
      numTargets10);
   HsmFileMetaData* hsmFileMetaData10 = new HsmFileMetaData(std::numeric_limits<uint16_t>::max(),
      std::numeric_limits<uint16_t>::max() );
   std::string* entryID10 = new std::string("0-00ADF231-7");
   std::string* origParentEntryID10 = new std::string("2-AFD734AF-6");
   FileInodeStoreData* fileInodeStoreData10 = new FileInodeStoreData(*entryID10, statData10,
      stripePattern10, FILEINODE_FEATURE_HAS_ORIG_UID, 1345, *origParentEntryID10,
      FileInodeOrigFeature_TRUE);
   fileInodeStoreData10->setHsmFileMetaData(*hsmFileMetaData10);
   FileInode* inode10 = new FileInode(*entryID10, fileInodeStoreData10, DirEntryType_REGULARFILE,
      UINT_MAX);
   inode10->setIsInlinedUnlocked(false);
   inode10->initLocksRandomForSerializationTests();
   EntryInfo* entryInfo10 = new EntryInfo(345, *origParentEntryID10, *entryID10, "testfile10",
      DirEntryType_REGULARFILE, 0);

   SettableFileAttribs* settableFileAttribs11 = new SettableFileAttribs();
   settableFileAttribs11->groupID = 0;
   settableFileAttribs11->lastAccessTimeSecs = std::numeric_limits<int64_t>::min();
   settableFileAttribs11->mode = INT_MIN;
   settableFileAttribs11->modificationTimeSecs = std::numeric_limits<int64_t>::min();
   settableFileAttribs11->userID = 0;
   size_t numTargets11 = 2487;
   StatData* statData11 = new StatData(std::numeric_limits<int64_t>::max(), settableFileAttribs11,
      std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(),
      UINT_MAX, UINT_MAX);
   statData11->setSparseFlag();
   UInt16Vector* targets11 = new UInt16Vector(numTargets11);
   TestSerialization::addRandomValuesToUInt16Vector(targets11, numTargets11);
   TestSerialization::addRandomTargetChunkBlocksToStatData(statData11, numTargets11);
   Raid0Pattern* stripePattern11 = new Raid0Pattern(1024, *targets11, numTargets11);
   HsmFileMetaData* hsmFileMetaData11 = new HsmFileMetaData(0, 0);
   std::string* entryID11 = new std::string("1-345234AF-4");
   std::string* origParentEntryID11 = new std::string("1-892CD123-9");
   FileInodeStoreData* fileInodeStoreData11 = new FileInodeStoreData(*entryID11, statData11,
      stripePattern11, FILEINODE_FEATURE_HAS_ORIG_UID, 145, *origParentEntryID11,
      FileInodeOrigFeature_TRUE);
   fileInodeStoreData11->setHsmFileMetaData(*hsmFileMetaData11);
   FileInode* inode11 = new FileInode(*entryID11, fileInodeStoreData11, DirEntryType_REGULARFILE,
      0);
   inode11->setIsInlinedUnlocked(true);
   inode11->initLocksRandomForSerializationTests();
   EntryInfo* entryInfo11 = new EntryInfo(45, *origParentEntryID11, *entryID11, "testfile11",
      DirEntryType_FIFO, ENTRYINFO_FEATURE_INLINED);

   SettableFileAttribs* settableFileAttribs12 = new SettableFileAttribs();
   settableFileAttribs12->groupID = 457;
   settableFileAttribs12->lastAccessTimeSecs = 7985603;
   settableFileAttribs12->mode = 785;
   settableFileAttribs12->modificationTimeSecs = 54113;
   settableFileAttribs12->userID = 784;
   size_t numTargets12 = 22;
   StatData* statData12 = new StatData(55568, settableFileAttribs12, 785133, 92334, 45, 78);
   statData12->setSparseFlag();
   UInt16Vector* targets12 = new UInt16Vector(numTargets12);
   TestSerialization::addRandomValuesToUInt16Vector(targets12, numTargets12);
   BuddyMirrorPattern* stripePattern12 = new BuddyMirrorPattern(4096, *targets12, numTargets12);
   HsmFileMetaData* hsmFileMetaData12 = new HsmFileMetaData(4656, 7864);
   std::string* entryID12 = new std::string("8-DC12AF12-3");
   std::string* origParentEntryID12 = new std::string("9-FE998021-3");
   FileInodeStoreData* fileInodeStoreData12 = new FileInodeStoreData(*entryID12, statData12,
      stripePattern12, FILEINODE_FEATURE_HAS_ORIG_UID, 1789, *origParentEntryID12,
      FileInodeOrigFeature_TRUE);
   fileInodeStoreData12->setHsmFileMetaData(*hsmFileMetaData12);
   FileInode* inode12 = new FileInode(*entryID12, fileInodeStoreData12, DirEntryType_REGULARFILE,
      DENTRY_FEATURE_IS_FILEINODE);
   inode12->setIsInlinedUnlocked(true);
   inode12->initLocksRandomForSerializationTests();
   EntryInfo* entryInfo12 = new EntryInfo(789, *origParentEntryID12, *entryID12, "testfile12",
      DirEntryType_DIRECTORY, 0);

   SettableFileAttribs* settableFileAttribs13 = new SettableFileAttribs();
   settableFileAttribs13->groupID = 556;
   settableFileAttribs13->lastAccessTimeSecs =798336;
   settableFileAttribs13->mode = 456;
   settableFileAttribs13->modificationTimeSecs = 12383635;
   settableFileAttribs13->userID = 886;
   size_t numTargets13 = 55;
   StatData* statData13 = new StatData(775616, settableFileAttribs13, 56633, 565663, 7893, 1315);
   UInt16Vector* targets13 = new UInt16Vector(numTargets13);
   TestSerialization::addRandomValuesToUInt16Vector(targets13, numTargets13);
   BuddyMirrorPattern* stripePattern13 = new BuddyMirrorPattern(10240, *targets13, numTargets13);
   HsmFileMetaData* hsmFileMetaData13 = new HsmFileMetaData(55, 45);
   std::string* entryID13 = new std::string("3-CDF1239F-9");
   std::string* origParentEntryID13 = new std::string("4-DFA23A31-2");
   FileInodeStoreData* fileInodeStoreData13 = new FileInodeStoreData(*entryID13, statData13,
      stripePattern13, FILEINODE_FEATURE_HAS_ORIG_UID, 1892, *origParentEntryID13,
      FileInodeOrigFeature_TRUE);
   fileInodeStoreData13->setHsmFileMetaData(*hsmFileMetaData13);
   FileInode* inode13 = new FileInode(*entryID13, fileInodeStoreData13, DirEntryType_REGULARFILE,
      DENTRY_FEATURE_IS_FILEINODE);
   inode13->setIsInlinedUnlocked(false);
   inode13->initLocksRandomForSerializationTests();
   EntryInfo* entryInfo13 = new EntryInfo(892, *origParentEntryID13, *entryID13, "testfile13",
      DirEntryType_REGULARFILE, ENTRYINFO_FEATURE_INLINED);

   SettableFileAttribs* settableFileAttribs14 = new SettableFileAttribs();
   settableFileAttribs14->groupID = 8963;
   settableFileAttribs14->lastAccessTimeSecs = 3468869;
   settableFileAttribs14->mode = 6858;
   settableFileAttribs14->modificationTimeSecs = 167859;
   settableFileAttribs14->userID = 23;
   size_t numTargets14 = 2;
   StatData* statData14 = new StatData(5435585, settableFileAttribs14, 12855, 78, 965, 55);
   statData14->setSparseFlag();
   UInt16Vector* targets14 = new UInt16Vector(numTargets14);
   TestSerialization::addRandomValuesToUInt16Vector(targets14, numTargets14);
   TestSerialization::addRandomTargetChunkBlocksToStatData(statData14, numTargets14);
   Raid0Pattern* stripePattern14 = new Raid0Pattern (512, *targets14, numTargets14);
   std::string* entryID14 = new std::string("4-345234AF-5");
   std::string* origParentEntryID14 = new std::string("8-AB782DDF-6");
   FileInodeStoreData* fileInodeStoreData14 = new FileInodeStoreData(*entryID14, statData14,
      stripePattern14, FILEINODE_FEATURE_HAS_ORIG_UID, 11021, *origParentEntryID14,
      FileInodeOrigFeature_TRUE);
   FileInode* inode14 = new FileInode(*entryID14, fileInodeStoreData14, DirEntryType_REGULARFILE,
      DENTRY_FEATURE_IS_FILEINODE);
   inode14->setIsInlinedUnlocked(false);
   inode14->initLocksRandomForSerializationTests();
   EntryInfo* entryInfo14 = new EntryInfo(1021, *origParentEntryID14, *entryID14, "testfile14",
      DirEntryType_SYMLINK, ENTRYINFO_FEATURE_INLINED);


   SettableFileAttribs* settableFileAttribs20 = new SettableFileAttribs();
   settableFileAttribs20->groupID = 6644;
   settableFileAttribs20->lastAccessTimeSecs = 546463;
   settableFileAttribs20->mode = 882;
   settableFileAttribs20->modificationTimeSecs = 45446;
   settableFileAttribs20->userID = 556;
   size_t numTargets20 = 3;
   StatData* statData20 = new StatData(95483, settableFileAttribs20, 35465, 4556, 78, 62);
   statData20->setSparseFlag();
   UInt16Vector* targets20 = new UInt16Vector(numTargets20);
   TestSerialization::addRandomValuesToUInt16Vector(targets20, 1);
   targets20->at(1) = std::numeric_limits<uint16_t>::max();
   targets20->at(2) = 0;
   TestSerialization::addRandomTargetChunkBlocksToStatData(statData20, numTargets20);
   Raid0Pattern* stripePattern20 = new Raid0Pattern(2048, *targets20, numTargets20);
   HsmFileMetaData* hsmFileMetaData20 = new HsmFileMetaData(789, 22);
   std::string* entryID20 = new std::string("9-AF891DC1-1");
   std::string* origParentEntryID20 = new std::string("4-DFA4512F-7");
   FileInodeStoreData* fileInodeStoreData20 = new FileInodeStoreData(*entryID20, statData20,
      stripePattern20, 0, 2331, *origParentEntryID20, FileInodeOrigFeature_TRUE);
   fileInodeStoreData20->setHsmFileMetaData(*hsmFileMetaData20);
   FileInode* inode20 = new FileInode(*entryID20, fileInodeStoreData20, DirEntryType_REGULARFILE,
      DENTRY_FEATURE_IS_FILEINODE);
   inode20->setIsInlinedUnlocked(true);
   inode20->initLocksRandomForSerializationTests();
   EntryInfo* entryInfo20 = new EntryInfo(331, *origParentEntryID20, *entryID20, "testfile20",
      DirEntryType_REGULARFILE, INT_MIN);

   SettableFileAttribs* settableFileAttribs21 = new SettableFileAttribs();
   settableFileAttribs21->groupID = 45;
   settableFileAttribs21->lastAccessTimeSecs = 5416131;
   settableFileAttribs21->mode = 533;
   settableFileAttribs21->modificationTimeSecs = 5441363;
   settableFileAttribs21->userID = 5823;
   size_t numTargets21 = 6;
   StatData* statData21 = new StatData(854721, settableFileAttribs21, 62685, 756416, 7864, 225);
   statData21->setSparseFlag();
   UInt16Vector* targets21 = new UInt16Vector(numTargets21);
   TestSerialization::addRandomValuesToUInt16Vector(targets21, numTargets21);
   UInt16Vector* mirrorTargets21 = new UInt16Vector(numTargets21);
   TestSerialization::addRandomValuesToUInt16Vector(mirrorTargets21, numTargets21);
   TestSerialization::addRandomTargetChunkBlocksToStatData(statData21, numTargets21);
   Raid10Pattern* stripePattern21 = new Raid10Pattern (512, *targets21, *mirrorTargets21,
      numTargets21);
   HsmFileMetaData* hsmFileMetaData21 = new HsmFileMetaData(789, 1321);
   std::string* entryID21 = new std::string("0-34A23FAB-6");
   std::string* origParentEntryID21 = new std::string("2-AFAF3444-6");
   FileInodeStoreData* fileInodeStoreData21 = new FileInodeStoreData(*entryID21, statData21,
      stripePattern21, UINT_MAX, 2890, *origParentEntryID21, FileInodeOrigFeature_TRUE);
   fileInodeStoreData21->setHsmFileMetaData(*hsmFileMetaData21);
   FileInode* inode21 = new FileInode(*entryID21, fileInodeStoreData21, DirEntryType_REGULARFILE,
      DENTRY_FEATURE_IS_FILEINODE);
   inode21->setIsInlinedUnlocked(false);
   inode21->initLocksRandomForSerializationTests();
   EntryInfo* entryInfo21 = new EntryInfo(890, *origParentEntryID21, *entryID21, "testfile21",
      DirEntryType_DIRECTORY, ENTRYINFO_FEATURE_INLINED);

   SettableFileAttribs* settableFileAttribs22 = new SettableFileAttribs();
   settableFileAttribs22->groupID = 7889;
   settableFileAttribs22->lastAccessTimeSecs = 1316313;
   settableFileAttribs22->mode = 5263;
   settableFileAttribs22->modificationTimeSecs = 1313416;
   settableFileAttribs22->userID = 5555;
   size_t numTargets22 = 42;
   StatData* statData22 = new StatData(9613463, settableFileAttribs22, 7646416, 6116546, 788, 4565);
   statData22->setSparseFlag();
   UInt16Vector* targets22 = new UInt16Vector(numTargets22);
   TestSerialization::addRandomValuesToUInt16Vector(targets22, numTargets22);
   TestSerialization::addRandomTargetChunkBlocksToStatData(statData22, numTargets22);
   BuddyMirrorPattern* stripePattern22 = new BuddyMirrorPattern (1024, *targets22, numTargets22);
   HsmFileMetaData* hsmFileMetaData22 = new HsmFileMetaData(789, 12354);
   std::string* entryID22 = new std::string("1-34F23DF1-5");
   std::string* origParentEntryID22 = new std::string("8-734AF123-1");
   FileInodeStoreData* fileInodeStoreData22 = new FileInodeStoreData(*entryID22, statData22,
      stripePattern22, FILEINODE_FEATURE_HAS_ORIG_UID, UINT_MAX, *origParentEntryID22,
      FileInodeOrigFeature_TRUE);
   fileInodeStoreData22->setHsmFileMetaData(*hsmFileMetaData22);
   FileInode* inode22 = new FileInode(*entryID22, fileInodeStoreData22, DirEntryType_REGULARFILE,
      DENTRY_FEATURE_IS_FILEINODE);
   inode22->setIsInlinedUnlocked(true);
   inode22->initLocksRandomForSerializationTests();
   EntryInfo* entryInfo22 = new EntryInfo(453, *origParentEntryID22, *entryID22, "testfile22",
      DirEntryType_SYMLINK, ENTRYINFO_FEATURE_INLINED);


   SettableFileAttribs* settableFileAttribs30 = new SettableFileAttribs();
   settableFileAttribs30->groupID = 5646;
   settableFileAttribs30->lastAccessTimeSecs = 11566300;
   settableFileAttribs30->mode = 556;
   settableFileAttribs30->modificationTimeSecs = 564633;
   settableFileAttribs30->userID = 44;
   size_t numTargets30 = 111;
   StatData* statData30 = new StatData(73663, settableFileAttribs30, 555, 874496, 346, 798);
   statData30->setSparseFlag();
   UInt16Vector* targets30 = new UInt16Vector(numTargets30);
   TestSerialization::addRandomValuesToUInt16Vector(targets30, numTargets30);
   TestSerialization::addRandomTargetChunkBlocksToStatData(statData30, numTargets30);
   Raid0Pattern* stripePattern30 = new Raid0Pattern (4096, *targets30, numTargets30);
   HsmFileMetaData* hsmFileMetaData30 = new HsmFileMetaData(12236, 7866);
   std::string* entryID30 = new std::string("2-893ADF21-9");
   std::string* origParentEntryID30 = new std::string("5-DA474213-8");
   FileInodeStoreData* fileInodeStoreData30 = new FileInodeStoreData(*entryID30, statData30,
      stripePattern30, FILEINODE_FEATURE_HAS_ORIG_UID, 3121, *origParentEntryID30,
      FileInodeOrigFeature_TRUE);
   fileInodeStoreData30->setHsmFileMetaData(*hsmFileMetaData30);
   FileInode* inode30 = new FileInode(*entryID30, fileInodeStoreData30, DirEntryType_REGULARFILE,
      DENTRY_FEATURE_IS_FILEINODE);
   inode30->setIsInlinedUnlocked(true);
   inode30->initLocksRandomForSerializationTests();
   EntryInfo* entryInfo30 = new EntryInfo(std::numeric_limits<uint16_t>::max(),
      *origParentEntryID30, *entryID30, "testfile30", DirEntryType_REGULARFILE, INT_MAX);



   SessionFile* sessionFile10 = new SessionFile(inode10, OPENFILE_ACCESS_WRITE |
      OPENFILE_ACCESS_SYNC, entryInfo10);
   sessionFile10->setSessionID(2344);
   sessionFile10->setUseAsyncCleanup();

   SessionFile* sessionFile11 = new SessionFile(inode11, OPENFILE_ACCESS_READWRITE, entryInfo11);
   sessionFile11->setSessionID(465);

   SessionFile* sessionFile12 = new SessionFile(inode12, OPENFILE_ACCESS_READ |
      OPENFILE_ACCESS_TRUNC, entryInfo12);
   sessionFile12->setSessionID(3599);

   SessionFile* sessionFile13 = new SessionFile(inode13, OPENFILE_ACCESS_WRITE |
      OPENFILE_ACCESS_APPEND, entryInfo13);
   sessionFile13->setSessionID(8836);
   sessionFile13->setUseAsyncCleanup();

   SessionFile* sessionFile14 = new SessionFile(inode14, OPENFILE_ACCESS_WRITE |
      OPENFILE_ACCESS_DIRECT, entryInfo14);
   sessionFile14->setSessionID(110);


   SessionFile* sessionFile20 = new SessionFile(inode20, OPENFILE_ACCESS_WRITE |
      OPENFILE_ACCESS_APPEND, entryInfo20);
   sessionFile20->setSessionID(0);

   SessionFile* sessionFile21 = new SessionFile(inode21, OPENFILE_ACCESS_READ, entryInfo21);
   sessionFile21->setSessionID(11);
   sessionFile21->setUseAsyncCleanup();

   SessionFile* sessionFile22 = new SessionFile(inode22, OPENFILE_ACCESS_READWRITE, entryInfo22);
   sessionFile22->setSessionID(7846);

   SessionFile* sessionFile30 = new SessionFile(inode30, OPENFILE_ACCESS_WRITE |
      OPENFILE_ACCESS_SYNC, entryInfo30);
   sessionFile30->setSessionID(UINT_MAX);



   Session* testSession1 = new Session("0815");
   testSession1->getFiles()->addSession(sessionFile10);
   testSession1->getFiles()->addSession(sessionFile11);
   testSession1->getFiles()->addSession(sessionFile12);
   testSession1->getFiles()->addSession(sessionFile13);
   testSession1->getFiles()->addSession(sessionFile14);

   Session* testSession2 = new Session("1024");
   testSession2->getFiles()->addSession(sessionFile20);
   testSession2->getFiles()->addSession(sessionFile21);
   testSession2->getFiles()->addSession(sessionFile22);

   Session* testSession3 = new Session("8122");
   testSession3->getFiles()->addSession(sessionFile30);


   testSessionStore.addSession(testSession1);
   testSessionStore.addSession(testSession2);
   testSessionStore.addSession(testSession3);
}

void TestSerialization::addRandomValuesToUInt16Vector(UInt16Vector* vector, size_t count)
{
   Random rand;

   for(size_t i = 0; i < count; i++)
   {
      uint16_t value = rand.getNextInRange(0, std::numeric_limits<uint16_t>::max() );
      vector->at(i) = value;
   }
}

void TestSerialization::addRandomTargetChunkBlocksToStatData(StatData* statData, size_t count)
{
   Random rand;

   for(size_t i = 0; i < count; i++)
   {
      uint64_t value = rand.getNextInt();
      statData->setTargetChunkBlocks(i, value, count);
   }
}

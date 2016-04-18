#include "TestSerialization.h"

#include <common/storage/EntryInfo.h>
#include <common/storage/striping/Raid0Pattern.h>

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

#include "TestSerialization.h"

#include <common/storage/StorageTargetInfo.h>
      void testStorageTargetInfoSerialization();
      void testStorageTargetInfoListSerialization();
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

void TestSerialization::testStorageTargetInfoSerialization()
{
   log.log(Log_DEBUG, "testStorageTargetInfoSerialization started");

   std::string pathStr = "/path/to/target";
   StorageTargetInfo objectIn(1234, pathStr, 654321, 123456, 98765, 56789,
         TargetConsistencyState_GOOD);
   StorageTargetInfo objectOut;

   size_t bufLen = objectIn.serialLen();
   char* buf = (char*) malloc(bufLen);
   unsigned serialOutLen;

   objectIn.serialize(buf);

   bool deserRes = objectOut.deserialize(buf, bufLen, &serialOutLen);

   if (!deserRes)
   {
      CPPUNIT_FAIL("Deserialization failed");
   }

   CPPUNIT_ASSERT(objectIn == objectOut);

   free(buf);

   log.log(Log_DEBUG, "testStorageTargetInfoSerialization finished");
}

void TestSerialization::testStorageTargetInfoListSerialization()
{
   log.log(Log_DEBUG, "testStorageTargetInfoListSerialization started");

   StorageTargetInfoList listIn;
   StorageTargetInfoList listOut;

   for (unsigned i = 0; i < 10; i++)
   {
      std::string pathStr = "/path/to/target" + StringTk::uintToStr(i);
      StorageTargetInfo object(i, pathStr, i*100, i*50, i*200, i*150,
            TargetConsistencyState_GOOD);
      listIn.push_back(object);
   }

   size_t bufLen = Serialization::serialLenStorageTargetInfoList(&listIn);
   char* buf = (char*) malloc(bufLen);

   Serialization::serializeStorageTargetInfoList(buf, &listIn);

   const char* infoStart;
   unsigned infoElemNum;
   unsigned infoLen;

   Serialization::deserializeStorageTargetInfoListPreprocess(buf, bufLen, &infoStart, &infoElemNum,
      &infoLen);

   Serialization::deserializeStorageTargetInfoList(infoElemNum, infoStart, &listOut);

   free(buf);

   CPPUNIT_ASSERT(listIn.size() == listOut.size());

   StorageTargetInfoListIter iterIn = listIn.begin();
   StorageTargetInfoListIter iterOut = listOut.begin();

   for ( ; iterIn != listIn.end(); iterIn++, iterOut++)
      CPPUNIT_ASSERT(*iterIn == *iterOut);

   log.log(Log_DEBUG, "testStorageTargetInfoListSerialization finished");
}

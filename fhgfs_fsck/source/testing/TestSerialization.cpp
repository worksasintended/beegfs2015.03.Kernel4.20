#include "TestSerialization.h"

#include <common/net/message/nodes/HeartbeatRequestMsg.h>
#include <common/net/message/nodes/HeartbeatMsg.h>
#include <common/toolkit/serialization/SerializationFsck.h>
#include <net/message/NetMessageFactory.h>
#include <toolkit/DatabaseTk.h>

TestSerialization::TestSerialization()
{
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

void TestSerialization::testFsckDirEntrySerialization()
{
   FsckDirEntry dirEntryIn;
   FsckDirEntry dirEntryOut;

   dirEntryIn = DatabaseTk::createDummyFsckDirEntry();

   char* buf;
   size_t bufLen;

   // serialize
   bufLen = dirEntryIn.serialLen();
   buf = (char*) malloc(bufLen);
   dirEntryIn.serialize(buf);

   // deserialize
   unsigned outBufLen;
   if (! dirEntryOut.deserialize(buf, bufLen, &outBufLen) )
      CPPUNIT_FAIL("Deserialization failed.");

   SAFE_FREE(buf);

   CPPUNIT_ASSERT (dirEntryIn == dirEntryOut);
}

void TestSerialization::testFsckDirEntryListSerialization()
{
   uint NUM_ENTRIES = 5;

   FsckDirEntryList dirEntryListIn;

   DatabaseTk::createDummyFsckDirEntries(NUM_ENTRIES, &dirEntryListIn);

   FsckDirEntryList dirEntryListOut;

   char* buf;
   size_t bufLen;

   const char* dirEntryListStart;
   unsigned dirEntryListElemNum;
   unsigned dirEntryListBufLen;

   // serialize
   bufLen = SerializationFsck::serialLenFsckDirEntryList(&dirEntryListIn);

   buf = (char*) malloc(bufLen);

   SerializationFsck::serializeFsckDirEntryList(buf, &dirEntryListIn);

   // deserialize
   SerializationFsck::deserializeFsckDirEntryListPreprocess(buf, bufLen, &dirEntryListStart,
      &dirEntryListElemNum, &dirEntryListBufLen);

   SerializationFsck::deserializeFsckDirEntryList(dirEntryListElemNum, dirEntryListStart,
      &dirEntryListOut);

   SAFE_FREE(buf);

   // check sizes
   CPPUNIT_ASSERT( dirEntryListIn.size() == dirEntryListOut.size() );

   FsckDirEntryListIter iterIn = dirEntryListIn.begin();
   FsckDirEntryListIter iterOut = dirEntryListOut.begin();

   while ( iterIn != dirEntryListIn.end() )
   {
      FsckDirEntry dirEntryIn = *iterIn;
      FsckDirEntry dirEntryOut = *iterOut;

      CPPUNIT_ASSERT (dirEntryIn == dirEntryOut);
      iterIn++;
      iterOut++;
   }
}

void TestSerialization::testFsckDirInodeSerialization()
{
   FsckDirInode dirInodeIn;
   FsckDirInode dirInodeOut;

   dirInodeIn = DatabaseTk::createDummyFsckDirInode();

   char* buf;
   size_t bufLen;

   // serialize
   bufLen = dirInodeIn.serialLen();
   buf = (char*) malloc(bufLen);
   dirInodeIn.serialize(buf);

   // deserialize
   unsigned outBufLen;
   if (! dirInodeOut.deserialize(buf, bufLen, &outBufLen) )
      CPPUNIT_FAIL("Deserialization failed.");

   SAFE_FREE(buf);

   CPPUNIT_ASSERT (dirInodeIn == dirInodeOut);
}

void TestSerialization::testFsckDirInodeListSerialization()
{
   uint NUM_ENTRIES = 5;

   FsckDirInodeList dirInodeListIn;

   DatabaseTk::createDummyFsckDirInodes(NUM_ENTRIES, &dirInodeListIn);

   FsckDirInodeList dirInodeListOut;

   char* buf;
   size_t bufLen;

   const char* dirInodeListStart;
   unsigned dirInodeListElemNum;
   unsigned dirInodeListBufLen;

   // serialize
   bufLen = SerializationFsck::serialLenFsckDirInodeList(&dirInodeListIn);

   buf = (char*) malloc(bufLen);

   SerializationFsck::serializeFsckDirInodeList(buf, &dirInodeListIn);

   // deserialize
   SerializationFsck::deserializeFsckDirInodeListPreprocess(buf, bufLen, &dirInodeListStart,
      &dirInodeListElemNum, &dirInodeListBufLen);

   SerializationFsck::deserializeFsckDirInodeList(dirInodeListElemNum, dirInodeListStart,
      &dirInodeListOut);

   SAFE_FREE(buf);

   // check sizes
   CPPUNIT_ASSERT( dirInodeListIn.size() == dirInodeListOut.size() );

   FsckDirInodeListIter iterIn = dirInodeListIn.begin();
   FsckDirInodeListIter iterOut = dirInodeListOut.begin();

   while ( iterIn != dirInodeListIn.end() )
   {
      FsckDirInode dirInodeIn = *iterIn;
      FsckDirInode dirInodeOut = *iterOut;

      CPPUNIT_ASSERT (dirInodeIn == dirInodeOut);
      iterIn++;
      iterOut++;
   }
}

void TestSerialization::testFsckFileInodeSerialization()
{
   FsckFileInode fileInodeIn;
   FsckFileInode fileInodeOut;

   fileInodeIn = DatabaseTk::createDummyFsckFileInode();

   char* buf;
   size_t bufLen;

   // serialize
   bufLen = fileInodeIn.serialLen();
   buf = (char*) malloc(bufLen);
   fileInodeIn.serialize(buf);

   // deserialize
   unsigned outBufLen;
   if (! fileInodeOut.deserialize(buf, bufLen, &outBufLen) )
      CPPUNIT_FAIL("Deserialization failed.");

   SAFE_FREE(buf);

   CPPUNIT_ASSERT (fileInodeIn == fileInodeOut);
}

void TestSerialization::testFsckFileInodeListSerialization()
{
   uint NUM_ENTRIES = 5;

   FsckFileInodeList fileInodeListIn;

   DatabaseTk::createDummyFsckFileInodes(NUM_ENTRIES, &fileInodeListIn);

   FsckFileInodeList fileInodeListOut;

   char* buf;
   size_t bufLen;

   const char* fileInodeListStart;
   unsigned fileInodeListElemNum;
   unsigned fileInodeListBufLen;

   // serialize
   bufLen = SerializationFsck::serialLenFsckFileInodeList(&fileInodeListIn);

   buf = (char*) malloc(bufLen);

   SerializationFsck::serializeFsckFileInodeList(buf, &fileInodeListIn);

   // deserialize
   SerializationFsck::deserializeFsckFileInodeListPreprocess(buf, bufLen, &fileInodeListStart,
      &fileInodeListElemNum, &fileInodeListBufLen);

   SerializationFsck::deserializeFsckFileInodeList(fileInodeListElemNum, fileInodeListStart,
      &fileInodeListOut);

   SAFE_FREE(buf);

   // check sizes
   CPPUNIT_ASSERT( fileInodeListIn.size() == fileInodeListOut.size() );

   FsckFileInodeListIter iterIn = fileInodeListIn.begin();
   FsckFileInodeListIter iterOut = fileInodeListOut.begin();

   while ( iterIn != fileInodeListIn.end() )
   {
      FsckFileInode fileInodeIn = *iterIn;
      FsckFileInode fileInodeOut = *iterOut;

      CPPUNIT_ASSERT (fileInodeIn == fileInodeOut);
      iterIn++;
      iterOut++;
   }
}

void TestSerialization::testFsckChunkSerialization()
{
   FsckChunk chunkIn;
   FsckChunk chunkOut;

   chunkIn = DatabaseTk::createDummyFsckChunk();

   char* buf;
   size_t bufLen;

   // serialize
   bufLen = chunkIn.serialLen();
   buf = (char*) malloc(bufLen);
   chunkIn.serialize(buf);

   // deserialize
   unsigned outBufLen;
   if (! chunkOut.deserialize(buf, bufLen, &outBufLen) )
      CPPUNIT_FAIL("Deserialization failed.");

   SAFE_FREE(buf);

   CPPUNIT_ASSERT (chunkIn == chunkOut);
}

void TestSerialization::testFsckChunkListSerialization()
{
   uint NUM_ENTRIES = 5;

   FsckChunkList chunkListIn;

   DatabaseTk::createDummyFsckChunks(NUM_ENTRIES, &chunkListIn);

   FsckChunkList chunkListOut;

   char* buf;
   size_t bufLen;

   const char* chunkListStart;
   unsigned chunkListElemNum;
   unsigned chunkListBufLen;

   // serialize
   bufLen = SerializationFsck::serialLenFsckChunkList(&chunkListIn);

   buf = (char*) malloc(bufLen);

   SerializationFsck::serializeFsckChunkList(buf, &chunkListIn);

   // deserialize
   SerializationFsck::deserializeFsckChunkListPreprocess(buf, bufLen, &chunkListStart,
      &chunkListElemNum, &chunkListBufLen);

   SerializationFsck::deserializeFsckChunkList(chunkListElemNum, chunkListStart,
      &chunkListOut);

   SAFE_FREE(buf);

   // check sizes
   CPPUNIT_ASSERT( chunkListIn.size() == chunkListOut.size() );

   FsckChunkListIter iterIn = chunkListIn.begin();
   FsckChunkListIter iterOut = chunkListOut.begin();

   while ( iterIn != chunkListIn.end() )
   {
      FsckChunk chunkIn = *iterIn;
      FsckChunk chunkOut = *iterOut;

      CPPUNIT_ASSERT (chunkIn == chunkOut);
      iterIn++;
      iterOut++;
   }
}

void TestSerialization::testFsckContDirSerialization()
{
   FsckContDir contDirIn;
   FsckContDir contDirOut;

   contDirIn = DatabaseTk::createDummyFsckContDir();

   char* buf;
   size_t bufLen;

   // serialize
   bufLen = contDirIn.serialLen();
   buf = (char*) malloc(bufLen);
   contDirIn.serialize(buf);

   // deserialize
   unsigned outBufLen;
   if (! contDirOut.deserialize(buf, bufLen, &outBufLen) )
      CPPUNIT_FAIL("Deserialization failed.");

   SAFE_FREE(buf);

   CPPUNIT_ASSERT (contDirIn == contDirOut);
}

void TestSerialization::testFsckContDirListSerialization()
{
   uint NUM_ENTRIES = 5;

   FsckContDirList contDirListIn;

   DatabaseTk::createDummyFsckContDirs(NUM_ENTRIES, &contDirListIn);

   FsckContDirList contDirListOut;

   char* buf;
   size_t bufLen;

   const char* contDirListStart;
   unsigned contDirListElemNum;
   unsigned contDirListBufLen;

   // serialize
   bufLen = SerializationFsck::serialLenFsckContDirList(&contDirListIn);

   buf = (char*) malloc(bufLen);

   SerializationFsck::serializeFsckContDirList(buf, &contDirListIn);

   // deserialize
   SerializationFsck::deserializeFsckContDirListPreprocess(buf, bufLen, &contDirListStart,
      &contDirListElemNum, &contDirListBufLen);

   SerializationFsck::deserializeFsckContDirList(contDirListElemNum, contDirListStart,
      &contDirListOut);

   SAFE_FREE(buf);

   // check sizes
   CPPUNIT_ASSERT( contDirListIn.size() == contDirListOut.size() );

   FsckContDirListIter iterIn = contDirListIn.begin();
   FsckContDirListIter iterOut = contDirListOut.begin();

   while ( iterIn != contDirListIn.end() )
   {
      FsckContDir contDirIn = *iterIn;
      FsckContDir contDirOut = *iterOut;

      CPPUNIT_ASSERT (contDirIn == contDirOut);
      iterIn++;
      iterOut++;
   }
}

void TestSerialization::testFsckFsIDSerialization()
{
   FsckFsID fsIDIn;
   FsckFsID fsIDOut;

   fsIDIn = DatabaseTk::createDummyFsckFsID();

   char* buf;
   size_t bufLen;

   // serialize
   bufLen = fsIDIn.serialLen();
   buf = (char*) malloc(bufLen);
   fsIDIn.serialize(buf);

   // deserialize
   unsigned outBufLen;
   if (! fsIDOut.deserialize(buf, bufLen, &outBufLen) )
      CPPUNIT_FAIL("Deserialization failed.");

   SAFE_FREE(buf);

   CPPUNIT_ASSERT (fsIDIn == fsIDOut);
}

void TestSerialization::testFsckFsIDListSerialization()
{
   uint NUM_ENTRIES = 5;

   FsckFsIDList fsIDListIn;

   DatabaseTk::createDummyFsckFsIDs(NUM_ENTRIES, &fsIDListIn);

   FsckFsIDList fsIDListOut;

   char* buf;
   size_t bufLen;

   const char* listStart;
   unsigned listElemNum;
   unsigned listBufLen;

   // serialize
   bufLen = SerializationFsck::serialLenFsckFsIDList(&fsIDListIn);

   buf = (char*) malloc(bufLen);

   SerializationFsck::serializeFsckFsIDList(buf, &fsIDListIn);

   // deserialize
   SerializationFsck::deserializeFsckFsIDListPreprocess(buf, bufLen, &listStart,
      &listElemNum, &listBufLen);

   SerializationFsck::deserializeFsckFsIDList(listElemNum, listStart, &fsIDListOut);

   SAFE_FREE(buf);

   // check sizes
   CPPUNIT_ASSERT( fsIDListIn.size() == fsIDListOut.size() );

   FsckFsIDListIter iterIn = fsIDListIn.begin();
   FsckFsIDListIter iterOut = fsIDListOut.begin();

   while ( iterIn != fsIDListIn.end() )
   {
      FsckFsID fsIDIn = *iterIn;
      FsckFsID fsIDOut = *iterOut;

      CPPUNIT_ASSERT (fsIDIn == fsIDOut);
      iterIn++;
      iterOut++;
   }
}

void TestSerialization::testUInt16Serialization()
{
   uint16_t valueIn = 1000;
   uint16_t valueOut;

   char* buf;
   size_t bufLen;

   // serialize
   bufLen = Serialization::serialLenUShort();

   buf = (char*) malloc(bufLen);

   Serialization::serializeUShort(buf, valueIn);

   // deserialize
   unsigned valueOutLen;
   Serialization::deserializeUShort(buf, bufLen, &valueOut, &valueOutLen);

   SAFE_FREE(buf);

   CPPUNIT_ASSERT( valueIn == valueOut);
}

void TestSerialization::testUInt16ListSerialization()
{
   UInt16List valuesIn;
   valuesIn.push_back(1000);
   valuesIn.push_back(2000);
   valuesIn.push_back(3000);
   valuesIn.push_back(4000);
   valuesIn.push_back(5000);

   UInt16List valuesOut;

   char* buf;
   size_t bufLen;

   const char* valuesOutStart;
   unsigned valuesOutElemNum;
   unsigned valuesOutBufLen;

   // serialize
   bufLen = Serialization::serialLenUInt16List(&valuesIn);

   buf = (char*) malloc(bufLen);

   Serialization::serializeUInt16List(buf, &valuesIn);

   // deserialize
   Serialization::deserializeUInt16ListPreprocess(buf, bufLen, &valuesOutElemNum, &valuesOutStart,
      &valuesOutBufLen);

   Serialization::deserializeUInt16List(valuesOutBufLen, valuesOutElemNum, valuesOutStart,
      &valuesOut);

   SAFE_FREE(buf);

   CPPUNIT_ASSERT( valuesIn == valuesOut);
}

void TestSerialization::testUInt16VectorSerialization()
{
   UInt16Vector valuesIn;
   valuesIn.push_back(1000);
   valuesIn.push_back(2000);
   valuesIn.push_back(3000);
   valuesIn.push_back(4000);
   valuesIn.push_back(5000);

   UInt16Vector valuesOut;

   char* buf;
   size_t bufLen;

   const char* valuesOutStart;
   unsigned valuesOutElemNum;
   unsigned valuesOutBufLen;

   // serialize
   bufLen = Serialization::serialLenUInt16Vector(&valuesIn);

   buf = (char*) malloc(bufLen);

   Serialization::serializeUInt16Vector(buf, &valuesIn);

   // deserialize
   Serialization::deserializeUInt16VectorPreprocess(buf, bufLen, &valuesOutElemNum, &valuesOutStart,
      &valuesOutBufLen);

   Serialization::deserializeUInt16Vector(valuesOutBufLen, valuesOutElemNum, valuesOutStart,
      &valuesOut);

   SAFE_FREE(buf);

   CPPUNIT_ASSERT( valuesIn == valuesOut);
}

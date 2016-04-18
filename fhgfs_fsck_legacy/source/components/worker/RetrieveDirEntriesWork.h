#ifndef RETRIEVEDIRENTRIESWORK_H
#define RETRIEVEDIRENTRIESWORK_H

/*
 * retrieve all dir entries from one node, inside a specified range of hashDirs and save them to DB
 */

#include <common/app/log/LogContext.h>
#include <common/components/worker/Work.h>
#include <components/DataFetcherBuffer.h>
#include <common/toolkit/SynchronizedCounter.h>

#include <database/FsckDB.h>

// the size of one response packet, i.e. how many dentries are asked for at once
#define RETRIEVE_DIR_ENTRIES_PACKET_SIZE 500

#define MAX_BUFFER_SIZE 10000

class RetrieveDirEntriesWork : public Work
{
   public:
      /*
       * @param node the node to retrieve data from
       * @param counter a pointer to a Synchronized counter; this is incremented by one at the end
       * and the calling thread can wait for the counter
       * @param hashDirStart the first top-level hashDir to open
       * @param hashDirEnd the last top-level hashDir to open
       * @param numDentriesFound
       * @param numFileInodesFound
       */
      RetrieveDirEntriesWork(Node* node, SynchronizedCounter* counter, unsigned hashDirStart,
         unsigned hashDirEnd, AtomicUInt64* numDentriesFound, AtomicUInt64* numFileInodesFound);
      virtual ~RetrieveDirEntriesWork();
      void process(char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen);

   private:
      LogContext log;
      Node* node;
      SynchronizedCounter* counter;
      AtomicUInt64* numDentriesFound;
      AtomicUInt64* numFileInodesFound;

      unsigned hashDirStart;
      unsigned hashDirEnd;

      DataFetcherBuffer<FsckDirEntry> bufferDirEntries;
      DataFetcherBuffer<FsckFileInode> bufferFileInodes;
      DataFetcherBuffer<FsckTargetID> bufferUsedTargets;
      DataFetcherBuffer<FsckContDir> bufferContDirs;

      void doWork();
      void flushDirEntries();
      void flushFileInodes();
      void flushUsedTargets();
      void flushContDirs();
};

#endif /* RETRIEVEDIRENTRIESWORK_H */

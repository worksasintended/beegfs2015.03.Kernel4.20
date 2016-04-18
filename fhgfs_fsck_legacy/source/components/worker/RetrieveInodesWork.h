#ifndef RETRIEVEINODESWORK_H
#define RETRIEVEINODESWORK_H

/*
 * retrieve all inodes from one node, inside a specified range of hashDirs and save them to DB

 */

#include <common/app/log/LogContext.h>
#include <common/components/worker/Work.h>
#include <common/toolkit/SynchronizedCounter.h>
#include <components/DataFetcherBuffer.h>
#include <database/FsckDB.h>

// the size of one response packet, i.e. how many inodes are asked for at once
#define RETRIEVE_INODES_PACKET_SIZE 500
#define MAX_BUFFER_SIZE 10000

class RetrieveInodesWork : public Work
{
   public:
      /*
       * @param node the node to retrieve data from
       * @param counter a pointer to a Synchronized counter; this is incremented by one at the end
       * and the calling thread can wait for the counter
       * @param hashDirStart the first top-level hashDir to open
       * @param hashDirEnd the last top-level hashDir to open
       */
      RetrieveInodesWork(Node* node, SynchronizedCounter* counter, unsigned hashDirStart,
         unsigned hashDirEnd, AtomicUInt64* numFileInodesFound, AtomicUInt64* numDirInodesFound);
      virtual ~RetrieveInodesWork();
      void process(char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen);

   private:
      LogContext log;
      Node* node;
      SynchronizedCounter* counter;

      unsigned hashDirStart;
      unsigned hashDirEnd;

      AtomicUInt64* numFileInodesFound;
      AtomicUInt64* numDirInodesFound;

      DataFetcherBuffer<FsckFileInode> bufferFileInodes;
      DataFetcherBuffer<FsckDirInode> bufferDirInodes;
      DataFetcherBuffer<FsckTargetID> bufferUsedTargets;

      void doWork();
      void flushFileInodes();
      void flushDirInodes();
      void flushUsedTargets();
};

#endif /* RETRIEVEINODESWORK_H */

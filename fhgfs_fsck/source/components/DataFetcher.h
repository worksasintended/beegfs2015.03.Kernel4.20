#ifndef DATAFETCHER_H_
#define DATAFETCHER_H_

#include <common/components/worker/queue/MultiWorkQueue.h>
#include <common/toolkit/SynchronizedCounter.h>
#include <database/FsckDB.h>

#define DATAFETCHER_OUTPUT_INTERVAL_MS 2000

#define DATAFETCHER_FETCHMODE_DIRENTRIES        1
#define DATAFETCHER_FETCHMODE_INODES            2
#define DATAFETCHER_FETCHMODE_CHUNKS            4
/* #define DATAFETCHER_FETCHMODE_MIRROR_CHUNKS     8 */
#define DATAFETCHER_FETCHMODE_ALL               7


class DataFetcher
{
   private:
      FsckDB* database;

      MultiWorkQueue* workQueue;

      AtomicUInt64 numDentriesFound;
      AtomicUInt64 numFileInodesFound;
      AtomicUInt64 numDirInodesFound;
      AtomicUInt64 numChunksFound;

   public:
      DataFetcher();
      virtual ~DataFetcher();

      bool execute(unsigned short fetchMode = DATAFETCHER_FETCHMODE_ALL);

   private:
      SynchronizedCounter finishedPackages;
      unsigned generatedPackages;

      void retrieveDirEntries(NodeList* nodeList);
      void retrieveInodes(NodeList* nodeList);
      void retrieveChunks();

      void printStatus(bool toLogFile = false);
};

#endif /* DATAFETCHER_H_ */

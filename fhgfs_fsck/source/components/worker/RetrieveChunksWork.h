#ifndef RETRIEVECHUNKSWORK_H
#define RETRIEVECHUNKSWORK_H

/*
 * retrieve all chunks from one storage server and save them to DB
 */

#include <common/app/log/LogContext.h>
#include <common/components/worker/Work.h>
#include <common/toolkit/SynchronizedCounter.h>
#include <database/FsckDB.h>
#include <database/FsckDBTable.h>

// the size of one response packet, i.e. how many chunks are asked for at once
#define RETRIEVE_CHUNKS_PACKET_SIZE 400

class RetrieveChunksWork : public Work
{
   public:
      /*
       * @param db database instance
       * @param node pointer to the node to retrieve data from
       * @param counter a pointer to a Synchronized counter; this is incremented by one at the end
       * and the calling thread can wait for the counter
       * @param numChunksFound
       */
      RetrieveChunksWork(FsckDB* db, Node* node, SynchronizedCounter* counter,
         AtomicUInt64* numChunksFound);
      virtual ~RetrieveChunksWork();
      void process(char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen);

   private:
      LogContext log;
      Node* node;
      SynchronizedCounter* counter;
      AtomicUInt64* numChunksFound;

      FsckDBChunksTable* chunks;
      FsckDBChunksTable::BulkHandle chunksHandle;

      void doWork();
};

#endif /* RETRIEVECHUNKSWORK_H */

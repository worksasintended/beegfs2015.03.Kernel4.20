#include "RetrieveChunksWork.h"

/* #include <common/net/message/fsck/RetrieveChunksMsg.h>
#include <common/net/message/fsck/RetrieveChunksRespMsg.h> */
#include <common/net/message/fsck/FetchFsckChunkListMsg.h>
#include <common/net/message/fsck/FetchFsckChunkListRespMsg.h>
#include <common/storage/Storagedata.h>
#include <common/toolkit/FsckTk.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/StorageTk.h>
#include <database/FsckDBException.h>
#include <toolkit/FsckException.h>

#include <program/Program.h>

RetrieveChunksWork::RetrieveChunksWork(Node* node, SynchronizedCounter* counter,
   AtomicUInt64* numChunksFound): Work()
{
   log.setContext("RetrieveChunksWork");
   this->node = node;
   this->counter = counter;
   this->numChunksFound = numChunksFound;
}

RetrieveChunksWork::~RetrieveChunksWork()
{
}

void RetrieveChunksWork::process(char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen)
{
   log.log(Log_DEBUG, "Processing RetrieveChunksWork");

   try
   {
      doWork();
      // work package finished => increment counter
      this->counter->incCount();

   }
   catch (std::exception& e)
   {
      // exception thrown, but work package is finished => increment counter
      this->counter->incCount();

      // after incrementing counter, re-throw exception
      throw;
   }

   log.log(Log_DEBUG, "Processed RetrieveChunksWork");
}

void RetrieveChunksWork::doWork()
{
   // take the node associated with the current target and send a RetrieveChunksMsg to
   // that node; the chunks are retrieved incrementally
   App* app = Program::getApp();
   NodeStore* storageNodes = app->getStorageNodes();

   if ( node )
   {
      std::string nodeID = node->getID();
      FetchFsckChunkListStatus status = FetchFsckChunkListStatus_NOTSTARTED;
      unsigned resultCount = 0;

      // (status == FetchFsckChunkListStatus_OK)
      do
      {
         bool commRes;
         char *respBuf = NULL;
         NetMessage *respMsg = NULL;

         FetchFsckChunkListMsg fetchFsckChunkListMsg(RETRIEVE_CHUNKS_PACKET_SIZE, status);

         commRes = MessagingTk::requestResponse(node, &fetchFsckChunkListMsg,
            NETMSGTYPE_FetchFsckChunkListResp, &respBuf, &respMsg);

         if (commRes)
         {
            FetchFsckChunkListRespMsg* fetchFsckChunkListRespMsg =
               (FetchFsckChunkListRespMsg*) respMsg;

            FsckChunkList chunks;
            fetchFsckChunkListRespMsg->parseChunkList(&chunks);
            resultCount = chunks.size();

            status = fetchFsckChunkListRespMsg->getStatus();

            SAFE_FREE(respBuf);
            SAFE_DELETE(respMsg);

            if (status == FetchFsckChunkListStatus_READERROR)
            {
               storageNodes->releaseNode(&node);
               throw FsckException("Read error occured while fetching chunks from node; nodeID: "
                  + nodeID);
            }

            if ( this->bufferChunks.add(chunks) > MAX_BUFFER_SIZE )
               this->flushChunks();

            numChunksFound->increase(resultCount);
         }
         else
         {
            storageNodes->releaseNode(&node);
            SAFE_FREE(respBuf);
            SAFE_DELETE(respMsg);

            throw FsckException("Communication error occured with node; nodeID: " + nodeID);
         }

         if ( Program::getApp()->getShallAbort() )
            break;

      } while ( (resultCount > 0) || (status == FetchFsckChunkListStatus_RUNNING) );

      storageNodes->releaseNode(&node);

      //flush remaining chunks in buffer
      this->flushChunks();
   }
   else
   {
      // basically this should never ever happen
      log.logErr("Requested node does not exist");
      throw FsckException("Requested node does not exist");
   }
}

void RetrieveChunksWork::flushChunks()
{
   FsckChunk failedInsert;
   int errorCode;

   bool success = this->bufferChunks.flush(failedInsert, errorCode);

   if ( !success )
   {
      log.logErr(
         "Failed to insert chunk with ID " + failedInsert.getID() + " from target "
            + StringTk::uintToStr(failedInsert.getTargetID()));
      log.logErr("SQLite Error was error code " + StringTk::intToStr(errorCode));
      throw FsckDBException(
         "Error while inserting chunks to database. Please see log for more information.");
   }
}

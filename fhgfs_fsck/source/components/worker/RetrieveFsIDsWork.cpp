#include "RetrieveFsIDsWork.h"
#include <common/net/message/fsck/RetrieveFsIDsMsg.h>
#include <common/net/message/fsck/RetrieveFsIDsRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/MetaStorageTk.h>
#include <database/FsckDBException.h>
#include <toolkit/FsckException.h>

#include <program/Program.h>


RetrieveFsIDsWork::RetrieveFsIDsWork(FsckDB* db, Node* node, SynchronizedCounter* counter,
   AtomicUInt64& errors, unsigned hashDirStart, unsigned hashDirEnd)
    : Work(),
      log("RetrieveFsIDsWork"),
      node(node),
      counter(counter),
      errors(&errors),
      hashDirStart(hashDirStart),
      hashDirEnd(hashDirEnd),
      table(db->getFsIDsTable() ),
      bulkHandle(table->newBulkHandle() )
{
}

RetrieveFsIDsWork::~RetrieveFsIDsWork()
{
}

void RetrieveFsIDsWork::process(char* bufIn, unsigned bufInLen, char* bufOut,
   unsigned bufOutLen)
{
   log.log(4, "Processing RetrieveFsIDsWork");

   try
   {
      doWork();
      table->flush(bulkHandle);
      // work package finished => increment counter
      this->counter->incCount();

   }
   catch (std::exception &e)
   {
      // exception thrown, but work package is finished => increment counter
      this->counter->incCount();

      // after incrementing counter, re-throw exception
      throw;
   }

   log.log(4, "Processed RetrieveFsIDsWork");
}

void RetrieveFsIDsWork::doWork()
{
   if ( node )
   {
      for ( unsigned firstLevelhashDirNum = hashDirStart; firstLevelhashDirNum <= hashDirEnd;
         firstLevelhashDirNum++ )
      {
         for ( unsigned secondLevelhashDirNum = 0;
            secondLevelhashDirNum < META_DENTRIES_LEVEL2_SUBDIR_NUM; secondLevelhashDirNum++ )
         {
            unsigned hashDirNum = StorageTk::mergeHashDirs(firstLevelhashDirNum,
               secondLevelhashDirNum);

            int64_t hashDirOffset = 0;
            int64_t contDirOffset = 0;
            std::string currentContDirID = "";
            int resultCount = 0;

            do
            {
               bool commRes;
               char *respBuf = NULL;
               NetMessage *respMsg = NULL;

               // TODO: Buddy Mirror
               RetrieveFsIDsMsg retrieveFsIDsMsg(hashDirNum, currentContDirID,
                  RETRIEVE_FSIDS_PACKET_SIZE, hashDirOffset, contDirOffset);

               commRes = MessagingTk::requestResponse(node, &retrieveFsIDsMsg,
                  NETMSGTYPE_RetrieveFsIDsResp, &respBuf, &respMsg);

               if ( commRes )
               {
                  RetrieveFsIDsRespMsg* retrieveFsIDsRespMsg =
                     (RetrieveFsIDsRespMsg*) respMsg;

                  // set new parameters
                  currentContDirID = retrieveFsIDsRespMsg->getCurrentContDirID();
                  hashDirOffset = retrieveFsIDsRespMsg->getNewHashDirOffset();
                  contDirOffset = retrieveFsIDsRespMsg->getNewContDirOffset();

                  // parse FS-IDs
                  FsckFsIDList fsIDs;
                  retrieveFsIDsRespMsg->parseFsIDs(&fsIDs);

                  // this is the actual result count we are interested in, because if no fsIDs
                  // were read, there is nothing left on the server
                  resultCount = fsIDs.size();

                  // check entry IDs
                  for (FsckFsIDListIter it = fsIDs.begin(); it != fsIDs.end(); )
                  {
                     if (db::EntryID::tryFromStr(it->getID()).first
                           && db::EntryID::tryFromStr(it->getParentDirID()).first)
                     {
                        ++it;
                        continue;
                     }

                     LogContext(__func__).logErr("Found fsid file with invalid entry IDs."
                           " node: " + StringTk::uintToStr(it->getSaveNodeID()) +
                           ", entryID: " + it->getID() +
                           ", parentEntryID: " + it->getParentDirID());

                     FsckFsIDListIter tmp = it;
                     ++it;
                     errors->increase();
                     fsIDs.erase(tmp);
                  }

                  this->table->insert(fsIDs, this->bulkHandle);

                  SAFE_FREE(respBuf);
                  SAFE_DELETE(respMsg);
               }
               else
               {
                  SAFE_FREE(respBuf);
                  SAFE_DELETE(respMsg);
                  throw FsckException("Communication error occured with node " + node->getID());
               }

               // if any of the worker threads threw an exception, we should stop now!
               if ( Program::getApp()->getShallAbort() )
                  return;

            } while ( resultCount > 0 );
         }
      }
   }
   else
   {
      // basically this should never ever happen
      log.logErr("Requested node does not exist");
      throw FsckException("Requested node does not exist");
   }
}

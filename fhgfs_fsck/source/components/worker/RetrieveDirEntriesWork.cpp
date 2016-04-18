#include "RetrieveDirEntriesWork.h"
#include <common/net/message/fsck/RetrieveDirEntriesMsg.h>
#include <common/net/message/fsck/RetrieveDirEntriesRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/MetaStorageTk.h>
#include <database/FsckDBException.h>
#include <toolkit/FsckException.h>

#include <program/Program.h>

RetrieveDirEntriesWork::RetrieveDirEntriesWork(Node* node, SynchronizedCounter* counter,
   unsigned hashDirStart, unsigned hashDirEnd, AtomicUInt64* numDentriesFound,
   AtomicUInt64* numFileInodesFound) : Work()
{
   log.setContext("RetrieveDirEntriesWork");
   this->node = node;
   this->counter = counter;
   this->hashDirStart = hashDirStart;
   this->hashDirEnd = hashDirEnd;
   this->numDentriesFound = numDentriesFound;
   this->numFileInodesFound = numFileInodesFound;
}

 RetrieveDirEntriesWork::~RetrieveDirEntriesWork()
{
}

void RetrieveDirEntriesWork::process(char* bufIn, unsigned bufInLen, char* bufOut,
   unsigned bufOutLen)
{
   log.log(4, "Processing RetrieveDirEntriesWork");

   try
   {
      doWork();
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

   log.log(4, "Processed RetrieveDirEntriesWork");
}

void RetrieveDirEntriesWork::doWork()
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

               RetrieveDirEntriesMsg retrieveDirEntriesMsg(hashDirNum, currentContDirID,
                  RETRIEVE_DIR_ENTRIES_PACKET_SIZE, hashDirOffset, contDirOffset);

               commRes = MessagingTk::requestResponse(node, &retrieveDirEntriesMsg,
                  NETMSGTYPE_RetrieveDirEntriesResp, &respBuf, &respMsg);

               if ( commRes )
               {
                  RetrieveDirEntriesRespMsg* retrieveDirEntriesRespMsg =
                     (RetrieveDirEntriesRespMsg*) respMsg;

                  // set new parameters
                  currentContDirID = retrieveDirEntriesRespMsg->getCurrentContDirID();
                  hashDirOffset = retrieveDirEntriesRespMsg->getNewHashDirOffset();
                  contDirOffset = retrieveDirEntriesRespMsg->getNewContDirOffset();

                  // parse directory entries
                  FsckDirEntryList dirEntries;
                  retrieveDirEntriesRespMsg->parseDirEntries(&dirEntries);
                  // this is the actual result count we are interested in, because if no dirEntries
                  // were read, there is nothing left on the server

                  resultCount = dirEntries.size();

                  // insert all dir entries to the buffer and, if MAX_BUFFER_SIZE is reached, flush
                  if ( this->bufferDirEntries.add(dirEntries) > MAX_BUFFER_SIZE )
                     this->flushDirEntries();

                  numDentriesFound->increase(resultCount);

                  // parse inlined file inodes
                  FsckFileInodeList inlinedFileInodes;
                  retrieveDirEntriesRespMsg->parseInlinedFileInodes(&inlinedFileInodes);

                  // insert all inlined inodes to the buffer and flush, if MAX_BUFFER_SIZE reached
                  if ( this->bufferFileInodes.add(inlinedFileInodes) > MAX_BUFFER_SIZE )
                     this->flushFileInodes();

                  numFileInodesFound->increase(inlinedFileInodes.size());

                  // add used targetIDs
                  // TODO: think about gathering targetIDs on the server side and sending them over
                  // as seperate list/set to avoid iterating here
                  for ( FsckFileInodeListIter iter = inlinedFileInodes.begin();
                     iter != inlinedFileInodes.end(); iter++ )
                  {
                     FsckTargetIDList targetIDs;
                     FsckTargetIDType fsckTargetIDType;

                     if (iter->getStripePatternType() == FsckStripePatternType_BUDDYMIRROR)
                        fsckTargetIDType = FsckTargetIDType_BUDDYGROUP;
                     else
                        fsckTargetIDType = FsckTargetIDType_TARGET;

                     for (UInt16VectorIter targetsIter = iter->getStripeTargets()->begin();
                        targetsIter != iter->getStripeTargets()->end(); targetsIter++)
                     {
                        FsckTargetID fsckTargetID(*targetsIter, fsckTargetIDType);
                        targetIDs.push_back(fsckTargetID);
                     }

                     if ( this->bufferUsedTargets.add(targetIDs, true) > MAX_BUFFER_SIZE )
                        this->flushUsedTargets();
                  }

                  // parse all new cont. directories
                  FsckContDirList contDirs;
                  retrieveDirEntriesRespMsg->parseContDirs(&contDirs);

                  // insert all contend dirs to the buffer and flush, if MAX_BUFFER_SIZE reached
                  if ( this->bufferContDirs.add(contDirs) > MAX_BUFFER_SIZE )
                     this->flushContDirs();

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

      // flush remaining entries in buffers
      this->flushDirEntries();
      this->flushFileInodes();
      this->flushUsedTargets();
      this->flushContDirs();
   }
   else
   {
      // basically this should never ever happen
      log.logErr("Requested node does not exist");
      throw FsckException("Requested node does not exist");
   }
}

void RetrieveDirEntriesWork::flushDirEntries()
{
   FsckDirEntry failedInsert;
   int errorCode;
   bool success = this->bufferDirEntries.flush(failedInsert, errorCode);

   if ( !success )
   {
      log.log(1, "Failed to insert dirEntry with name " + failedInsert.getName() + " and parentID "
         + failedInsert.getParentDirID());
      log.log(1, "SQLite Error was error code " + StringTk::intToStr(errorCode));
      throw FsckDBException(
         "Error while inserting dir entries to database. Please see log for more information.");
   }
}

void RetrieveDirEntriesWork::flushFileInodes()
{
   FsckFileInode failedInsert;
   int errorCode;
   bool success = this->bufferFileInodes.flush(failedInsert, errorCode);

   if ( !success )
   {
      log.log(1, "Failed to insert inlined file inode with ID " + failedInsert.getID());
      log.log(1, "SQLite Error was error code " + StringTk::intToStr(errorCode));
      throw FsckDBException(
         "Error while inserting file inodes to database. Please see log for more information.");
   }
}

void RetrieveDirEntriesWork::flushUsedTargets()
{
   FsckTargetID failedInsert;
   int errorCode;

   bool success = this->bufferUsedTargets.flush(failedInsert, errorCode);

   if ( !success )
   {
      log.log(1, "Failed to insert used target; ID: " + StringTk::uintToStr(failedInsert.getID()));
      log.log(1, "SQLite Error was error code " + StringTk::intToStr(errorCode));
      throw FsckDBException(
         "Error while inserting target IDs to database. Please see log for more information.");
   }
}

void RetrieveDirEntriesWork::flushContDirs()
{
   FsckContDir failedInsert;
   int errorCode;
   bool success = this->bufferContDirs.flush(failedInsert, errorCode);

   if ( !success )
   {
      log.log(1, "Failed to insert cont. dir with ID " + failedInsert.getID() + " from node "
         + StringTk::uintToStr(failedInsert.getSaveNodeID()));
      log.log(1, "SQLite Error was error code " + StringTk::intToStr(errorCode));
      throw FsckDBException(
         "Error while inserting chunks to database. Please see log for more information.");
   }
}

#include "RetrieveInodesWork.h"
#include <common/net/message/fsck/RetrieveInodesMsg.h>
#include <common/net/message/fsck/RetrieveInodesRespMsg.h>
#include <common/toolkit/MetaStorageTk.h>
#include <common/toolkit/MessagingTk.h>
#include <database/FsckDBException.h>
#include <toolkit/FsckException.h>

#include <program/Program.h>

RetrieveInodesWork::RetrieveInodesWork(Node* node, SynchronizedCounter* counter,
   unsigned hashDirStart, unsigned hashDirEnd, AtomicUInt64* numFileInodesFound,
   AtomicUInt64* numDirInodesFound) : Work()
{
   log.setContext("RetrieveInodesWork");
   this->node = node;
   this->counter = counter;
   this->hashDirStart = hashDirStart;
   this->hashDirEnd = hashDirEnd;
   this->numFileInodesFound = numFileInodesFound;
   this->numDirInodesFound = numDirInodesFound;
}

RetrieveInodesWork::~RetrieveInodesWork()
{
}

void RetrieveInodesWork::process(char* bufIn, unsigned bufInLen, char* bufOut,
   unsigned bufOutLen)
{
   log.log(4, "Processing RetrieveInodesWork");

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

   log.log(4, "Processed RetrieveInodesWork");
}

void RetrieveInodesWork::doWork()
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

            int64_t lastOffset = 0;
            size_t fileInodeCount;
            size_t dirInodeCount;

            do
            {
               bool commRes;
               char *respBuf = NULL;
               NetMessage *respMsg = NULL;

               RetrieveInodesMsg retrieveInodesMsg(hashDirNum, lastOffset,
                  RETRIEVE_INODES_PACKET_SIZE);

               commRes = MessagingTk::requestResponse(node, &retrieveInodesMsg,
                  NETMSGTYPE_RetrieveInodesResp, &respBuf, &respMsg);
               if ( commRes )
               {
                  RetrieveInodesRespMsg* retrieveInodesRespMsg = (RetrieveInodesRespMsg*) respMsg;

                  // set new parameters
                  lastOffset = retrieveInodesRespMsg->getLastOffset();

                  // parse all file inodes
                  FsckFileInodeList fileInodes;
                  retrieveInodesRespMsg->parseFileInodes(&fileInodes);

                  // add targetIDs
                  // TODO: think about gathering targetIDs on the server side and sending them over as
                  // seperate list/set to avoid iterating here
                  for ( FsckFileInodeListIter iter = fileInodes.begin(); iter != fileInodes.end();
                     iter++ )
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

                  // parse all directory inodes
                  FsckDirInodeList dirInodes;
                  retrieveInodesRespMsg->parseDirInodes(&dirInodes);

                  fileInodeCount = fileInodes.size();
                  dirInodeCount = dirInodes.size();

                  // insert all file inodes to the buffer and flush, if MAX_BUFFER_SIZE reached
                  if ( this->bufferFileInodes.add(fileInodes) > MAX_BUFFER_SIZE )
                     this->flushFileInodes();

                  // insert all dir inodes to the buffer and flush, if MAX_BUFFER_SIZE reached
                  if ( this->bufferDirInodes.add(dirInodes) > MAX_BUFFER_SIZE )
                     this->flushDirInodes();

                  numFileInodesFound->increase(fileInodeCount);
                  numDirInodesFound->increase(dirInodeCount);

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

            } while ( (fileInodeCount + dirInodeCount) > 0 );
         }
      }

      // flush remaining entries in buffers
      this->flushUsedTargets();
      this->flushFileInodes();
      this->flushDirInodes();
   }
   else
   {
      // basically this should never ever happen
      log.logErr("Requested node does not exist");
      throw FsckException("Requested node does not exist");
   }
}

void RetrieveInodesWork::flushFileInodes()
{
   FsckFileInode failedInsert;
   int errorCode;

   bool success = this->bufferFileInodes.flush(failedInsert, errorCode);

   if ( !success )
   {
      log.log(1, "Failed to insert file inode; ID: " + failedInsert.getID());
      log.log(1, "SQLite Error was error code " + StringTk::intToStr(errorCode));
      throw FsckDBException (
         "Error while inserting file inodes to database. Please see log for more information.");
   }
}

void RetrieveInodesWork::flushDirInodes()
{
   FsckDirInode failedInsert;
   int errorCode;

   bool success = this->bufferDirInodes.flush(failedInsert, errorCode);

   if ( !success )
   {
      log.log(1, "Failed to insert directory inode; ID: " + failedInsert.getID());
      log.log(1, "SQLite Error was error code " + StringTk::intToStr(errorCode));
      throw FsckDBException(
         "Error while inserting dir inodes to database. Please see log for more information.");
   }
}

void RetrieveInodesWork::flushUsedTargets()
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

#include "AdjustChunkPermissionsWork.h"
#include <common/net/message/fsck/AdjustChunkPermissionsMsg.h>
#include <common/net/message/fsck/AdjustChunkPermissionsRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <toolkit/FsckException.h>
#include <program/Program.h>

AdjustChunkPermissionsWork::AdjustChunkPermissionsWork(Node* node, SynchronizedCounter* counter,
   AtomicUInt64* fileCount, AtomicUInt64* errorCount) :
   Work()
{
   log.setContext("AdjustChunkPermissionsWork");
   this->node = node;
   this->counter = counter;
   this->fileCount = fileCount;
   this->errorCount = errorCount;
}

AdjustChunkPermissionsWork::~AdjustChunkPermissionsWork()
{
}

void AdjustChunkPermissionsWork::process(char* bufIn, unsigned bufInLen, char* bufOut,
   unsigned bufOutLen)
{
   log.log(4, "Processing AdjustChunkPermissionsWork");

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

   log.log(4, "Processed AdjustChunkPermissionsWork");
}

void AdjustChunkPermissionsWork::doWork()
{
   if ( node )
   {
      for ( unsigned firstLevelhashDirNum = 0;
         firstLevelhashDirNum <= META_DENTRIES_LEVEL1_SUBDIR_NUM - 1; firstLevelhashDirNum++ )
      {
         for ( unsigned secondLevelhashDirNum = 0;
            secondLevelhashDirNum < META_DENTRIES_LEVEL2_SUBDIR_NUM; secondLevelhashDirNum++ )
         {
            unsigned hashDirNum = StorageTk::mergeHashDirs(firstLevelhashDirNum,
               secondLevelhashDirNum);

            int64_t hashDirOffset = 0;
            int64_t contDirOffset = 0;
            std::string currentContDirID = "";
            unsigned resultCount = 0;

            do
            {
               bool commRes;
               char *respBuf = NULL;
               NetMessage *respMsg = NULL;

               AdjustChunkPermissionsMsg adjustChunkPermissionsMsg(hashDirNum, currentContDirID,
                  ADJUST_AT_ONCE, hashDirOffset, contDirOffset);

               commRes = MessagingTk::requestResponse(node, &adjustChunkPermissionsMsg,
                  NETMSGTYPE_AdjustChunkPermissionsResp, &respBuf, &respMsg);

               if ( commRes )
               {
                  AdjustChunkPermissionsRespMsg* adjustChunkPermissionsRespMsg =
                     (AdjustChunkPermissionsRespMsg*) respMsg;

                  // set new parameters
                  currentContDirID = adjustChunkPermissionsRespMsg->getCurrentContDirID();
                  hashDirOffset = adjustChunkPermissionsRespMsg->getNewHashDirOffset();
                  contDirOffset = adjustChunkPermissionsRespMsg->getNewContDirOffset();
                  resultCount = adjustChunkPermissionsRespMsg->getCount();

                  this->fileCount->increase(resultCount);

                  if (adjustChunkPermissionsRespMsg->getErrorCount() > 0)
                     this->errorCount->increase(adjustChunkPermissionsRespMsg->getErrorCount());

                  SAFE_DELETE(respMsg);
                  SAFE_FREE(respBuf);
               }
               else
               {
                  SAFE_DELETE(respMsg);
                  SAFE_FREE(respBuf);
                  throw FsckException("Communication error occured with node " + node->getID());
               }

               // if any of the worker threads threw an exception, we should stop now!
               if ( Program::getApp()->getSelfTerminate() )
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

/*
 void AdjustChunkPermissionsWork::doWork()
 {
 if ( node )
 {
 for ( unsigned firstLevelhashDirNum = 0;
 firstLevelhashDirNum <= META_DENTRIES_LEVEL1_SUBDIR_NUM - 1; firstLevelhashDirNum++ )
 {
 for ( unsigned secondLevelhashDirNum = 0;
 secondLevelhashDirNum < META_DENTRIES_LEVEL2_SUBDIR_NUM; secondLevelhashDirNum++ )
 {
 unsigned hashDirNum = StorageTk::mergeHashDirs(firstLevelhashDirNum,
 secondLevelhashDirNum);

 int64_t lastOffset = 0;
 int resultCount = 0;

 do
 {
 bool commRes;
 char *respBuf = NULL;
 NetMessage *respMsg = NULL;

 AdjustChunkPermissionsMsg adjustChunkPermissionsMsg(hashDirNum, lastOffset,
 ADJUST_AT_ONCE);

 commRes = MessagingTk::requestResponse(node, &adjustChunkPermissionsMsg,
 NETMSGTYPE_AdjustChunkPermissionsResp, &respBuf, &respMsg);

 if ( commRes )
 {
 AdjustChunkPermissionsRespMsg* adjustChunkPermissionsRespMsg =
 (AdjustChunkPermissionsRespMsg*) respMsg;

 resultCount = adjustChunkPermissionsRespMsg->getCount();
 lastOffset = adjustChunkPermissionsRespMsg->getLastOffset();

 SAFE_FREE(respBuf);
 SAFE_DELETE(respMsg);

 this->fileCount->increase(resultCount);
 }
 else
 {
 SAFE_FREE(respBuf);
 SAFE_DELETE(respMsg);
 throw HardFsckException("Communication error occured with node " + node->getID());
 }

 // if any of the worker threads threw an exception, we should stop now!
 if ( Program::getApp()->getSelfTerminate() )
 return;

 } while ( resultCount > 0 );
 }
 }
 }
 else
 {
 // basically this should never ever happen
 log.logErr("Requested node does not exist");
 throw HardFsckException("Requested node does not exist");
 }
 } */

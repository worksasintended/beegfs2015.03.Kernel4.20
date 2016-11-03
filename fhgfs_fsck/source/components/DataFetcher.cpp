#include "DataFetcher.h"

#include <common/Common.h>
#include <common/storage/Storagedata.h>
#include <components/worker/RetrieveChunksWork.h>
#include <components/worker/RetrieveDirEntriesWork.h>
#include <components/worker/RetrieveFsIDsWork.h>
#include <components/worker/RetrieveInodesWork.h>
#include <program/Program.h>
#include <toolkit/FsckTkEx.h>
#include <toolkit/FsckException.h>

DataFetcher::DataFetcher(FsckDB& db, bool forceRestart)
   : database(&db),
     workQueue(Program::getApp()->getWorkQueue() ),
     generatedPackages(0),
     forceRestart(forceRestart)
{
}

FhgfsOpsErr DataFetcher::execute()
{
   FhgfsOpsErr retVal = FhgfsOpsErr_SUCCESS;

   NodeStore* metaNodeStore = Program::getApp()->getMetaNodes();
   NodeList metaNodeList;
   metaNodeStore->referenceAllNodes(&metaNodeList);

   printStatus();

   retrieveDirEntries(&metaNodeList);
   retrieveInodes(&metaNodeList);
   const bool retrieveRes = retrieveChunks();

   if (!retrieveRes)
   {
      retVal = FhgfsOpsErr_INUSE;
      Program::getApp()->abort();
   }

   // wait for all packages to finish, because we cannot proceed if not all data was fetched
   // BUT : update output each OUTPUT_INTERVAL_MS ms
   while ( !this->finishedPackages.timedWaitForCount(generatedPackages,
   DATAFETCHER_OUTPUT_INTERVAL_MS) )
   {
      printStatus();

      if ( retVal != FhgfsOpsErr_INUSE && Program::getApp()->getShallAbort() )
      {
         // setting retVal to INTERRUPTED
         // but still, we needed to wait for the workers to terminate, because of the
         // SynchronizedCounter (this object cannnot be destroyed before all workers terminate)
         retVal = FhgfsOpsErr_INTERRUPTED;
      }
   }

   if (retVal == FhgfsOpsErr_SUCCESS && fatalErrorsFound.read() > 0)
      retVal = FhgfsOpsErr_INTERNAL;

   if(retVal == FhgfsOpsErr_SUCCESS)
   {
      std::set<FsckTargetID> allUsedTargets;

      while(!this->usedTargets.empty() )
      {
         allUsedTargets.insert(this->usedTargets.front().begin(), this->usedTargets.front().end() );
         this->usedTargets.pop_front();
      }

      std::list<FsckTargetID> usedTargetsList(allUsedTargets.begin(), allUsedTargets.end() );

      this->database->getUsedTargetIDsTable()->insert(usedTargetsList,
         this->database->getUsedTargetIDsTable()->newBulkHandle() );
   }

   printStatus(true);

   FsckTkEx::fsckOutput(""); // just a new line

   metaNodeStore->releaseAllNodes(&metaNodeList);
   return retVal;
}

void DataFetcher::retrieveDirEntries(NodeList* nodeList)
{
   for (NodeListIter nodeIter = nodeList->begin(); nodeIter != nodeList->end(); nodeIter++)
   {
      Node *node = *nodeIter;

      int requestsPerNode = 2;

      unsigned hashDirsPerRequest = (unsigned)(META_DENTRIES_LEVEL1_SUBDIR_NUM/requestsPerNode);

      unsigned hashDirStart = 0;
      unsigned hashDirEnd = 0;

      do
      {
         hashDirEnd = hashDirStart + hashDirsPerRequest;

         // fetch DirEntries

         // before we create a package we increment the generated packages counter
         this->generatedPackages++;
         this->usedTargets.insert(this->usedTargets.end(), std::set<FsckTargetID>() );

         this->workQueue->addIndirectWork(
            new RetrieveDirEntriesWork(this->database, node, &(this->finishedPackages),
               fatalErrorsFound, hashDirStart,
               BEEGFS_MIN(hashDirEnd, META_DENTRIES_LEVEL1_SUBDIR_NUM - 1),
               &numDentriesFound, &numFileInodesFound, this->usedTargets.back()));

         // fetch fsIDs

         // before we create a package we increment the generated packages counter
         this->generatedPackages++;

         this->workQueue->addIndirectWork(
            new RetrieveFsIDsWork(this->database, node, &(this->finishedPackages), fatalErrorsFound,
               hashDirStart, BEEGFS_MIN(hashDirEnd, META_DENTRIES_LEVEL1_SUBDIR_NUM - 1)));

         hashDirStart = hashDirEnd + 1;
      }  while (hashDirEnd < META_DENTRIES_LEVEL1_SUBDIR_NUM);
   }
}

void DataFetcher::retrieveInodes(NodeList* nodeList)
{
   for (NodeListIter nodeIter = nodeList->begin(); nodeIter != nodeList->end(); nodeIter++)
   {
      Node *node = *nodeIter;

      int requestsPerNode = 2;

      unsigned hashDirsPerRequest = (unsigned)(META_INODES_LEVEL1_SUBDIR_NUM/ requestsPerNode);

      unsigned hashDirStart = 0;
      unsigned hashDirEnd = 0;

      do
      {
         // before we create a package we increment the generated packages counter
         this->generatedPackages++;
         this->usedTargets.insert(this->usedTargets.end(), std::set<FsckTargetID>() );

         hashDirEnd = hashDirStart + hashDirsPerRequest;

         this->workQueue->addIndirectWork(
            new RetrieveInodesWork(this->database, node, &(this->finishedPackages),
               fatalErrorsFound, hashDirStart,
               BEEGFS_MIN(hashDirEnd, META_INODES_LEVEL1_SUBDIR_NUM - 1),
               &numFileInodesFound, &numDirInodesFound, this->usedTargets.back()));

         hashDirStart = hashDirEnd + 1;
      } while (hashDirEnd < META_INODES_LEVEL1_SUBDIR_NUM);
   }
}

bool DataFetcher::retrieveChunks()
{
   App* app = Program::getApp();

   NodeStore* storageNodes = app->getStorageNodes();

   // for each server create a work package to retrieve chunks
   Node* node = storageNodes->referenceFirstNode();

   while (node)
   {
      uint16_t nodeNumID = node->getNumID();

      // before we create a package we increment the generated packages counter
      this->generatedPackages++;

      // node will be released inside of work package
      RetrieveChunksWork* retrieveWork = new RetrieveChunksWork(this->database, node,
            &(this->finishedPackages), fatalErrorsFound, &numChunksFound, forceRestart);
      this->workQueue->addIndirectWork(retrieveWork);

      bool started;
      retrieveWork->waitForStarted(&started);
      if (!started)
         return false;

      node = storageNodes->referenceNextNode(nodeNumID);
   }

   return true;
}

void DataFetcher::printStatus(bool toLogFile)
{
   uint64_t dentryCount = numDentriesFound.read();
   uint64_t fileInodeCount = numFileInodesFound.read();
   uint64_t dirInodeCount = numDirInodesFound.read();
   uint64_t chunkCount = numChunksFound.read();

   std::string outputStr = "Fetched data > Directory entries: " + StringTk::uint64ToStr(dentryCount)
      + " | Inodes: " + StringTk::uint64ToStr(fileInodeCount+dirInodeCount) + " | Chunks: " +
      StringTk::uint64ToStr(chunkCount);

   int outputFlags = OutputOptions_LINEDELETE;

   if (!toLogFile)
      outputFlags = outputFlags | OutputOptions_NOLOG;

   FsckTkEx::fsckOutput(outputStr, outputFlags);
}

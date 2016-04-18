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

DataFetcher::DataFetcher()
{
   generatedPackages = 0;
   this->database = Program::getApp()->getDatabase();
   this->workQueue = Program::getApp()->getWorkQueue();
}

DataFetcher::~DataFetcher()
{
}

bool DataFetcher::execute(unsigned short fetchMode)
{
   bool retVal = true;

   NodeStore* metaNodeStore = Program::getApp()->getMetaNodes();
   NodeList metaNodeList;
   metaNodeStore->referenceAllNodes(&metaNodeList);

   printStatus();

   if (fetchMode & DATAFETCHER_FETCHMODE_DIRENTRIES)
      retrieveDirEntries(&metaNodeList);

   if (fetchMode & DATAFETCHER_FETCHMODE_INODES)
      retrieveInodes(&metaNodeList);

   if (fetchMode & DATAFETCHER_FETCHMODE_CHUNKS)
      retrieveChunks();

   // wait for all packages to finish, because we cannot proceed if not all data was fetched
   // BUT : update output each OUTPUT_INTERVAL_MS ms
   while ( !this->finishedPackages.timedWaitForCount(generatedPackages,
   DATAFETCHER_OUTPUT_INTERVAL_MS) )
   {
      printStatus();

      if ( Program::getApp()->getShallAbort() )
      {
         // setting retVal to false
         // but still, we needed to wait for the workers to terminate, because of the
         // SynchronizedCounter (this object cannnot be destroyed before all workers terminate)
         retVal = false;
      }
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

         this->workQueue->addIndirectWork(
            new RetrieveDirEntriesWork(node, &(this->finishedPackages), hashDirStart,
               BEEGFS_MIN(hashDirEnd, META_DENTRIES_LEVEL1_SUBDIR_NUM - 1), &numDentriesFound,
               &numFileInodesFound));

         // fetch fsIDs

         // before we create a package we increment the generated packages counter
         this->generatedPackages++;

         this->workQueue->addIndirectWork(
            new RetrieveFsIDsWork(node, &(this->finishedPackages), hashDirStart,
               BEEGFS_MIN(hashDirEnd, META_DENTRIES_LEVEL1_SUBDIR_NUM - 1)) );

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

         hashDirEnd = hashDirStart + hashDirsPerRequest;

         this->workQueue->addIndirectWork(
            new RetrieveInodesWork(node, &(this->finishedPackages), hashDirStart,
               BEEGFS_MIN(hashDirEnd, META_INODES_LEVEL1_SUBDIR_NUM - 1), &numFileInodesFound,
               &numDirInodesFound));

         hashDirStart = hashDirEnd + 1;
      } while (hashDirEnd < META_INODES_LEVEL1_SUBDIR_NUM);
   }
}

void DataFetcher::retrieveChunks()
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
      this->workQueue->addIndirectWork(new RetrieveChunksWork(node, &(this->finishedPackages),
         &numChunksFound));

      node = storageNodes->referenceNextNode(nodeNumID);
   }
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

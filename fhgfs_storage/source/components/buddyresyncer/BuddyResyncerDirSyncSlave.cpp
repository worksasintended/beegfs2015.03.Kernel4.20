#include <app/App.h>
#include <common/net/message/storage/creating/RmChunkPathsMsg.h>
#include <common/net/message/storage/creating/RmChunkPathsRespMsg.h>
#include <common/net/message/storage/listing/ListChunkDirIncrementalMsg.h>
#include <common/net/message/storage/listing/ListChunkDirIncrementalRespMsg.h>
#include <toolkit/StorageTkEx.h>
#include <program/Program.h>

#include "BuddyResyncerDirSyncSlave.h"

#define CHECK_AT_ONCE 50

BuddyResyncerDirSyncSlave::BuddyResyncerDirSyncSlave(uint16_t targetID,
   ChunkSyncCandidateStore* syncCandidates, uint8_t slaveID) :
   PThread("BuddyResyncerDirSyncSlave_" + StringTk::uintToStr(targetID) + "-"
      + StringTk::uintToStr(slaveID))
{
   this->isRunning = false;
   this->targetID = targetID;
   this->syncCandidates = syncCandidates;
}

BuddyResyncerDirSyncSlave::~BuddyResyncerDirSyncSlave()
{
}

/**
 * This is a component, which is started through its control frontend on-demand at
 * runtime and terminates when it's done.
 * We have to ensure (in cooperation with the control frontend) that we don't get multiple instances
 * of this thread running at the same time.
 */
void BuddyResyncerDirSyncSlave::run()
{
   setIsRunning(true);

   try
   {
      LogContext(__func__).log(Log_DEBUG, "Component started.");

      registerSignalHandler();

      numAdditionalDirsMatched.setZero();
      numDirsSynced.setZero();
      errorCount.setZero();

      syncLoop();

      LogContext(__func__).log(Log_DEBUG, "Component stopped.");
   }
   catch (std::exception& e)
   {
      PThread::getCurrentThreadApp()->handleComponentException(e);
   }

   setIsRunning(false);
}

void BuddyResyncerDirSyncSlave::syncLoop()
{
   App* app = Program::getApp();
   MirrorBuddyGroupMapper* buddyGroupMapper = app->getMirrorBuddyGroupMapper();

   while (! getSelfTerminateNotIdle())
   {
      if((syncCandidates->isDirsEmpty()) && (getSelfTerminate()))
         break;

      ChunkSyncCandidateDir candidate;

      syncCandidates->fetch(candidate, this);

      if (unlikely(candidate.getTargetID() == 0)) // ignore targetID 0
         continue;

      std::string relativePath = candidate.getRelativePath();
      uint16_t localTargetID = candidate.getTargetID();

      // get buddy targetID
      uint16_t buddyTargetID = buddyGroupMapper->getBuddyTargetID(localTargetID);
      // perform sync
      FhgfsOpsErr resyncRes = doSync(relativePath, localTargetID, buddyTargetID);
      if (resyncRes == FhgfsOpsErr_SUCCESS)
         numDirsSynced.increase();
      else
      if (resyncRes != FhgfsOpsErr_INTERRUPTED)
         errorCount.increase();
   }
}

FhgfsOpsErr BuddyResyncerDirSyncSlave::doSync(std::string& dirPath, uint16_t localTargetID,
   uint16_t buddyTargetID)
{
   FhgfsOpsErr retVal = FhgfsOpsErr_SUCCESS;

   App* app = Program::getApp();
   TargetMapper* targetMapper = app->getTargetMapper();
   NodeStoreServers* storageNodes = app->getStorageNodes();

   // try to find the node with the buddyTargetID
   uint16_t buddyNodeID = targetMapper->getNodeID(buddyTargetID);
   Node* node = storageNodes->referenceNode(buddyNodeID);

   if(!node)
   {
      LogContext(__func__).logErr(
         "Storage node does not exist; nodeID " + StringTk::uintToStr(buddyNodeID));

      return FhgfsOpsErr_UNKNOWNNODE;
   }

   int64_t offset = 0;
   unsigned entriesFetched;

   do
   {
      int64_t newOffset;
      StringList names;
      IntList entryTypes;

      FhgfsOpsErr listRes = getBuddyDirContents(node, dirPath, buddyTargetID, offset, names,
         entryTypes, newOffset);

      if(listRes != FhgfsOpsErr_SUCCESS)
      {
         retVal = listRes;
         break;
      }

      offset = newOffset;
      entriesFetched = names.size();

      // match locally
      FhgfsOpsErr findRes = findChunks(localTargetID, dirPath, names, entryTypes);

      if(findRes != FhgfsOpsErr_SUCCESS)
      {
         retVal = findRes;
         break;
      }

      // delete the remaining chunks/dirs on the buddy
      StringList rmPaths;
      for (StringListIter iter = names.begin(); iter != names.end(); iter++)
      {
         std::string path = dirPath + "/" + *iter;
         rmPaths.push_back(path);
      }

      FhgfsOpsErr rmRes = removeBuddyChunkPaths(node, localTargetID, buddyTargetID, rmPaths);

      if (rmRes != FhgfsOpsErr_SUCCESS)
      {
         retVal = rmRes;
         break;
      }

      if (getSelfTerminateNotIdle())
      {
         retVal = FhgfsOpsErr_INTERRUPTED;
         break;
      }

   } while (entriesFetched == CHECK_AT_ONCE);

   storageNodes->releaseNode(&node);

   return retVal;
}

FhgfsOpsErr BuddyResyncerDirSyncSlave::getBuddyDirContents(Node* node, std::string& dirPath,
   uint16_t targetID, int64_t offset, StringList& outNames, IntList& outEntryTypes,
   int64_t& outNewOffset)
{
   FhgfsOpsErr retVal = FhgfsOpsErr_SUCCESS;
   unsigned msgRetryIntervalMS = 5000;

   // get a part of the dir contents from the buddy target
   ListChunkDirIncrementalMsg listMsg(targetID, true, dirPath, offset, CHECK_AT_ONCE, false);
   listMsg.setMsgHeaderFeatureFlags(LISTCHUNKDIRINCREMENTALMSG_FLAG_IGNORENOTEXISTS);
   listMsg.setMsgHeaderTargetID(targetID);

   bool commRes = false;
   CombinedTargetState state;
   bool getStateRes = Program::getApp()->getTargetStateStore()->getState(targetID, state);

   // send request to node and receive response
   char* respBuf;
   NetMessage* respMsg;

   while ( (!commRes) && (getStateRes)
      && (state.reachabilityState != TargetReachabilityState_OFFLINE) )
   {
      commRes = MessagingTk::requestResponse(node, &listMsg, NETMSGTYPE_ListChunkDirIncrementalResp,
         &respBuf, &respMsg);

      if(!commRes)
      {
         LOG_DEBUG(__func__, Log_NOTICE,
            "Unable to communicate, but target is not offline; sleeping "
            + StringTk::uintToStr(msgRetryIntervalMS) + "ms before retry. targetID: "
            + StringTk::uintToStr(targetID));

         PThread::sleepMS(msgRetryIntervalMS);

         // if thread shall terminate, break loop here
         if ( getSelfTerminateNotIdle() )
            break;

         getStateRes = Program::getApp()->getTargetStateStore()->getState(targetID, state);
      }
   }

   if(!commRes)
   { // communication error
      LogContext(__func__).logErr(
         "Communication with storage node failed: " + node->getTypedNodeID());

      retVal = FhgfsOpsErr_COMMUNICATION;
   }
   else
   if(!getStateRes)
   {
      LogContext(__func__).logErr("No valid state for node ID: " + node->getTypedNodeID() );

      retVal = FhgfsOpsErr_INTERNAL;
   }
   else
   {
      // correct response type received
      ListChunkDirIncrementalRespMsg* respMsgCast = (ListChunkDirIncrementalRespMsg*) respMsg;

      FhgfsOpsErr listRes = respMsgCast->getResult();

      if (listRes == FhgfsOpsErr_SUCCESS)
      {
         outNewOffset = respMsgCast->getNewOffset();
         respMsgCast->parseNames(&outNames);
         respMsgCast->parseEntryTypes(&outEntryTypes);
      }
      else
      if (listRes != FhgfsOpsErr_PATHNOTEXISTS)
      {  // not exists is ok, because path might have been deleted
         LogContext(__func__).log(Log_WARNING, "Error listing chunks dir; "
            "dirPath: " + dirPath + "; "
            "targetID: " + StringTk::uintToStr(targetID) + "; "
            "node: " + node->getTypedNodeID() + "; "
            "Error: " + FhgfsOpsErrTk::toErrString(listRes));

         retVal = listRes;
      }

      delete (respMsg);
      free(respBuf);
   }

   return retVal;
}

FhgfsOpsErr BuddyResyncerDirSyncSlave::findChunks(uint16_t targetID, std::string& dirPath,
   StringList& inOutNames, IntList& inOutEntryTypes)
{
   App* app = Program::getApp();
   ChunkLockStore* chunkLockStore = app->getChunkLockStore();

   int targetFD = app->getTargetFD(targetID, true);

   StringListIter namesIter = inOutNames.begin();
   IntListIter typesIter = inOutEntryTypes.begin();
   while (namesIter != inOutNames.end())
   {
      std::string entryID = *namesIter;
      DirEntryType entryType = (DirEntryType)*typesIter;

      std::string entryPath;
      if (likely(!dirPath.empty()))
         entryPath = dirPath + "/" + entryID;
      else
         entryPath = entryID;

      if (DirEntryType_ISDIR(entryType))
      {
         bool entryExists = StorageTk::pathExists(targetFD, entryPath);

         if (!entryExists)
         {
            // dir not found, so we didn't know about it yet => add it to sync candidate store, so
            // that it gets checked and we get a list of its contents;
            ChunkSyncCandidateDir syncCandidate(entryPath, targetID);
            syncCandidates->add(syncCandidate, this);
            numAdditionalDirsMatched.increase();
         }

         // no matter if found or not:  remove it from the list, because we do not explicitely
         // delete directories on the buddy
         namesIter = inOutNames.erase(namesIter);
         typesIter = inOutEntryTypes.erase(typesIter);
      }
      else
      {
         // need to lock the chunk to check it
         chunkLockStore->lockChunk(targetID, entryID);

         bool entryExists = StorageTk::pathExists(targetFD, entryPath);

         if (entryExists)
         {
            // chunk found => delete it from list an unlock it
            namesIter = inOutNames.erase(namesIter);
            typesIter = inOutEntryTypes.erase(typesIter);
            chunkLockStore->unlockChunk(targetID, entryID);
         }
         else
         {
            // chunk not found => keep lock; will be unlocked after removal
            namesIter++;
            typesIter++;
         }

      }
   }

   return FhgfsOpsErr_SUCCESS;
}

FhgfsOpsErr BuddyResyncerDirSyncSlave::removeBuddyChunkPaths(Node* node, uint16_t localTargetID,
   uint16_t buddyTargetID, StringList& paths)
{
   unsigned msgRetryIntervalMS = 5000;

   ChunkLockStore* chunkLockStore = Program::getApp()->getChunkLockStore();
   RmChunkPathsMsg rmMsg(buddyTargetID, &paths);
   rmMsg.addMsgHeaderFeatureFlag(RMCHUNKPATHSMSG_FLAG_BUDDYMIRROR);
   rmMsg.setMsgHeaderTargetID(buddyTargetID);

   bool commRes = false;
   CombinedTargetState state;
   bool getStateRes = Program::getApp()->getTargetStateStore()->getState(buddyTargetID, state);

   // send request to node and receive response
   char* respBuf;
   NetMessage* respMsg;

   while ((!commRes) && (getStateRes)
      && (state.reachabilityState != TargetReachabilityState_OFFLINE))
   {
      commRes = MessagingTk::requestResponse(node, &rmMsg, NETMSGTYPE_RmChunkPathsResp,
         &respBuf, &respMsg);

      if(!commRes)
      {
         LOG_DEBUG(__func__, Log_NOTICE,
            "Unable to communicate, but target is not offline; sleeping "
            + StringTk::uintToStr(msgRetryIntervalMS) + "ms before retry. targetID: "
            + StringTk::uintToStr(targetID));
         PThread::sleepMS(msgRetryIntervalMS);

         // if thread shall terminate, break loop here
         if ( getSelfTerminateNotIdle() )
            break;

         getStateRes = Program::getApp()->getTargetStateStore()->getState(buddyTargetID, state);
      }
   }

   // no matter if that succeeded or not we unlock all chunks here first
   for (StringListIter iter = paths.begin(); iter != paths.end(); iter++)
   {
      std::string entryID = StorageTk::getPathBasename(*iter);
      chunkLockStore->unlockChunk(localTargetID, entryID);
   }

   if(!commRes)
   { // communication error
      LogContext(__func__).logErr(
         "Communication with storage node failed: " + node->getTypedNodeID());

      return FhgfsOpsErr_COMMUNICATION;
   }
   else
   if(!getStateRes)
   {
      LogContext(__func__).logErr("No valid state for node ID: " + node->getTypedNodeID() );

      return FhgfsOpsErr_INTERNAL;
   }
   else
   {
      // correct response type received
      RmChunkPathsRespMsg* respMsgCast = (RmChunkPathsRespMsg*) respMsg;
      StringList failedPaths;

      respMsgCast->parseFailedPaths(&failedPaths);
      for(StringListIter iter = failedPaths.begin(); iter != failedPaths.end(); iter++)
      {
         LogContext(__func__).logErr("Chunk path could not be deleted; "
            "path: " + *iter + "; "
            "buddyTargetID: " + StringTk::uintToStr(buddyTargetID) + "; "
            "node: " + node->getTypedNodeID());
      }

      delete (respMsg);
      free(respBuf);
   }

   return FhgfsOpsErr_SUCCESS;
}

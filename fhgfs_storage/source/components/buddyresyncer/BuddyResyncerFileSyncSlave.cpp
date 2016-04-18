#include <app/App.h>
#include <common/net/message/storage/creating/RmChunkPathsMsg.h>
#include <common/net/message/storage/creating/RmChunkPathsRespMsg.h>
#include <common/net/message/storage/mirroring/ResyncLocalFileMsg.h>
#include <common/net/message/storage/mirroring/ResyncLocalFileRespMsg.h>
#include <toolkit/StorageTkEx.h>
#include <program/Program.h>

#include "BuddyResyncerFileSyncSlave.h"

#define PROCESS_AT_ONCE 1
#define SYNC_BLOCK_SIZE (1024*1024) // 1M

BuddyResyncerFileSyncSlave::BuddyResyncerFileSyncSlave(uint16_t targetID,
   ChunkSyncCandidateStore* syncCandidates, uint8_t slaveID) :
   PThread("BuddyResyncerFileSyncSlave_" + StringTk::uintToStr(targetID) + "-"
      + StringTk::uintToStr(slaveID))
{
   this->isRunning = false;
   this->syncCandidates = syncCandidates;
   this->targetID = targetID;
}

BuddyResyncerFileSyncSlave::~BuddyResyncerFileSyncSlave()
{
}

/**
 * This is a component, which is started through its control frontend on-demand at
 * runtime and terminates when it's done.
 * We have to ensure (in cooperation with the control frontend) that we don't get multiple instances
 * of this thread running at the same time.
 */
void BuddyResyncerFileSyncSlave::run()
{
   setIsRunning(true);

   try
   {
      LogContext(__func__).log(Log_DEBUG, "Component started.");

      registerSignalHandler();

      numChunksSynced.setZero();
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

void BuddyResyncerFileSyncSlave::syncLoop()
{
   App* app = Program::getApp();
   MirrorBuddyGroupMapper* buddyGroupMapper = app->getMirrorBuddyGroupMapper();

   while (! getSelfTerminateNotIdle())
   {
      if((syncCandidates->isFilesEmpty()) && (getSelfTerminate()))
         break;

      ChunkSyncCandidateFile candidate;

      syncCandidates->fetch(candidate, this);

      if (unlikely(candidate.getTargetID() == 0)) // ignore targetID 0
         continue;

      std::string relativePath = candidate.getRelativePath();
      uint16_t localTargetID = candidate.getTargetID();
      bool onlyAttribs = candidate.getOnlyAttribs();

      // get buddy targetID
      uint16_t buddyTargetID = buddyGroupMapper->getBuddyTargetID(localTargetID);
      // perform sync
      FhgfsOpsErr resyncRes = doResync(relativePath, localTargetID, buddyTargetID, onlyAttribs);
      if (resyncRes == FhgfsOpsErr_SUCCESS)
         numChunksSynced.increase();
      else
      if (resyncRes != FhgfsOpsErr_INTERRUPTED)
         errorCount.increase();
   }
}

FhgfsOpsErr BuddyResyncerFileSyncSlave::doResync(std::string& chunkPathStr, uint16_t localTargetID,
   uint16_t buddyTargetID, bool onlyAttribs)
{
   FhgfsOpsErr retVal = FhgfsOpsErr_SUCCESS;
   unsigned msgRetryIntervalMS = 5000;

   App* app = Program::getApp();
   TargetMapper* targetMapper = app->getTargetMapper();
   NodeStoreServers* storageNodes = app->getStorageNodes();
   ChunkLockStore* chunkLockStore = app->getChunkLockStore();

   std::string entryID = StorageTk::getPathBasename(chunkPathStr);

   // try to find the node with the buddyTargetID
   uint16_t buddyNodeID = targetMapper->getNodeID(buddyTargetID);

   Node* node = storageNodes->referenceNode(buddyNodeID);

   if(!node)
   {
      LogContext(__func__).log(Log_WARNING,
         "Storage node does not exist; nodeID " + StringTk::uintToStr(buddyNodeID));

      return FhgfsOpsErr_UNKNOWNNODE;
   }

   int64_t offset = 0;
   ssize_t readRes = 0;
   ssize_t maxCount;
   unsigned resyncMsgFlags = 0;

   LogContext(__func__).log(Log_DEBUG,
      "File sync started. chunkPath: " + chunkPathStr + "; localTargetID: "
         + StringTk::uintToStr(localTargetID) + "; buddyTargetID"
         + StringTk::uintToStr(buddyTargetID));


   if (onlyAttribs)
   {
      maxCount = 0;
      resyncMsgFlags |= RESYNCLOCALFILEMSG_FLAG_NODATA;
   }
   else
      maxCount = SYNC_BLOCK_SIZE;

   do
   {
      char* data = (char*) (maxCount ? malloc(maxCount) : NULL);

      if (unlikely(maxCount && !data) ) // malloc failed
      {
         LogContext(__func__).logErr("Could not allocate memory for data buf");
         throw std::bad_alloc();
      }

      // lock the chunk
      chunkLockStore->lockChunk(localTargetID, entryID);

      int fd = openat(app->getTargetFD(localTargetID, true), chunkPathStr.c_str(),
         O_RDONLY | O_NOATIME);

      if (fd == -1)
      {
         int errCode = errno;

         if(errCode == ENOENT)
         { // chunk was deleted => no error
            // delete the mirror chunk before resync starts
            bool rmRes = removeBuddyChunkUnlocked(node, buddyTargetID, chunkPathStr);

            if (!rmRes) // rm failed; stop resync
            {
               LogContext(__func__).log(Log_WARNING,
                  "File sync not started. chunkPath: " + chunkPathStr + "; localTargetID: "
                     + StringTk::uintToStr(localTargetID) + "; buddyTargetID: "
                     + StringTk::uintToStr(buddyTargetID));

               retVal = FhgfsOpsErr_INTERNAL;
            }
         }
         else
         {
            LogContext(__func__).logErr(
               "Open of chunk failed. chunkPath: " + chunkPathStr + "; targetID: "
                  + StringTk::uintToStr(localTargetID) + "; Error: "
                  + System::getErrString(errCode));

            retVal = FhgfsOpsErr_INTERNAL;
         }

         chunkLockStore->unlockChunk(localTargetID, entryID);

         goto cleanup;
      }

      int seekRes = lseek(fd, offset, SEEK_SET);

      if (seekRes == -1)
      {
         LogContext(__func__).logErr(
            "Seeking in chunk failed. chunkPath: " + chunkPathStr + "; targetID: "
               + StringTk::uintToStr(localTargetID) + "; offset: " + StringTk::int64ToStr(offset));

         chunkLockStore->unlockChunk(localTargetID, entryID);

         goto cleanup;
      }

      readRes = read(fd, data, maxCount);

      if( readRes == -1)
      {
         LogContext(__func__).logErr("Error during read; "
            "chunkPath: " + chunkPathStr + "; "
            "targetID: " + StringTk::uintToStr(localTargetID) + "; "
            "BuddyNode: " + node->getTypedNodeID() + "; "
            "buddyTargetID: " + StringTk::uintToStr(buddyTargetID) + "; "
            "Error: " + System::getErrString(errno));

         retVal = FhgfsOpsErr_INTERNAL;

         goto end_of_loop;
      }

      if(readRes > 0)
      {
         const char zeroBuf[RESYNCER_SPARSE_BLOCK_SIZE] = { 0 };

         // check if sparse blocks are in the buffer
         ssize_t bufPos = 0;
         bool dataFound = false;
         while (bufPos < readRes)
         {
            size_t cmpLen = BEEGFS_MIN(readRes-bufPos, RESYNCER_SPARSE_BLOCK_SIZE);

            int cmpRes = memcmp(data + bufPos, zeroBuf, cmpLen);
            if(cmpRes != 0)
               dataFound = true;
            else // sparse area detected
            {
               if(dataFound) // had data before
               {
                  resyncMsgFlags |= RESYNCLOCALFILEMSG_CHECK_SPARSE; // let the receiver do a check
                  break; // and stop checking here
               }
            }

            bufPos += cmpLen;
         }

         // this inner loop is over and there are only sparse areas

         /* make sure we always send a msg at offset==0 to truncate the file and allow concurrent
            writers in a big inital sparse area */
         if(offset && (readRes > 0) && (readRes == maxCount) && !dataFound)
         {
            goto end_of_loop;
            // => no transfer needed
         }

         /* let the receiver do a check, because we might be sending a sparse block at beginnig or
            end of file */
         if(!dataFound)
            resyncMsgFlags |= RESYNCLOCALFILEMSG_CHECK_SPARSE;
      }

      {
         ResyncLocalFileMsg resyncMsg(data, chunkPathStr, buddyTargetID, offset, (int) readRes);

         if (!readRes || (readRes < maxCount) ) // last iteration, set attribs and trunc buddy chunk
         {
            struct stat statBuf;
            int statRes = fstat(fd, &statBuf);

            if (statRes == 0)
            {
               if(statBuf.st_size < offset)
               { // in case someone truncated the file while we're reading at a high offset
                  offset = statBuf.st_size;
                  resyncMsg.setOffset(offset);
               }
               else
               if(offset && !readRes)
                  resyncMsgFlags |= RESYNCLOCALFILEMSG_FLAG_TRUNC;

               int mode = statBuf.st_mode;
               unsigned userID = statBuf.st_uid;
               unsigned groupID = statBuf.st_gid;
               int64_t mtimeSecs = statBuf.st_mtim.tv_sec;
               int64_t atimeSecs = statBuf.st_atim.tv_sec;
               SettableFileAttribs chunkAttribs = {mode, userID,groupID, mtimeSecs, atimeSecs};
               resyncMsg.setChunkAttribs(chunkAttribs);
               resyncMsgFlags |= RESYNCLOCALFILEMSG_FLAG_SETATTRIBS;
            }
            else
            {
               LogContext(__func__).logErr("Error getting chunk attributes; "
                  "chunkPath: " + chunkPathStr + "; "
                  "targetID: " + StringTk::uintToStr(localTargetID) + "; "
                  "BuddyNode: " + node->getTypedNodeID() + "; "
                  "buddyTargetID: " + StringTk::uintToStr(buddyTargetID) + "; "
                  "Error: " + System::getErrString(errno));
            }
         }

         resyncMsg.setMsgHeaderFeatureFlags(resyncMsgFlags);
         resyncMsg.setMsgHeaderTargetID(buddyTargetID);

         bool commRes = false;
         CombinedTargetState state;
         bool getStateRes =
            Program::getApp()->getTargetStateStore()->getState(buddyTargetID, state);

         // send request to node and receive response
         char* respBuf;
         NetMessage* respMsg;

         while ( (!commRes) && (getStateRes)
            && (state.reachabilityState != TargetReachabilityState_OFFLINE) )
         {
            commRes = MessagingTk::requestResponse(node, &resyncMsg, NETMSGTYPE_ResyncLocalFileResp,
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

               getStateRes =
                  Program::getApp()->getTargetStateStore()->getState(buddyTargetID, state);
            }
         }

         if(!commRes)
         { // communication error
            LogContext(__func__).log(Log_WARNING,
               "Communication with storage node failed: " + node->getTypedNodeID());

            retVal = FhgfsOpsErr_COMMUNICATION;

            // set readRes to non-zero to force exiting loop
            readRes = -2;
         }
         else
         if(!getStateRes)
         {
            LogContext(__func__).log(Log_WARNING,
               "No valid state for node ID: " + node->getTypedNodeID());

            retVal = FhgfsOpsErr_INTERNAL;

            // set readRes to non-zero to force exiting loop
            readRes = -2;
         }
         else
         {
            // correct response type received
            ResyncLocalFileRespMsg* respMsgCast = (ResyncLocalFileRespMsg*) respMsg;

            FhgfsOpsErr syncRes = respMsgCast->getResult();

            if(syncRes != FhgfsOpsErr_SUCCESS)
            {
               LogContext(__func__).log(Log_WARNING, "Error during resync; "
                  "chunkPath: " + chunkPathStr + "; "
                  "targetID: " + StringTk::uintToStr(localTargetID) + "; "
                  "BuddyNode: " + node->getTypedNodeID() + "; "
                  "buddyTargetID: " + StringTk::uintToStr(buddyTargetID) + "; "
                  "Error: " + FhgfsOpsErrTk::toErrString(syncRes));

               retVal = syncRes;

               // set readRes to non-zero to force exiting loop
               readRes = -2;
            }

            delete (respMsg);
            free(respBuf);
         }
      }

   end_of_loop:
      int closeRes = close(fd);
      if (closeRes == -1)
      {
         LogContext(__func__).log(Log_WARNING, "Error closing file descriptor; "
            "chunkPath: " + chunkPathStr + "; "
            "targetID: " + StringTk::uintToStr(localTargetID) + "; "
            "BuddyNode: " + node->getTypedNodeID() + "; "
            "buddyTargetID: " + StringTk::uintToStr(buddyTargetID) + "; "
            "Error: " + System::getErrString(errno));
      }
      // unlock the chunk
      chunkLockStore->unlockChunk(localTargetID, entryID);

      // increment offset for next iteration
      offset += readRes;
      SAFE_FREE(data);

      if ( getSelfTerminateNotIdle() )
      {
         retVal = FhgfsOpsErr_INTERRUPTED;
         break;
      }

   } while (readRes == maxCount);

cleanup:
   storageNodes->releaseNode(&node);

   LogContext(__func__).log(Log_DEBUG, "File sync finished. chunkPath: " + chunkPathStr);

   return retVal;
}

/**
 * Note: Chunk has to be locked by caller.
 */
bool BuddyResyncerFileSyncSlave::removeBuddyChunkUnlocked(Node* node, uint16_t buddyTargetID,
   std::string& pathStr)
{
   bool retVal = true;
   unsigned msgRetryIntervalMS = 5000;

   std::string entryID = StorageTk::getPathBasename(pathStr);
   StringList rmPaths;
   rmPaths.push_back(pathStr);

   RmChunkPathsMsg rmMsg(buddyTargetID, &rmPaths);
   rmMsg.addMsgHeaderFeatureFlag(RMCHUNKPATHSMSG_FLAG_BUDDYMIRROR);
   rmMsg.setMsgHeaderTargetID(buddyTargetID);

   bool commRes = false;
   CombinedTargetState state;
   bool getStateRes = Program::getApp()->getTargetStateStore()->getState(buddyTargetID, state);

   // send request to node and receive response
   char* respBuf;
   NetMessage* respMsg;

   while ( (!commRes) && (getStateRes)
      && (state.reachabilityState != TargetReachabilityState_OFFLINE) )
   {
      commRes = MessagingTk::requestResponse(node, &rmMsg, NETMSGTYPE_RmChunkPathsResp,
         &respBuf, &respMsg);

      if(!commRes)
      {
         LOG_DEBUG(__func__, Log_NOTICE,
            "Unable to communicate, but target is not offline; "
            "sleeping " + StringTk::uintToStr(msgRetryIntervalMS) + " ms before retry. "
            "targetID: " + StringTk::uintToStr(targetID) );

         PThread::sleepMS(msgRetryIntervalMS);

         // if thread shall terminate, break loop here
         if ( getSelfTerminateNotIdle() )
            break;

         getStateRes = Program::getApp()->getTargetStateStore()->getState(buddyTargetID, state);
      }
   }

   if(!commRes)
   { // communication error
      LogContext(__func__).logErr(
         "Communication with storage node failed: " + node->getTypedNodeID() );

      return false;
   }
   else
   if(!getStateRes)
   {
      LogContext(__func__).log(Log_WARNING,
         "No valid state for node ID: " + node->getTypedNodeID() );

      return false;
   }
   else
   {
      // correct response type received
      RmChunkPathsRespMsg* respMsgCast = (RmChunkPathsRespMsg*) respMsg;
      StringList failedPaths;

      respMsgCast->parseFailedPaths(&failedPaths);
      for (StringListIter iter = failedPaths.begin(); iter != failedPaths.end(); iter++)
      {
         LogContext(__func__).logErr("Chunk path could not be deleted; "
            "path: " + *iter + "; "
            "targetID: " + StringTk::uintToStr(targetID) + "; "
            "node: " + node->getTypedNodeID());
         retVal = false;
      }

      delete (respMsg);
      free(respBuf);
   }

   return retVal;
}

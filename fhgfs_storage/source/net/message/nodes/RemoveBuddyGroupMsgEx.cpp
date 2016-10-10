#include "RemoveBuddyGroupMsgEx.h"

#include <common/net/message/nodes/RemoveBuddyGroupRespMsg.h>
#include <net/message/storage/listing/ListChunkDirIncrementalMsgEx.h>
#include <program/Program.h>

static FhgfsOpsErr checkChunkDirRemovable(int dirFD)
{
   DIR* dir = fdopendir(dirFD);

   struct dirent buffer;
   struct dirent* result;

   while (readdir_r(dir, &buffer, &result) == 0)
   {
      if (!result)
         goto success;

      if (strcmp(result->d_name, ".") == 0 || strcmp(result->d_name, "..") == 0)
         continue;

      struct stat statData;

      int statRes = ::fstatat(dirfd(dir), result->d_name, &statData, AT_SYMLINK_NOFOLLOW);
      if (statRes != 0)
      {
         LogContext(__func__).logErr("Could not stat something in chunk directory.");
         goto error;
      }

      if (!S_ISDIR(statData.st_mode))
         goto notempty;

      int subdir = ::openat(dirfd(dir), result->d_name, O_RDONLY);
      if (subdir < 0)
      {
         LogContext(__func__).logErr("Could not open directory in chunk path.");
         goto error;
      }

      const FhgfsOpsErr checkRes = checkChunkDirRemovable(subdir);
      if (checkRes != FhgfsOpsErr_SUCCESS)
      {
         closedir(dir);
         return checkRes;
      }
   }

error:
   closedir(dir);
   return FhgfsOpsErr_INTERNAL;

success:
   closedir(dir);
   return FhgfsOpsErr_SUCCESS;

notempty:
   closedir(dir);
   return FhgfsOpsErr_NOTEMPTY;
}

bool RemoveBuddyGroupMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
      char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   App* app = Program::getApp();

   LogContext log("RemoveBuddyGroupMsg incoming");

   LOG_DEBUG_CONTEXT(log, Log_DEBUG, "Received RemoveBuddyGroupMsg from: " + sock->getPeername());

   if (type != NODETYPE_Storage)
   {
      RemoveBuddyGroupRespMsg result(FhgfsOpsErr_INTERNAL);
      result.serialize(respBuf, bufLen);
      sock->sendto(respBuf, result.getMsgLength(), 0, NULL, 0);
      return true;
   }

   uint16_t targetID = app->getMirrorBuddyGroupMapper()->getPrimaryTargetID(groupID);
   if (app->getTargetMapper()->getNodeID(targetID) != app->getLocalNode()->getNumID())
      targetID = app->getMirrorBuddyGroupMapper()->getSecondaryTargetID(groupID);
   if (app->getTargetMapper()->getNodeID(targetID) != app->getLocalNode()->getNumID())
   {
      log.logErr("Group is not mapped on this target. groupID: "
            + StringTk::uintToStr(groupID));
      RemoveBuddyGroupRespMsg result(FhgfsOpsErr_INTERNAL);
      result.serialize(respBuf, bufLen);
      sock->sendto(respBuf, result.getMsgLength(), 0, NULL, 0);
      return true;
   }

   int dirFD = openat(Program::getApp()->getTargetFD(targetID, true), ".", O_RDONLY);
   if (dirFD < 0)
   {
      log.logErr("Could not open directory file descriptor. groupID: "
            + StringTk::uintToStr(groupID));
      RemoveBuddyGroupRespMsg result(FhgfsOpsErr_INTERNAL);
      result.serialize(respBuf, bufLen);
      sock->sendto(respBuf, result.getMsgLength(), 0, NULL, 0);
      return true;
   }

   FhgfsOpsErr checkRes = checkChunkDirRemovable(dirFD);

   if (checkRes != FhgfsOpsErr_SUCCESS)
   {
      RemoveBuddyGroupRespMsg result(checkRes);
      result.serialize(respBuf, bufLen);
      sock->sendto(respBuf, result.getMsgLength(), 0, NULL, 0);
      return true;
   }

   if (!checkOnly)
   {
      MirrorBuddyGroupMapper* bgm = Program::getApp()->getMirrorBuddyGroupMapper();

      if (!bgm->unmapMirrorBuddyGroup(groupID))
      {
         RemoveBuddyGroupRespMsg result(FhgfsOpsErr_INTERNAL);
         result.serialize(respBuf, bufLen);
         sock->sendto(respBuf, result.getMsgLength(), 0, NULL, 0);
         return true;
      }
   }

   RemoveBuddyGroupRespMsg result(FhgfsOpsErr_SUCCESS);
   result.serialize(respBuf, bufLen);
   sock->sendto(respBuf, result.getMsgLength(), 0, NULL, 0);
   return true;
}

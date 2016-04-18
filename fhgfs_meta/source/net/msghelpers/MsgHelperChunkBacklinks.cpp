#include "MsgHelperChunkBacklinks.h"

#include <common/net/message/storage/attribs/UpdateBacklinkMsg.h>
#include <common/net/message/storage/attribs/UpdateBacklinkRespMsg.h>
#include <program/Program.h>
#include <storage/MetaStore.h>

FhgfsOpsErr MsgHelperChunkBacklinks::updateBacklink(std::string parentID, std::string name)
{
   MetaStore* metaStore = Program::getApp()->getMetaStore();
   DirInode* parentDirInode = metaStore->referenceDir(parentID, true);

   FhgfsOpsErr retVal = updateBacklink(parentDirInode, name);

   metaStore->releaseDir(parentID);

   return retVal;
}

FhgfsOpsErr MsgHelperChunkBacklinks::updateBacklink(DirInode* parent, std::string name)
{
   EntryInfo entryInfo;
   parent->getEntryInfo(name, entryInfo);

   return updateBacklink(entryInfo);
}

/**
 * Note: this method works for buddymirrored files, but uses only group's primary.
 */
FhgfsOpsErr MsgHelperChunkBacklinks::updateBacklink(EntryInfo& entryInfo)
{
   const char* logContext = "MsgHelperChunkBacklinks (update backlinks)";

   App* app = Program::getApp();
   MetaStore* metaStore = app->getMetaStore();

   FhgfsOpsErr retVal = FhgfsOpsErr_SUCCESS;

   FileInode *fileInode = metaStore->referenceFile(&entryInfo);

   if (!fileInode)
   {
      LogContext(logContext).logErr("Unable to reference file with ID: " + entryInfo.getEntryID() );
      return FhgfsOpsErr_PATHNOTEXISTS;
   }

   PathInfo pathInfo;
   fileInode->getPathInfo(&pathInfo);

   StripePattern* pattern = fileInode->getStripePattern();
   const UInt16Vector* stripeTargets = pattern->getStripeTargetIDs();

   // send update msg to each stripe target
   for ( UInt16VectorConstIter iter = stripeTargets->begin(); iter != stripeTargets->end(); iter++ )
   {
      UpdateBacklinkRespMsg* updateBacklinkRespMsg;
      FhgfsOpsErr updateBacklinkRes;

      uint16_t targetID = *iter;

      // prepare request message

      unsigned serialEntryInfoBufLen = entryInfo.serialLen();
      char* serialEntryInfoBuf = (char*)malloc(serialEntryInfoBufLen);

      if(unlikely(!serialEntryInfoBuf) )
      { // malloc failed
         LogContext(logContext).log(Log_CRITICAL,
            "Memory allocation for backlink buffer failed." +
            std::string( (pattern->getPatternType() == STRIPEPATTERN_BuddyMirror) ? "Mirror " : "")+
            "TargetID: " + StringTk::uintToStr(targetID) + "; "
            "EntryID: " + entryInfo.getEntryID() );

         continue;
      }

      entryInfo.serialize(serialEntryInfoBuf);

      std::string entryID(entryInfo.getEntryID() ); // copy needed for UpdateBacklinkMsg constructor

      UpdateBacklinkMsg updateBacklinkMsg(entryID, targetID, &pathInfo,
         serialEntryInfoBuf, serialEntryInfoBufLen);

      if(pattern->getPatternType() == STRIPEPATTERN_BuddyMirror)
         updateBacklinkMsg.addMsgHeaderFeatureFlag(UPDATEBACKLINKSMSG_FLAG_BUDDYMIRROR);

      // prepare communication

      RequestResponseTarget rrTarget(targetID, app->getTargetMapper(), app->getStorageNodes() );

      rrTarget.setTargetStates(app->getTargetStateStore() );

      if(pattern->getPatternType() == STRIPEPATTERN_BuddyMirror)
         rrTarget.setMirrorInfo(app->getMirrorBuddyGroupMapper(), false);

      RequestResponseArgs rrArgs(NULL, &updateBacklinkMsg, NETMSGTYPE_UpdateBacklinkResp);

      // communicate

      FhgfsOpsErr requestRes = MessagingTk::requestResponseTarget(&rrTarget, &rrArgs);

      if(unlikely(requestRes != FhgfsOpsErr_SUCCESS) )
      { // communication error
         LogContext(logContext).log(Log_WARNING,
            "Communication with storage target failed. " +
            std::string( (pattern->getPatternType() == STRIPEPATTERN_BuddyMirror) ? "Mirror " : "")+
            "TargetID: " + StringTk::uintToStr(targetID) + "; "
            "EntryID: " + entryID);
         retVal = FhgfsOpsErr_INTERNAL;
         goto cleanup_loop;
      }

      updateBacklinkRespMsg = (UpdateBacklinkRespMsg*)rrArgs.outRespMsg;
      updateBacklinkRes = (FhgfsOpsErr)updateBacklinkRespMsg->getValue();

      if(updateBacklinkRes != FhgfsOpsErr_SUCCESS)
      {
         LogContext(logContext).logErr("Updating backlink on target failed. " +
            std::string( (pattern->getPatternType() == STRIPEPATTERN_BuddyMirror) ? "Mirror " : "")+
            "TargetID: " + StringTk::uintToStr(targetID) + "; "
            "EntryID: " + entryID + "; "
            "Error: " + std::string(FhgfsOpsErrTk::toErrString(updateBacklinkRes) ) );
         retVal = FhgfsOpsErr_INTERNAL;
      }

   cleanup_loop:
      free(serialEntryInfoBuf);
   }

   metaStore->releaseFile(entryInfo.getParentEntryID(), fileInode);

   return retVal;
}

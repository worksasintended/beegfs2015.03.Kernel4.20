#include <common/net/message/fsck/ChangeStripeTargetMsg.h>
#include <common/net/message/fsck/ChangeStripeTargetRespMsg.h>
#include <common/net/message/fsck/CreateDefDirInodesMsg.h>
#include <common/net/message/fsck/CreateDefDirInodesRespMsg.h>
#include <common/net/message/fsck/CreateEmptyContDirsMsg.h>
#include <common/net/message/fsck/CreateEmptyContDirsRespMsg.h>
#include <common/net/message/fsck/DeleteDirEntriesMsg.h>
#include <common/net/message/fsck/DeleteDirEntriesRespMsg.h>
#include <common/net/message/fsck/DeleteChunksMsg.h>
#include <common/net/message/fsck/DeleteChunksRespMsg.h>
#include <common/net/message/fsck/FixInodeOwnersMsg.h>
#include <common/net/message/fsck/FixInodeOwnersRespMsg.h>
#include <common/net/message/fsck/FixInodeOwnersInDentryMsg.h>
#include <common/net/message/fsck/FixInodeOwnersInDentryRespMsg.h>
#include <common/net/message/fsck/LinkToLostAndFoundMsg.h>
#include <common/net/message/fsck/LinkToLostAndFoundRespMsg.h>
#include <common/net/message/fsck/MoveChunkFileMsg.h>
#include <common/net/message/fsck/MoveChunkFileRespMsg.h>
#include <common/net/message/fsck/RecreateFsIDsMsg.h>
#include <common/net/message/fsck/RecreateFsIDsRespMsg.h>
#include <common/net/message/fsck/RecreateDentriesMsg.h>
#include <common/net/message/fsck/RecreateDentriesRespMsg.h>
#include <common/net/message/fsck/RemoveInodesMsg.h>
#include <common/net/message/fsck/RemoveInodesRespMsg.h>

#include <common/net/message/fsck/UpdateFileAttribsMsg.h>
#include <common/net/message/fsck/UpdateFileAttribsRespMsg.h>
#include <common/net/message/fsck/UpdateDirAttribsMsg.h>
#include <common/net/message/fsck/UpdateDirAttribsRespMsg.h>
#include <common/net/message/storage/creating/MkDirMsg.h>
#include <common/net/message/storage/creating/MkDirRespMsg.h>
#include <common/net/message/storage/creating/UnlinkFileMsg.h>
#include <common/net/message/storage/creating/UnlinkFileRespMsg.h>
#include <common/net/message/storage/attribs/SetLocalAttrMsg.h>
#include <common/net/message/storage/attribs/SetLocalAttrRespMsg.h>

#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/MetadataTk.h>

#include <program/Program.h>

#include "MsgHelperRepair.h"

void MsgHelperRepair::deleteDanglingDirEntries(Node *node, FsckDirEntryList* dentries,
   FsckDirEntryList* failedDeletes)
{
   const char* logContext = "MsgHelperRepair (deleteDanglingDirEntries)";

   bool commRes;
   char *respBuf = NULL;
   NetMessage *respMsg = NULL;

   DeleteDirEntriesMsg deleteDirEntriesMsg(dentries);

   commRes = MessagingTk::requestResponse(node, &deleteDirEntriesMsg,
      NETMSGTYPE_DeleteDirEntriesResp, &respBuf, &respMsg);

   if ( commRes )
   {
      DeleteDirEntriesRespMsg* deleteDirEntriesRespMsg = (DeleteDirEntriesRespMsg*) respMsg;

      // parse failed directory entries
      deleteDirEntriesRespMsg->parseFailedEntries(failedDeletes);

      if (! failedDeletes->empty())
      {
         for (FsckDirEntryListIter iter = failedDeletes->begin(); iter != failedDeletes->end();
            iter++)
         {
            LogContext(logContext).log(Log_CRITICAL, "Failed to delete directory entry from "
               "metadata node: " + node->getID() + " entryID: " + iter->getID());
         }
      }

      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);
   }
   else
   {
      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);

      *failedDeletes = *dentries;

      LogContext(logContext).logErr("Communication error occured with node: " + node->getID());
   }
}

void MsgHelperRepair::createDefDirInodes(Node* node, StringList* inodeIDs,
   FsckDirInodeList* createdInodes)
{
   const char* logContext = "MsgHelperRepair (createDefDirInodes)";

   StringList failedInodeIDs;

   bool commRes;
   char *respBuf = NULL;
   NetMessage *respMsg = NULL;

   CreateDefDirInodesMsg createDefDirInodesMsgEx(inodeIDs);

   commRes = MessagingTk::requestResponse(node, &createDefDirInodesMsgEx,
      NETMSGTYPE_CreateDefDirInodesResp, &respBuf, &respMsg);

   if ( commRes )
   {
      CreateDefDirInodesRespMsg* createDefDirInodesRespMsg = (CreateDefDirInodesRespMsg*) respMsg;

      // parse failed creates
      createDefDirInodesRespMsg->parseFailedInodeIDs(&failedInodeIDs);

      // parse created inodes
      createDefDirInodesRespMsg->parseCreatedInodes(createdInodes);

      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);
   }
   else
   {
      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);

      failedInodeIDs = *inodeIDs;

      LogContext(logContext).logErr("Communication error occured with node: " + node->getID());
   }

   if (! failedInodeIDs.empty())
   {
      for (StringListIter iter = failedInodeIDs.begin(); iter != failedInodeIDs.end(); iter++)
      {
         LogContext(logContext).log(Log_CRITICAL, "Failed to create default directory inode on "
            "metadata node: " + node->getID() + " entryID: " + *iter);
      }
   }
}

void MsgHelperRepair::correctInodeOwnersInDentry(Node* node, FsckDirEntryList* dentries,
   FsckDirEntryList* failedCorrections)
{
   const char* logContext = "MsgHelperRepair (correctInodeOwnersInDentry)";

   bool commRes;
   char *respBuf = NULL;
   NetMessage *respMsg = NULL;

   FixInodeOwnersInDentryMsg fixInodeOwnersMsg(dentries);

   commRes = MessagingTk::requestResponse(node, &fixInodeOwnersMsg,
      NETMSGTYPE_FixInodeOwnersInDentryResp, &respBuf, &respMsg);

   if ( commRes )
   {
      FixInodeOwnersInDentryRespMsg* fixInodeOwnersRespMsg =
         (FixInodeOwnersInDentryRespMsg*) respMsg;

      // parse failed corrections
      fixInodeOwnersRespMsg->parseFailedEntries(failedCorrections);

      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);
   }
   else
   {
      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);

      *failedCorrections = *dentries;

      LogContext(logContext).logErr("Communication error occured with node: " + node->getID());
   }

   if (! failedCorrections->empty())
   {
      for (FsckDirEntryListIter iter = failedCorrections->begin(); iter != failedCorrections->end();
         iter++)
      {
         LogContext(logContext).log(Log_CRITICAL, "Failed to correct inode owner information in "
            "dentry on on metadata node: " + node->getID() + " entryID: " + iter->getID());
      }
   }
}

void MsgHelperRepair::correctInodeOwners(Node* node, FsckDirInodeList* dirInodes,
   FsckDirInodeList* failedCorrections)
{
   const char* logContext = "MsgHelperRepair (correctInodeOwners)";

   bool commRes;
   char *respBuf = NULL;
   NetMessage *respMsg = NULL;

   FixInodeOwnersMsg fixInodeOwnersMsg(dirInodes);

   commRes = MessagingTk::requestResponse(node, &fixInodeOwnersMsg,
      NETMSGTYPE_FixInodeOwnersResp, &respBuf, &respMsg);

   if ( commRes )
   {
      FixInodeOwnersRespMsg* fixInodeOwnersRespMsg = (FixInodeOwnersRespMsg*) respMsg;

      // parse failed corrections
      fixInodeOwnersRespMsg->parseFailedInodes(failedCorrections);

      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);
   }
   else
   {
      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);

      *failedCorrections = *dirInodes;

      LogContext(logContext).logErr("Communication error occured with node: " + node->getID());
   }

   if (! failedCorrections->empty())
   {
      for (FsckDirInodeListIter iter = failedCorrections->begin(); iter != failedCorrections->end();
         iter++)
      {
         LogContext(logContext).log(Log_CRITICAL, "Failed to correct inode owner information on "
            "metadata node: " + node->getID() + " entryID: " + iter->getID());
      }
   }
}

void MsgHelperRepair::deleteFiles(Node* node, FsckDirEntryList* dentries,
   FsckDirEntryList* failedDeletes)
{
   const char* logContext = "MsgHelperRepair (deleteFiles)";

   for ( FsckDirEntryListIter iter = dentries->begin(); iter != dentries->end(); iter++ )
   {
      bool commRes;
      char *respBuf = NULL;
      NetMessage *respMsg = NULL;

      EntryInfo parentInfo(node->getNumID(), "", iter->getParentDirID(), "", DirEntryType_DIRECTORY,
         0);

      std::string entryName = iter->getName();
      UnlinkFileMsg unlinkFileMsg(&parentInfo, entryName);

      commRes = MessagingTk::requestResponse(node, &unlinkFileMsg, NETMSGTYPE_UnlinkFileResp,
         &respBuf, &respMsg);

      if ( commRes )
      {
         UnlinkFileRespMsg* unlinkFileRespMsg = (UnlinkFileRespMsg*) respMsg;

         // get result
         int unlinkRes = unlinkFileRespMsg->getValue();

         if ( unlinkRes )
         {
            LogContext(logContext).log(Log_CRITICAL,
               "Failed to delete file; entryID: " + iter->getID());
            failedDeletes->push_back(*iter);
         }

         SAFE_FREE(respBuf);
         SAFE_DELETE(respMsg);
      }
      else
      {
         SAFE_FREE(respBuf);
         SAFE_DELETE(respMsg);

         failedDeletes->push_back(*iter);

         LogContext(logContext).logErr("Communication error occured with node: " + node->getID());
      }

   }
}

void MsgHelperRepair::deleteChunks(Node* node, FsckChunkList* chunks, FsckChunkList* failedDeletes)
{
   const char* logContext = "MsgHelperRepair (deleteChunks)";

   bool commRes;
   char *respBuf = NULL;
   NetMessage *respMsg = NULL;

   DeleteChunksMsg deleteChunksMsg(chunks);

   commRes = MessagingTk::requestResponse(node, &deleteChunksMsg,
      NETMSGTYPE_DeleteChunksResp, &respBuf, &respMsg);

   if ( commRes )
   {
      DeleteChunksRespMsg* deleteChunksRespMsg = (DeleteChunksRespMsg*) respMsg;

      // parse failed deletes
      deleteChunksRespMsg->parseFailedChunks(failedDeletes);

      if (! failedDeletes->empty())
      {
         for (FsckChunkListIter iter = failedDeletes->begin(); iter != failedDeletes->end();
            iter++)
         {
            LogContext(logContext).log(Log_CRITICAL, "Failed to delete chunk entry. targetID: " +
               StringTk::uintToStr(iter->getTargetID()) + " chunkID: " + iter->getID());
         }
      }

      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);
   }
   else
   {
      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);

      *failedDeletes = *chunks;

      LogContext(logContext).logErr("Communication error occured with node: " + node->getID());
   }
}

Node* MsgHelperRepair::referenceLostAndFoundOwner(EntryInfo* outLostAndFoundEntryInfo)
{
   const char* logContext = "MsgHelperRepair (referenceLostAndFoundOwner)";

// find owner node
   Path path(META_LOSTANDFOUND_PATH);
   path.setAbsolute(true);

   NodeStore* metaNodes = Program::getApp()->getMetaNodes();

   Node* ownerNode = NULL;

   FhgfsOpsErr findRes = MetadataTk::referenceOwner(&path, false, metaNodes, &ownerNode,
      outLostAndFoundEntryInfo);

   if ( findRes != FhgfsOpsErr_SUCCESS )
   {
      LogContext(logContext).log(Log_DEBUG, "No owner node found for lost+found. Directory does not"
         " seem to exist (yet)");
   }

   return ownerNode;
}

bool MsgHelperRepair::createLostAndFound(Node** outReferencedNode,
   EntryInfo& outLostAndFoundEntryInfo)
{
   const char* logContext = "MsgHelperRepair (createLostAndFound)";

   bool retVal = false;

   NodeStore* metaNodes = Program::getApp()->getMetaNodes();

   // get root owner node and entryInfo
   Node* rootNode = NULL;
   Path rootPath("");
   // rootPath.setAbsolute(true);
   EntryInfo rootEntryInfo;

   FhgfsOpsErr findRes = MetadataTk::referenceOwner(&rootPath, false, metaNodes, &rootNode,
      &rootEntryInfo);

   if ( findRes != FhgfsOpsErr_SUCCESS )
   {
      LogContext(logContext).log(Log_CRITICAL, "Unable to reference metadata node for root "
         "directory");
      return false;
   }

   // create the directory
   std::string lostFoundPathStr = META_LOSTANDFOUND_PATH;
   UInt16List preferredNodes;

   MkDirMsg mkDirMsg(&rootEntryInfo, lostFoundPathStr , 0, 0, S_IFDIR | S_IRWXU | S_IRWXG,
      &preferredNodes);

   // request/response
   char* respBuf = NULL;
   NetMessage* respMsg = NULL;
   MkDirRespMsg* mkDirRespMsg;
   FhgfsOpsErr mkDirRes;

   bool commRes = MessagingTk::requestResponse(rootNode, &mkDirMsg, NETMSGTYPE_MkDirResp, &respBuf,
      &respMsg);

   if ( !commRes )
      goto err_cleanup;

   mkDirRespMsg = (MkDirRespMsg*) respMsg;

   mkDirRes = (FhgfsOpsErr) mkDirRespMsg->getResult();
   if ( mkDirRes != FhgfsOpsErr_SUCCESS )
   {
      LogContext(logContext).log(Log_CRITICAL,
         std::string("Node encountered an error: ") + FhgfsOpsErrTk::toErrString(mkDirRes));
      goto err_cleanup;
   }

   // create seems to have succeeded
   // copy is created because delete is called on mkDirRespMsg, but we still need this object
   outLostAndFoundEntryInfo = *(mkDirRespMsg->getEntryInfo());

   *outReferencedNode = metaNodes->referenceNode(outLostAndFoundEntryInfo.getOwnerNodeID());
   retVal = true;

   err_cleanup:
   metaNodes->releaseNode(&rootNode);
   SAFE_DELETE(respMsg);
   SAFE_FREE(respBuf);

   return retVal;
}

void MsgHelperRepair::linkToLostAndFound(Node* lostAndFoundNode, EntryInfo* lostAndFoundInfo,
   FsckDirInodeList* dirInodes, FsckDirInodeList* failedInodes, FsckDirEntryList* createdDentries)
{
   const char* logContext = "MsgHelperRepair (linkToLostAndFound)";

   bool commRes;
   char *respBuf = NULL;
   NetMessage *respMsg = NULL;

   LinkToLostAndFoundMsg linkToLostAndFoundMsg(lostAndFoundInfo, dirInodes);

   commRes = MessagingTk::requestResponse(lostAndFoundNode, &linkToLostAndFoundMsg,
      NETMSGTYPE_LinkToLostAndFoundResp, &respBuf, &respMsg);

   if ( commRes )
   {
      LinkToLostAndFoundRespMsg* linkToLostAndFoundRespMsg = (LinkToLostAndFoundRespMsg*) respMsg;

      // parse failed inserts
      linkToLostAndFoundRespMsg->parseFailedInodes(failedInodes);

      // parse created dentries
      linkToLostAndFoundRespMsg->parseCreatedDirEntries(createdDentries);

      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);
   }
   else
   {
      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);

      *failedInodes = *dirInodes;

      LogContext(logContext).logErr("Communication error occured with node: " +
         lostAndFoundNode->getID());
   }

   if (! failedInodes->empty())
   {
      for (FsckDirInodeListIter iter = failedInodes->begin(); iter != failedInodes->end();
         iter++)
      {
         LogContext(logContext).log(Log_CRITICAL, "Failed to link directory inode to lost+found. "
            "entryID: " + iter->getID());
      }
   }
}

void MsgHelperRepair::linkToLostAndFound(Node* lostAndFoundNode, EntryInfo* lostAndFoundInfo,
   FsckFileInodeList* fileInodes, FsckFileInodeList* failedInodes,
   FsckDirEntryList* createdDentries)
{
   const char* logContext = "MsgHelperRepair (linkToLostAndFound)";

   bool commRes;
   char *respBuf = NULL;
   NetMessage *respMsg = NULL;

   LinkToLostAndFoundMsg linkToLostAndFoundMsg(lostAndFoundInfo, fileInodes);

   commRes = MessagingTk::requestResponse(lostAndFoundNode, &linkToLostAndFoundMsg,
      NETMSGTYPE_LinkToLostAndFoundResp, &respBuf, &respMsg);

   if ( commRes )
   {
      LinkToLostAndFoundRespMsg* linkToLostAndFoundRespMsg = (LinkToLostAndFoundRespMsg*) respMsg;

      // parse failed inserts
      linkToLostAndFoundRespMsg->parseFailedInodes(failedInodes);

      // parse created dentries
      linkToLostAndFoundRespMsg->parseCreatedDirEntries(createdDentries);

      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);
   }
   else
   {
      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);

      *failedInodes = *fileInodes;

      LogContext(logContext).logErr("Communication error occured with node: " +
         lostAndFoundNode->getID());
   }

   if (! failedInodes->empty())
   {
      for (FsckFileInodeListIter iter = failedInodes->begin(); iter != failedInodes->end();
         iter++)
      {
         LogContext(logContext).log(Log_CRITICAL, "Failed to link file inode to lost+found. "
            "entryID: " + iter->getID());
      }
   }
}

void MsgHelperRepair::createContDirs(Node* node, FsckDirInodeList* inodes,
   StringList* failedCreates)
{
   const char* logContext = "MsgHelperRepair (createContDirs)";

   // create a string list with the IDs
   StringList directoryIDs;
   for (FsckDirInodeListIter iter = inodes->begin(); iter != inodes->end(); iter++)
   {
      directoryIDs.push_back(iter->getID());
   }

   bool commRes;
   char *respBuf = NULL;
   NetMessage *respMsg = NULL;

   CreateEmptyContDirsMsg createContDirsMsg(&directoryIDs);

   commRes = MessagingTk::requestResponse(node, &createContDirsMsg,
      NETMSGTYPE_CreateEmptyContDirsResp, &respBuf, &respMsg);

   if ( commRes )
   {
      CreateEmptyContDirsRespMsg* createContDirsRespMsg = (CreateEmptyContDirsRespMsg*) respMsg;

      // parse failed inserts
      createContDirsRespMsg->parseFailedIDs(failedCreates);

      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);
   }
   else
   {
      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);

      *failedCreates = directoryIDs;

      LogContext(logContext).logErr(
         "Communication error occured with node: " + node->getID());
   }

   if ( !failedCreates->empty() )
   {
      for ( StringListIter iter = failedCreates->begin(); iter != failedCreates->end();
         iter++ )
      {
         LogContext(logContext).log(Log_CRITICAL, "Failed to create empty content directory. "
            "directoryID: " + *iter);
      }
   }
}

void MsgHelperRepair::updateFileAttribs(Node* node, FsckFileInodeList* inodes,
   FsckFileInodeList* failedUpdates)
{
   const char* logContext = "MsgHelperRepair (updateFileAttribs)";

   bool commRes;
   char *respBuf = NULL;
   NetMessage *respMsg = NULL;

   UpdateFileAttribsMsg updateFileAttribsMsg(inodes);

   commRes = MessagingTk::requestResponse(node, &updateFileAttribsMsg,
      NETMSGTYPE_UpdateFileAttribsResp, &respBuf, &respMsg);

   if ( commRes )
   {
      UpdateFileAttribsRespMsg* updateFileAttribsRespMsg = (UpdateFileAttribsRespMsg*) respMsg;

      // parse failed update
      updateFileAttribsRespMsg->parseFailedInodes(failedUpdates);

      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);
   }
   else
   {
      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);

      *failedUpdates = *inodes;

      LogContext(logContext).logErr("Communication error occured with node: " + node->getID());
   }

   if ( !failedUpdates->empty() )
   {
      for ( FsckFileInodeListIter iter = failedUpdates->begin(); iter != failedUpdates->end();
         iter++ )
      {
         LogContext(logContext).log(Log_CRITICAL, "Failed to update attributes of file inode. "
            "entryID: " + iter->getID());
      }
   }
}

void MsgHelperRepair::updateDirAttribs(Node* node, FsckDirInodeList* inodes,
   FsckDirInodeList* failedUpdates)
{
   const char* logContext = "MsgHelperRepair (updateDirAttribs)";

   bool commRes;
   char *respBuf = NULL;
   NetMessage *respMsg = NULL;

   UpdateDirAttribsMsg updateDirAttribsMsg(inodes);

   commRes = MessagingTk::requestResponse(node, &updateDirAttribsMsg,
      NETMSGTYPE_UpdateDirAttribsResp, &respBuf, &respMsg);

   if ( commRes )
   {
      UpdateDirAttribsRespMsg* updateDirAttribsRespMsg = (UpdateDirAttribsRespMsg*) respMsg;

      // parse failed update
      updateDirAttribsRespMsg->parseFailedInodes(failedUpdates);

      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);
   }
   else
   {
      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);

      *failedUpdates = *inodes;

      LogContext(logContext).logErr("Communication error occured with node: " + node->getID());
   }

   if ( !failedUpdates->empty() )
   {
      for ( FsckDirInodeListIter iter = failedUpdates->begin(); iter != failedUpdates->end();
         iter++ )
      {
         LogContext(logContext).log(Log_CRITICAL, "Failed to update attributes of directory inode."
            " entryID: " + iter->getID());
      }
   }
}

void MsgHelperRepair::changeStripeTarget(Node* node, FsckFileInodeList* inodes, uint16_t targetID,
   uint16_t newTargetID, FsckFileInodeList* failedInodes)
{
   const char* logContext = "MsgHelperRepair (changeStripeTarget)";

   bool commRes;
   char *respBuf = NULL;
   NetMessage *respMsg = NULL;

   ChangeStripeTargetMsg changeStripeTargetMsg(inodes, targetID, newTargetID);

   commRes = MessagingTk::requestResponse(node, &changeStripeTargetMsg,
      NETMSGTYPE_ChangeStripeTargetResp, &respBuf, &respMsg);

   if ( commRes )
   {
      ChangeStripeTargetRespMsg* changeStripeTargetRespMsg = (ChangeStripeTargetRespMsg*) respMsg;

      // parse failed update
      changeStripeTargetRespMsg->parseFailedInodes(failedInodes);

      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);
   }
   else
   {
      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);

      *failedInodes = *inodes;

      LogContext(logContext).logErr("Communication error occured with node: " + node->getID());
   }

   if ( !failedInodes->empty() )
   {
      for ( FsckFileInodeListIter iter = failedInodes->begin(); iter != failedInodes->end();
         iter++ )
      {
         LogContext(logContext).log(Log_CRITICAL, "Failed to change stripe pattern of inode."
            " entryID: " + iter->getID());
      }
   }
}

void MsgHelperRepair::recreateFsIDs(Node* node,  FsckDirEntryList* dentries,
   FsckDirEntryList* failedEntries)
{
   const char* logContext = "MsgHelperRepair (recreateFsIDs)";

   bool commRes;
   char *respBuf = NULL;
   NetMessage *respMsg = NULL;

   RecreateFsIDsMsg recreateFsIDsMsg(dentries);

   commRes = MessagingTk::requestResponse(node, &recreateFsIDsMsg,
      NETMSGTYPE_RecreateFsIDsResp, &respBuf, &respMsg);

   if ( commRes )
   {
      RecreateFsIDsRespMsg* recreateFsIDsRespMsg = (RecreateFsIDsRespMsg*) respMsg;

      // parse failed creates
      recreateFsIDsRespMsg->parseFailedEntries(failedEntries);

      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);
   }
   else
   {
      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);

      *failedEntries = *dentries;

      LogContext(logContext).logErr("Communication error occured with node: " + node->getID());
   }

   if ( !failedEntries->empty() )
   {
      for ( FsckDirEntryListIter iter = failedEntries->begin(); iter != failedEntries->end();
         iter++ )
      {
         LogContext(logContext).log(Log_CRITICAL, "Failed to recreate dentry-by-ID file link."
            " entryID: " + iter->getID());
      }
   }
}

void MsgHelperRepair::recreateDentries(Node* node, FsckFsIDList* fsIDs, FsckFsIDList* failedCreates,
   FsckDirEntryList* createdDentries, FsckFileInodeList* createdInodes)
{
   const char* logContext = "MsgHelperRepair (recreateDentries)";

   bool commRes;
   char *respBuf = NULL;
   NetMessage *respMsg = NULL;

   RecreateDentriesMsg recreateDentriesMsg(fsIDs);

   commRes = MessagingTk::requestResponse(node, &recreateDentriesMsg,
      NETMSGTYPE_RecreateDentriesResp, &respBuf, &respMsg);

   if ( commRes )
   {
      RecreateDentriesRespMsg* recreateDentriesMsg = (RecreateDentriesRespMsg*) respMsg;

      // parse failed creates
      recreateDentriesMsg->parseFailedCreates(failedCreates);
      recreateDentriesMsg->parseCreatedDentries(createdDentries);
      recreateDentriesMsg->parseCreatedInodes(createdInodes);

      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);
   }
   else
   {
      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);

      *failedCreates = *fsIDs;

      LogContext(logContext).logErr("Communication error occured with node: " + node->getID());
   }

   if ( !failedCreates->empty() )
   {
      for ( FsckFsIDListIter iter = failedCreates->begin(); iter != failedCreates->end();
         iter++ )
      {
         LogContext(logContext).log(Log_CRITICAL, "Failed to recreate dentry."
            " entryID: " + iter->getID());
      }
   }
}

void MsgHelperRepair::fixChunkPermissions(Node* node, FsckChunkList& chunkList,
   PathInfoList& pathInfoList, FsckChunkList& failedChunks)
{
   const char* logContext = "MsgHelperRepair (fixChunkPermissions)";

   if ( chunkList.size() != pathInfoList.size() )
   {
      LogContext(logContext).logErr(
         "Failed to set uid/gid for chunks. Size of lists does not match.");
      return;
   }

   FsckChunkListIter chunksIter = chunkList.begin();
   PathInfoListIter pathInfoIter = pathInfoList.begin();
   for ( ; chunksIter != chunkList.end(); chunksIter++, pathInfoIter++ )
   {
      bool commRes;
      char *respBuf = NULL;
      NetMessage *respMsg = NULL;

      std::string chunkID = chunksIter->getID();
      uint16_t targetID = chunksIter->getTargetID();
      int validAttribs = SETATTR_CHANGE_USERID | SETATTR_CHANGE_GROUPID; // only interested in these
      SettableFileAttribs attribs;
      attribs.userID = chunksIter->getUserID();
      attribs.groupID = chunksIter->getGroupID();

      bool enableCreation = false;

      PathInfo pathInfo = *pathInfoIter;
      SetLocalAttrMsg setLocalAttrMsg(chunkID, targetID, &pathInfo, validAttribs, &attribs,
         enableCreation);
      setLocalAttrMsg.addMsgHeaderFeatureFlag(SETLOCALATTRMSG_FLAG_USE_QUOTA);

      commRes = MessagingTk::requestResponse(node, &setLocalAttrMsg, NETMSGTYPE_SetLocalAttrResp,
         &respBuf, &respMsg);

      if ( commRes )
      {
         SetLocalAttrRespMsg* setLocalAttrRespMsg = (SetLocalAttrRespMsg*) respMsg;

         if ( setLocalAttrRespMsg->getValue() != FhgfsOpsErr_SUCCESS )
         {
            LogContext(logContext).logErr(
               "Failed to set uid/gid for chunk. chunkID: " + chunkID + "; targetID: "
                  + StringTk::uintToStr(targetID));

            failedChunks.push_back(*chunksIter);
         }
      }
      else
      {
         LogContext(logContext).logErr("Communication error occured with node: " + node->getID());

         failedChunks.push_back(*chunksIter);
      }

      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);
   }
}

/*
 * NOTE: chunkList gets modified! (new savedPath is set)
 */
// TODO Christian: this has to work with buddy mirroring
void MsgHelperRepair::moveChunks(Node* node, FsckChunkList& chunkList, StringList& moveToList,
   FsckChunkList& failedChunks)
{
   /*
   const char* logContext = "MsgHelperRepair (moveChunks)";

   if (chunkList.size() != moveToList.size())
   {
      // this must be a programming error
      LogContext(logContext).logErr(
         "Failed to move chunks. Size of lists does not equal!");
      return;
   }

   FsckChunkListIter chunkIter = chunkList.begin();
   StringListIter moveToIter = moveToList.begin();
   for (; chunkIter != chunkList.end(); chunkIter++, moveToIter++)
   {
      bool commRes;
      char *respBuf = NULL;
      NetMessage *respMsg = NULL;

      std::string chunkID = chunkIter->getID();
      uint16_t targetID = chunkIter->getTargetID();
      uint16_t buddyGroupID = chunkIter->getBuddyGroupID();
      std::string oldPath = chunkIter->getSavedPath()->getPathAsStr();
      std::string newPath = *moveToIter;
      bool overwriteExisting = false;

      MoveChunkFileMsg moveChunkFileMsg(chunkID, targetID, mirroredFrom, oldPath, newPath,
         overwriteExisting);

      commRes = MessagingTk::requestResponse(node, &moveChunkFileMsg,
         NETMSGTYPE_MoveChunkFileResp, &respBuf, &respMsg);

      if ( commRes )
      {
         MoveChunkFileRespMsg* moveChunkFileRespMsg = (MoveChunkFileRespMsg*) respMsg;

         if (moveChunkFileRespMsg->getValue() != FhgfsOpsErr_SUCCESS)
         {
            LogContext(logContext).logErr(
               "Failed to move chunk. chunkID: " + chunkID + "; targetID: "
                  + StringTk::uintToStr(targetID) + "; fromPath: " + oldPath + "; toPath: "
                  + newPath);

            failedChunks.push_back(*chunkIter);
         }
      }
      else
      {
         LogContext(logContext).logErr("Communication error occured with node: " + node->getID());

         failedChunks.push_back(*chunkIter);
      }

      // set newPath in chunk
      chunkIter->setSavedPath(Path(newPath));

      SAFE_FREE(respBuf);
      SAFE_DELETE(respMsg);
   }
   */
}

void MsgHelperRepair::deleteFileInodes(Node* node, FsckFileInodeList& inodes,
   StringList& failedDeletes)
{
   const char* logContext = "MsgHelperRepair (deleteFileInodes)";

   StringList entryIDList;
   DirEntryTypeList entryTypeList;

   for ( FsckFileInodeListIter iter = inodes.begin(); iter != inodes.end(); iter++ )
   {
      entryIDList.push_back(iter->getID());
      entryTypeList.push_back(DirEntryType_REGULARFILE);
   }

   bool commRes;
   char *respBuf = NULL;
   NetMessage *respMsg = NULL;

   RemoveInodesMsg removeInodesMsg(&entryIDList, &entryTypeList);

   commRes = MessagingTk::requestResponse(node, &removeInodesMsg,
      NETMSGTYPE_RemoveInodesResp, &respBuf, &respMsg);

   if ( commRes )
   {
      RemoveInodesRespMsg* removeInodesRespMsg = (RemoveInodesRespMsg*) respMsg;
      removeInodesRespMsg->parseFailedEntryIDList(&failedDeletes);

      for ( StringListIter iter = failedDeletes.begin(); iter != failedDeletes.end();
         iter++ )
      {
         LogContext(logContext).logErr(
            "Failed to delete file inode; nodeID: " + StringTk::uintToStr(node->getNumID())
               + "; entryID: " + *iter);
      }

   }
   else
   {
      LogContext(logContext).logErr("Communication error occured with node: " + node->getID());

      failedDeletes = entryIDList;
   }

   SAFE_FREE(respBuf);
   SAFE_DELETE(respMsg);
}

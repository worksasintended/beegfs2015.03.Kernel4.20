#include <common/net/message/nodes/GenericDebugRespMsg.h>
#include <common/net/msghelpers/MsgHelperGenericDebug.h>
#include <common/storage/quota/Quota.h>
#include <common/toolkit/MessagingTk.h>
#include <program/Program.h>
#include <session/SessionStore.h>
#include "GenericDebugMsgEx.h"


#define GENDBGMSG_OP_LISTFILEAPPENDLOCKS  "listfileappendlocks"
#define GENDBGMSG_OP_LISTFILEENTRYLOCKS   "listfileentrylocks"
#define GENDBGMSG_OP_LISTFILERANGELOCKS   "listfilerangelocks"
#define GENDBGMSG_OP_LISTOPENFILES        "listopenfiles"
#define GENDBGMSG_OP_REFERENCESTATISTICS  "refstats"
#define GENDBGMSG_OP_CACHESTATISTICS      "cachestats"
#define GENDBGMSG_OP_VERSION              "version"
#define GENDBGMSG_OP_MIRRORERSTATS        "mirrorerstats"
#define GENDBGMSG_OP_MSGQUEUESTATS        "msgqueuestats"
#define GENDBGMSG_OP_LISTPOOLS            "listpools"
#define GENDBGMSG_OP_DUMPDENTRY           "dumpdentry"
#define GENDBGMSG_OP_DUMPINODE            "dumpinode"
#define GENDBGMSG_OP_DUMPINLINEDINODE     "dumpinlinedinode"

#ifdef BEEGFS_DEBUG
   #define GENDBGMSG_OP_WRITEDIRDENTRY       "writedirdentry"
   #define GENDBGMSG_OP_WRITEDIRINODE        "writedirinode"
   #define GENDBGMSG_OP_WRITEFILEINODE       "writefileinode"
#endif // BEEGFS_DEBUG


bool GenericDebugMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("GenericDebugMsg incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG_CONTEXT(log, 4, std::string("Received a GenericDebugMsg from: ") + peer);

   LOG_DEBUG_CONTEXT(log, 5, std::string("Command string: ") + getCommandStr() );

   std::string cmdRespStr = processCommand();


   // send response

   GenericDebugRespMsg respMsg(cmdRespStr.c_str() );
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   return true;
}

/**
 * @return command response string
 */
std::string GenericDebugMsgEx::processCommand()
{
   App* app = Program::getApp();
   Config* cfg = app->getConfig();

   std::string responseStr;
   std::string operation;

   // load command string into a stream to allow us to use getline
   std::istringstream commandStream(getCommandStr() );

   // get operation type from command string
   std::getline(commandStream, operation, ' ');

   if(operation == GENDBGMSG_OP_LISTFILEAPPENDLOCKS)
      responseStr = processOpListFileAppendLocks(commandStream);
   else
   if(operation == GENDBGMSG_OP_LISTFILEENTRYLOCKS)
      responseStr = processOpListFileEntryLocks(commandStream);
   else
   if(operation == GENDBGMSG_OP_LISTFILERANGELOCKS)
      responseStr = processOpListFileRangeLocks(commandStream);
   else
   if(operation == GENDBGMSG_OP_LISTOPENFILES)
      responseStr = processOpListOpenFiles(commandStream);
   else
   if(operation == GENDBGMSG_OP_REFERENCESTATISTICS)
      responseStr = processOpReferenceStatistics(commandStream);
   else
   if(operation == GENDBGMSG_OP_CACHESTATISTICS)
      responseStr = processOpCacheStatistics(commandStream);
   else
   if(operation == GENDBGMSG_OP_VERSION)
      responseStr = processOpVersion(commandStream);
   else
   if(operation == GENDBGMSG_OP_MIRRORERSTATS)
      responseStr = processOpMirrorerStats(commandStream);
   else
   if(operation == GENDBGMSG_OP_MSGQUEUESTATS)
      responseStr = processOpMsgQueueStats(commandStream);
   else
   if(operation == GENDBGMSG_OP_VARLOGMESSAGES)
      responseStr = MsgHelperGenericDebug::processOpVarLogMessages(commandStream);
   else
   if(operation == GENDBGMSG_OP_VARLOGKERNLOG)
      responseStr = MsgHelperGenericDebug::processOpVarLogKernLog(commandStream);
   else
   if(operation == GENDBGMSG_OP_FHGFSLOG)
      responseStr = MsgHelperGenericDebug::processOpFhgfsLog(commandStream);
   else
   if(operation == GENDBGMSG_OP_LOADAVG)
      responseStr = MsgHelperGenericDebug::processOpLoadAvg(commandStream);
   else
   if(operation == GENDBGMSG_OP_DROPCACHES)
      responseStr = MsgHelperGenericDebug::processOpDropCaches(commandStream);
   else
   if(operation == GENDBGMSG_OP_GETCFG)
      responseStr = MsgHelperGenericDebug::processOpCfgFile(commandStream, cfg->getCfgFile() );
   else
   if(operation == GENDBGMSG_OP_GETLOGLEVEL)
      responseStr = MsgHelperGenericDebug::processOpGetLogLevel(commandStream);
   else
   if(operation == GENDBGMSG_OP_SETLOGLEVEL)
      responseStr = MsgHelperGenericDebug::processOpSetLogLevel(commandStream);
   else
   if(operation == GENDBGMSG_OP_NETOUT)
      responseStr = MsgHelperGenericDebug::processOpNetOut(commandStream,
         app->getMgmtNodes(), app->getMetaNodes(), app->getStorageNodes() );
   else
   if(operation == GENDBGMSG_OP_LISTMETASTATES)
      responseStr = MsgHelperGenericDebug::processOpListTargetStates(commandStream,
         app->getMetaStateStore() );
   else
   if(operation == GENDBGMSG_OP_LISTSTORAGESTATES)
      responseStr = MsgHelperGenericDebug::processOpListTargetStates(commandStream,
         app->getTargetStateStore() );
   else
   if(operation == GENDBGMSG_OP_LISTPOOLS)
      responseStr = processOpListPools(commandStream);
   else
   if(operation == GENDBGMSG_OP_DUMPDENTRY)
      responseStr = processOpDumpDentry(commandStream);
   else
   if(operation == GENDBGMSG_OP_DUMPINODE)
      responseStr = processOpDumpInode(commandStream);
   else
   if(operation == GENDBGMSG_OP_DUMPINLINEDINODE)
      responseStr = processOpDumpInlinedInode(commandStream);
   else
   if(operation == GENDBGMSG_OP_QUOTAEXCEEDED)
      responseStr = processOpQuotaExceeded(commandStream);
#ifdef BEEGFS_DEBUG
   else
   if(operation == GENDBGMSG_OP_WRITEDIRDENTRY)
      responseStr = processOpWriteDirDentry(commandStream);
   else
   if(operation == GENDBGMSG_OP_WRITEDIRINODE)
      responseStr = processOpWriteDirInode(commandStream);
   else
   if(operation == GENDBGMSG_OP_WRITEFILEINODE)
      responseStr = processOpWriteInlinedFileInode(commandStream);
#endif // BEEGFS_DEBUG
   else
      responseStr = "Unknown/invalid operation";

   return responseStr;
}

/**
 * Retrieve append lock stats for a certain file.
 */
std::string GenericDebugMsgEx::processOpListFileAppendLocks(std::istringstream& commandStream)
{
   // procotol: entryID as only argument

   std::string parentEntryID;
   std::string entryID;
   std::string responseStr;

   // get entryID from command string
   std::getline(commandStream, parentEntryID, ' ');
   std::getline(commandStream, entryID, ' ');

   if (parentEntryID.empty() )
      return "Invalid or missing parentEntryID";

   if(entryID.empty() )
      return "Invalid or missing entryID";

   MetaStore* metaStore = Program::getApp()->getMetaStore();
   FileInode* inode = metaStore->referenceLoadedFile(parentEntryID, entryID);
   if(!inode)
      return "FileID not exists: " + entryID;

   responseStr = inode->flockAppendGetAllAsStr();

   metaStore->releaseFile(parentEntryID, inode);

   return responseStr;
}

std::string GenericDebugMsgEx::processOpListFileEntryLocks(std::istringstream& commandStream)
{
   // procotol: entryID as only argument

   std::string parentEntryID;
   std::string entryID;
   std::string responseStr;

   // get entryID from command string
   std::getline(commandStream, parentEntryID, ' ');
   std::getline(commandStream, entryID, ' ');

   if (parentEntryID.empty() )
      return "Invalid or missing parentEntryID";

   if(entryID.empty() )
      return "Invalid or missing entryID";

   MetaStore* metaStore = Program::getApp()->getMetaStore();
   FileInode* inode = metaStore->referenceLoadedFile(parentEntryID, entryID);
   if(!inode)
      return "FileID not exists: " + entryID;

   responseStr = inode->flockEntryGetAllAsStr();

   metaStore->releaseFile(parentEntryID, inode);

   return responseStr;
}

std::string GenericDebugMsgEx::processOpListFileRangeLocks(std::istringstream& commandStream)
{
   // procotol: entryID as only argument

   std::string parentEntryID;
   std::string entryID;

   // get parentEntryID from command string
   std::getline(commandStream, parentEntryID, ' ');

   if(parentEntryID.empty() )
      return "Invalid or missing parentEntryID";

   // get entryID from command string
   std::getline(commandStream, entryID, ' ');

   if(entryID.empty() )
      return "Invalid or missing entryID";

   MetaStore* metaStore = Program::getApp()->getMetaStore();
   FileInode* file = metaStore->referenceLoadedFile(parentEntryID, entryID);
   if(!file)
      return "FileID not found: " + entryID;

   std::string responseStr = file->flockRangeGetAllAsStr();

   metaStore->releaseFile(parentEntryID, file);

   return responseStr;
}

std::string GenericDebugMsgEx::processOpListOpenFiles(std::istringstream& commandStream)
{
   // protocol: no arguments

   App* app = Program::getApp();
   SessionStore* sessions = app->getSessions();

   std::ostringstream responseStream;
   StringList sessionIDs;
   size_t numFilesTotal = 0;
   size_t numCheckedSessions = 0; // may defer from number of initially queried sessions

   size_t numSessions = sessions->getAllSessionIDs(&sessionIDs);

   responseStream << "Found " << numSessions << " sessions." << std::endl;

   responseStream << std::endl;

   // walk over all sessions
   for(StringListConstIter iter = sessionIDs.begin(); iter != sessionIDs.end(); iter++)
   {
      // note: sessionID might have become removed since we queried it, e.g. because client is gone

      Session* session = sessions->referenceSession(*iter, false);
      if(!session)
         continue;

      numCheckedSessions++;

      SessionFileStore* sessionFiles = session->getFiles();

      size_t numFiles = sessionFiles->getSize();

      sessions->releaseSession(session);

      if(!numFiles)
         continue; // only print sessions with open files

      numFilesTotal += numFiles;

      responseStream << *iter << ": " << numFiles << std::endl;
   }

   responseStream << std::endl;

   responseStream << "Final results: " << numFilesTotal << " open files in " <<
      numCheckedSessions << " checked sessions";

   return responseStream.str();
}

std::string GenericDebugMsgEx::processOpReferenceStatistics(std::istringstream& commandStream)
{
   // protocol: no arguments

   App* app = Program::getApp();
   MetaStore* metaStore = app->getMetaStore();

   std::ostringstream responseStream;
   size_t numDirs;
   size_t numFiles;

   metaStore->getReferenceStats(&numDirs, &numFiles);

   responseStream << "Dirs: " << numDirs << std::endl;
   responseStream << "Files: " << numFiles;

   return responseStream.str();
}

std::string GenericDebugMsgEx::processOpCacheStatistics(std::istringstream& commandStream)
{
   // protocol: no arguments

   App* app = Program::getApp();
   MetaStore* metaStore = app->getMetaStore();

   std::ostringstream responseStream;
   size_t numDirs;

   metaStore->getCacheStats(&numDirs);

   responseStream << "Dirs: " << numDirs;

   return responseStream.str();
}

std::string GenericDebugMsgEx::processOpVersion(std::istringstream& commandStream)
{
   return BEEGFS_VERSION;
}

std::string GenericDebugMsgEx::processOpMirrorerStats(std::istringstream& commandStream)
{
   // protocol: no arguments

   App* app = Program::getApp();
   MetadataMirrorer* mirrorer = app->getMetadataMirrorer();

   std::ostringstream responseStream;
   size_t numNodes;
   size_t numTasksQueued;
   size_t numTasksInProgress;
   size_t numTaskMsgsInProgress;

   mirrorer->getStats(&numNodes, &numTasksQueued, &numTasksInProgress, &numTaskMsgsInProgress);

   responseStream << "numNodes: " << numNodes << std::endl;
   responseStream << "numTasksQueued: " << numTasksQueued << std::endl;
   responseStream << "numTasksInProgress: " << numTasksInProgress << std::endl;
   responseStream << "numTaskMsgsInProgress: " << numTaskMsgsInProgress << std::endl;

   return responseStream.str();
}

std::string GenericDebugMsgEx::processOpMsgQueueStats(std::istringstream& commandStream)
{
   // protocol: no arguments

   App* app = Program::getApp();
   MultiWorkQueue* workQ = app->getWorkQueue();

   std::ostringstream responseStream;
   std::string indirectQueueStats;
   std::string directQueueStats;
   std::string mirrorQueueStats;
   std::string busyStats;

   workQ->getStatsAsStr(indirectQueueStats, directQueueStats, mirrorQueueStats, busyStats);

   responseStream << "general queue stats: " << std::endl <<
      indirectQueueStats << std::endl;

   responseStream << "direct queue stats: " << std::endl <<
      directQueueStats << std::endl;

   responseStream << "mirror queue stats: " << std::endl <<
      mirrorQueueStats << std::endl;

   responseStream << "busy worker stats: " << std::endl <<
      busyStats << std::endl;

   return responseStream.str();
}

/**
 * List internal state of meta and storage capacity pools.
 */
std::string GenericDebugMsgEx::processOpListPools(std::istringstream& commandStream)
{
   // protocol: no arguments

   App* app = Program::getApp();
   NodeCapacityPools* metaPools = app->getMetaCapacityPools();
   TargetCapacityPools* storagePools = app->getStorageCapacityPools();
   NodeCapacityPools* storageBuddyPools = app->getStorageBuddyCapacityPools();

   std::ostringstream responseStream;

   {
      std::string metaPoolsState;

      metaPools->getStateAsStr(metaPoolsState);

      responseStream << "META POOLS STATE: " << std::endl <<
         metaPoolsState << std::endl;
   }

   {
      std::string storagePoolsState;

      storagePools->getStateAsStr(storagePoolsState);

      responseStream << "STORAGE POOLS STATE: " << std::endl <<
         storagePoolsState << std::endl;
   }

   {
      std::string storagePoolsState;

      storageBuddyPools->getStateAsStr(storagePoolsState);

      responseStream << "STORAGE BUDDY POOLS STATE: " << std::endl <<
         storagePoolsState << std::endl;
   }

   return responseStream.str();
}

std::string GenericDebugMsgEx::processOpDumpDentry(std::istringstream& commandStream)
{
   MetaStore* metaStore = Program::getApp()->getMetaStore();

   std::ostringstream responseStream;

   StringList parameterList;
   StringTk::explode(commandStream.str(), ' ', &parameterList);

   if ( parameterList.size() != 3 )
      return "Invalid or missing parameters; Parameter format: parentDirID entryName";

   StringListIter iter = parameterList.begin();
   iter++;
   std::string parentDirID = *iter;
   iter++;
   std::string entryName = *iter;

   DirInode* parentDirInode = metaStore->referenceDir(parentDirID, false);

   if (!parentDirInode)
   return "Unable to reference parent directory.";

   DirEntry dentry(entryName);
   bool getDentryRes = parentDirInode->getDentry(entryName, dentry);

   metaStore->releaseDir(parentDirID);

   if (!getDentryRes)
   return "Unable to get dentry from parent directory.";

   responseStream << "entryType: " << dentry.getEntryType() << std::endl;
   responseStream << "ID: " << dentry.getID() << std::endl;
   responseStream << "ownerNodeID: " << dentry.getOwnerNodeID() << std::endl;
   responseStream << "featureFlags: " << dentry.getDentryFeatureFlags() << std::endl;

   return responseStream.str();
}

#ifdef BEEGFS_DEBUG
std::string GenericDebugMsgEx::processOpWriteDirDentry(std::istringstream& commandStream)
{
   MetaStore* metaStore = Program::getApp()->getMetaStore();
   std::string dentriesPath = Program::getApp()->getDentriesPath()->getPathAsStrConst();

   std::ostringstream responseStream;

   StringList parameterList;
   StringTk::explode(commandStream.str(), ' ', &parameterList);

   if ( parameterList.size() != 4 )
      return "Invalid or missing parameters; Parameter format: parentDirID entryName ownerNodeID";

   StringListIter iter = parameterList.begin();
   iter++;
   std::string parentDirID = *iter;
   iter++;
   std::string entryName = *iter;
   iter++;
   uint16_t ownerNodeID = StringTk::strToUInt(*iter);

   DirInode* parentDirInode = metaStore->referenceDir(parentDirID, true);

   if (!parentDirInode)
      return "Unable to reference parent directory.";

   DirEntry dentry(entryName);
   bool getDentryRes = parentDirInode->getDentry(entryName, dentry);

   metaStore->releaseDir(parentDirID);

   if (!getDentryRes)
      return "Unable to get dentry from parent directory.";

   bool setOwnerRes = dentry.setOwnerNodeID(
      MetaStorageTk::getMetaDirEntryPath(dentriesPath, parentDirID), ownerNodeID);

   if (!setOwnerRes)
      return "Unable to set new owner node ID in dentry.";

   return "OK";
}
#endif // BEEGFS_DEBUG

std::string GenericDebugMsgEx::processOpDumpInode(std::istringstream& commandStream)
{
   // commandStream: ID of inode

   MetaStore* metaStore = Program::getApp()->getMetaStore();

   std::ostringstream responseStream;

   std::string inodeID;

   // get inodeID from command string
   std::getline(commandStream, inodeID, ' ');

   if(inodeID.empty() )
      return "Invalid or missing inode ID";

   FileInode* fileInode = NULL;
   DirInode* dirInode = NULL;

   metaStore->referenceInode(inodeID, &fileInode, &dirInode);

   if (fileInode)
   {
      StatData statData;
      if (fileInode->getStatData(statData) != FhgfsOpsErr_SUCCESS)
      { // stat data retrieval failed
         metaStore->releaseFile("", fileInode);
         return "Could not get stat data for requested file inode";
      }

      DirEntryType dirEntryType = MetadataTk::posixFileTypeToDirEntryType(fileInode->getMode() );

      std::string parentDirID = "cannotBeUsed";
      uint16_t parentNodeID   = 0;

      responseStream << "entryType: " << dirEntryType << std::endl;
      responseStream << "parentEntryID: " <<  parentDirID << std::endl;
      responseStream << "parentNodeID: " << StringTk::uintToStr(parentNodeID) << std::endl;
      responseStream << "mode: " << StringTk::intToStr(statData.getMode()) << std::endl;
      responseStream << "uid: " << StringTk::uintToStr(statData.getUserID()) << std::endl;
      responseStream << "gid: " << StringTk::uintToStr(statData.getGroupID()) << std::endl;
      responseStream << "filesize: " << StringTk::int64ToStr(statData.getFileSize()) << std::endl;
      responseStream << "ctime: " << StringTk::int64ToStr(statData.getCreationTimeSecs()) << std::endl;
      responseStream << "atime: " << StringTk::int64ToStr(statData.getLastAccessTimeSecs()) << std::endl;
      responseStream << "mtime: " << StringTk::int64ToStr(statData.getModificationTimeSecs()) << std::endl;
      responseStream << "hardlinks: " << StringTk::intToStr(statData.getNumHardlinks()) << std::endl;
      responseStream << "stripeTargets: "
         << StringTk::uint16VecToStr(fileInode->getStripePattern()->getStripeTargetIDs())
         << std::endl;
      responseStream << "chunkSize: "
         << StringTk::uintToStr(fileInode->getStripePattern()->getChunkSize()) << std::endl;
      responseStream << "featureFlags: " << fileInode->getFeatureFlags() << std::endl;

      metaStore->releaseFile("", fileInode);
   }
   else
   if (dirInode)
   {
      StatData statData;
      if (dirInode->getStatData(statData) != FhgfsOpsErr_SUCCESS)
      { // stat data retrieval failed
         metaStore->releaseDir(inodeID);
         return "Could not get stat data for requested dir inode";
      }

      DirEntryType dirEntryType = MetadataTk::posixFileTypeToDirEntryType(dirInode->getMode() );

      std::string parentDirID;
      uint16_t parentNodeID;
      dirInode->getParentInfo(&parentDirID, &parentNodeID);

         responseStream << "entryType: " << dirEntryType << std::endl;
         responseStream << "parentEntryID: " << parentDirID << std::endl;
         responseStream << "parentNodeID: " << StringTk::uintToStr(parentNodeID) << std::endl;
         responseStream << "ownerNodeID: " << StringTk::uintToStr(dirInode->getOwnerNodeID())
            << std::endl;
         responseStream << "mode: " << StringTk::intToStr(statData.getMode()) << std::endl;
         responseStream << "uid: " << StringTk::uintToStr(statData.getUserID()) << std::endl;
         responseStream << "gid: " << StringTk::uintToStr(statData.getGroupID()) << std::endl;
         responseStream << "size: " << StringTk::int64ToStr(statData.getFileSize()) << std::endl;
         responseStream << "numLinks: " << StringTk::int64ToStr(statData.getNumHardlinks()) << std::endl;
         responseStream << "ctime: " << StringTk::int64ToStr(statData.getCreationTimeSecs())
            << std::endl;
         responseStream << "atime: " << StringTk::int64ToStr(statData.getLastAccessTimeSecs())
            << std::endl;
         responseStream << "mtime: " << StringTk::int64ToStr(statData.getModificationTimeSecs())
            << std::endl;
         responseStream << "featureFlags: " << dirInode->getFeatureFlags() << std::endl;

      metaStore->releaseDir(inodeID);
   }
   else
   {
      return "Could not read requested inode";
   }

   return responseStream.str();
}

std::string GenericDebugMsgEx::processOpDumpInlinedInode(std::istringstream& commandStream)
{
   // commandStream: parentID, name

   MetaStore* metaStore = Program::getApp()->getMetaStore();
   uint16_t localNodeID = Program::getApp()->getLocalNode()->getNumID();
   std::ostringstream responseStream;

   StringList parameterList;
   StringTk::explode(commandStream.str(), ' ', &parameterList);

   if ( parameterList.size() != 3 )
      return "Invalid or missing parameters; Parameter format: parentDirID entryName";

   StringListIter iter = parameterList.begin();
   iter++;
   std::string parentEntryID = *iter;
   iter++;
   std::string entryName = *iter;

   EntryInfo entryInfo(localNodeID, parentEntryID, "unknown", entryName, DirEntryType_REGULARFILE,
      0);

   DirInode* parentInode = metaStore->referenceDir(parentEntryID, false);

   if ( !parentInode )
      return "Could not open parent directory";

   DirEntry dirEntry(entryName);
   bool getDentryRes = parentInode->getDentry(entryName, dirEntry);

   if ( !getDentryRes )
   {
      metaStore->releaseDir(parentEntryID);
      return "Could not open dir entry";
   }

   FileInodeStoreData* inodeData = dirEntry.getInodeStoreData();

   if ( !inodeData )
   {
      metaStore->releaseDir(parentEntryID);
      return "Could not get inlined inode data";
   }

   StatData* statData = inodeData->getInodeStatData();
   if ( !statData )
   {
      metaStore->releaseDir(parentEntryID);
      return "Could not get stat data for requested file inode";
   }

   responseStream << "entryID: " << inodeData->getEntryID() << std::endl;
   responseStream << "mode: " << StringTk::intToStr(statData->getMode()) << std::endl;
   responseStream << "uid: " << StringTk::uintToStr(statData->getUserID()) << std::endl;
   responseStream << "gid: " << StringTk::uintToStr(statData->getGroupID()) << std::endl;
   responseStream << "filesize: " << StringTk::int64ToStr(statData->getFileSize()) << std::endl;
   responseStream << "ctime: " << StringTk::int64ToStr(statData->getCreationTimeSecs()) << std::endl;
   responseStream << "atime: " << StringTk::int64ToStr(statData->getLastAccessTimeSecs())
      << std::endl;
   responseStream << "mtime: " << StringTk::int64ToStr(statData->getModificationTimeSecs())
      << std::endl;
   responseStream << "hardlinks: " << StringTk::intToStr(statData->getNumHardlinks()) << std::endl;
   responseStream << "stripeTargets: "
      << StringTk::uint16VecToStr(inodeData->getPattern()->getStripeTargetIDs()) << std::endl;
   responseStream << "chunkSize: "
      << StringTk::uintToStr(inodeData->getPattern()->getChunkSize()) << std::endl;
   responseStream << "featureFlags: " << inodeData->getInodeFeatureFlags() << std::endl;

   metaStore->releaseDir(parentEntryID);

   return responseStream.str();
}

std::string GenericDebugMsgEx::processOpQuotaExceeded(std::istringstream& commandStream)
{
   App* app = Program::getApp();

   std::string returnString;

   if(!app->getConfig()->getQuotaEnableEnforcement() )
      return "No quota exceeded IDs on this storage daemon because quota enforcement is"
         "disabled.";

   returnString = MsgHelperGenericDebug::processOpQuotaExceeded(commandStream,
      app->getExceededQuotaStore() );

   return returnString;
}

#ifdef BEEGFS_DEBUG
std::string GenericDebugMsgEx::processOpWriteDirInode(std::istringstream& commandStream)
{
   MetaStore* metaStore = Program::getApp()->getMetaStore();

   // get parameters from command string
   StringVector paramVec;
   StringTk::explode(commandStream.str(), ' ', &paramVec);

   std::string entryID;
   std::string parentDirID;
   uint16_t parentNodeID;
   uint16_t ownerNodeID;
   int mode;
   uint uid;
   uint gid;
   int64_t size;
   unsigned numLinks;

   try
   {
      unsigned i = 1;
      entryID = paramVec.at(i++);
      parentDirID = paramVec.at(i++);
      parentNodeID = StringTk::strToUInt(paramVec.at(i++));
      ownerNodeID = StringTk::strToUInt(paramVec.at(i++));
      mode = StringTk::strToInt(paramVec.at(i++));
      uid = StringTk::strToUInt(paramVec.at(i++));
      gid = StringTk::strToUInt(paramVec.at(i++));
      size = StringTk::strToInt64(paramVec.at(i++));
      numLinks = StringTk::strToUInt(paramVec.at(i++));
   }
   catch (std::out_of_range& e)
   {
      std::string paramFormatStr =
         "entryID parentDirID parentNodeID ownerNodeID mode uid gid size numLinks";
      return "Invalid or missing parameters; Parameter format: " + paramFormatStr;
   }

   DirInode* dirInode = metaStore->referenceDir(entryID, true);

   if ( !dirInode )
      return "Could not find directory with ID: " + entryID;

   StatData statData;
   if ( dirInode->getStatData(statData) != FhgfsOpsErr_SUCCESS )
   {
      metaStore->releaseDir(entryID);
      return "Could not get stat data for requested dir inode";
   }

   dirInode->setParentInfoInitial(parentDirID, parentNodeID);
   dirInode->setOwnerNodeID(ownerNodeID);

   statData.setMode(mode);
   statData.setUserID(uid);
   statData.setGroupID(gid);
   statData.setFileSize(size);
   statData.setNumHardLinks(numLinks);

   dirInode->setStatData(statData);

   metaStore->releaseDir(entryID);

   return "OK";
}


std::string GenericDebugMsgEx::processOpWriteInlinedFileInode(std::istringstream& commandStream)
{
   MetaStore* metaStore = Program::getApp()->getMetaStore();
   std::string retStr = "OK";

   // get parameters from command string
   StringVector paramVec;
   StringTk::explode(commandStream.str(), ' ', &paramVec);

   std::string parentDirID;
   std::string name;
   std::string entryID;
   int mode;
   uint uid;
   uint gid;
   int64_t filesize;
   unsigned numLinks;
   UInt16Vector stripeTargets;
 //  UInt16Vector* origStripeTargets;

   try
   {
      unsigned i = 1;
      parentDirID = paramVec.at(i++);
      name = paramVec.at(i++);
      entryID = paramVec.at(i++);
      mode = StringTk::strToInt(paramVec.at(i++));
      uid = StringTk::strToUInt(paramVec.at(i++));
      gid = StringTk::strToUInt(paramVec.at(i++));
      filesize = StringTk::strToInt64(paramVec.at(i++));
      numLinks = StringTk::strToUInt(paramVec.at(i++));
      StringTk::strToUint16Vec(paramVec.at(i++), &stripeTargets);
   }
   catch (std::out_of_range& e)
   {
      std::string paramFormatStr =
         "parentDirID entryName entryID mode uid gid filesize numLinks stripeTargets";
      return "Invalid or missing parameters; Parameter format: " + paramFormatStr;
   }

   EntryInfo entryInfo(Program::getApp()->getLocalNodeNumID(), parentDirID, entryID, name,
      DirEntryType_REGULARFILE, 0);

   FileInode* fileInode = metaStore->referenceFile(&entryInfo);

   if (!fileInode)
      return "Could not reference inode";

   StatData statData;
   fileInode->getStatData(statData);

   statData.setMode(mode);
   statData.setUserID(uid);
   statData.setGroupID(gid);
   statData.setFileSize(filesize);
   statData.setNumHardLinks(numLinks);

   fileInode->setStatData(statData);

   StripePattern* pattern = fileInode->getStripePattern();
   UInt16Vector* origTargets = pattern->getStripeTargetIDsModifyable();
   *origTargets = stripeTargets;

   fileInode->updateInodeOnDisk(&entryInfo, pattern);

   metaStore->releaseFile(parentDirID, fileInode);

   return retStr;
}
#endif // BEEGFS_DEBUG

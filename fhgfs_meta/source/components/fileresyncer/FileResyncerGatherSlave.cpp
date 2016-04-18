#include <app/App.h>
#include <app/config/Config.h>
#include <common/toolkit/Time.h>
#include <common/storage/EntryInfo.h>
#include <net/msghelpers/MsgHelperStat.h>
#include <program/Program.h>
#include <storage/MetadataEx.h>
#include "FileResyncer.h"


#define FILERESYNCGATHER_DENTRY_DEPTH  4 /* dentry depth relative to "<storageDir>/dentries" dir */
#define FILERESYNCGATHER_INODE_DEPTH   3 /* inode depth relative to "<storageDir>/inodes" dir */


FileResyncerGatherSlave* FileResyncerGatherSlave::thisStatic = NULL;


FileResyncerGatherSlave::FileResyncerGatherSlave() throw(ComponentInitException) :
   PThread("SyncGather")
{
   log.setContext("ResyncGather");

   this->isRunning = false;

   this->thisStatic = this;
}

FileResyncerGatherSlave::~FileResyncerGatherSlave()
{
   this->thisStatic = NULL; // (not necessary, but just to be careful...)
}


/**
 * This is a singleton component, which is started through its control frontend on-demand at
 * runtime and terminates when it's done.
 * We have to ensure (in cooperation with the control frontend) that we don't get multiple instances
 * of this thread running at the same time.
 */
void FileResyncerGatherSlave::run()
{
   setIsRunning(true);

   try
   {
      registerSignalHandler();

      walkAllMetadata();

      log.log(Log_DEBUG, "Component stopped.");
   }
   catch(std::exception& e)
   {
      PThread::getCurrentThreadApp()->handleComponentException(e);
   }

   setIsRunning(false);
}

/**
 * Walk over all metadata to identify resync candidates.
 */
void FileResyncerGatherSlave::walkAllMetadata()
{
   const unsigned maxOpenFDsNum = 20; // max open FDs => max path sub-depth for efficient traversal

   App* app = Program::getApp();


   // (note: member stats counters are set to 0 in refresher master before running this thread.)

   // walk over all dentries to get inlined inodes

   log.log(Log_WARNING, "Starting dentries walk...");

   std::string dentriesPath = app->getMetaPath() + "/" + META_DENTRIES_SUBDIR_NAME;

   int nftwDentriesRes = nftw(dentriesPath.c_str(),
      handleDiscoveredDentriesSubentry, maxOpenFDsNum, FTW_ACTIONRETVAL);
   if(nftwDentriesRes == -1)
   { // error occurred
      log.logErr("Error during dentries walk. SysErr: " + System::getErrString() );
   }


   // walk over all non-inlined inodes

   log.log(Log_WARNING, "Starting inodes walk...");

   std::string inodesPath = app->getMetaPath() + "/" + META_INODES_SUBDIR_NAME;

   int nftwInodesRes = nftw(inodesPath.c_str(),
      handleDiscoveredInodesSubentry, maxOpenFDsNum, FTW_ACTIONRETVAL);
   if(nftwInodesRes == -1)
   { // error occurred
      log.logErr("Error during inodes walk. SysErr: " + System::getErrString() );
   }


   log.log(Log_WARNING, "End of metadata walk.");
}

/**
 * This is the static nftw() callback, which is called for every file/subdir within
 * "<storageDir>/dentries".
 *
 * Note: This method gets access to the FileResyncerGatherSlave instance through the "thisStatic"
 * member.
 *
 * @param path full path to file
 * @param statBuf normal struct stat contents (as in "man stat(2)")
 * @param ftwEntryType FTW_... (e.g. FTW_F for files)
 * @param ftwBuf ftwBuf->level is depth relative to nftw search path; ftwBuf->base is index in
 * path where filename starts (typically used as "path+ftwBuf->base").
 */
int FileResyncerGatherSlave::handleDiscoveredDentriesSubentry(const char* path,
   const struct stat* statBuf, int ftwEntryType, struct FTW* ftwBuf)
{
   const char* logContext = "FileResyncerGatherSlave (handle dentries subentry)";

   const char* entryName = path + ftwBuf->base;

   // skip too deep dirs (e.g. fsids dirs)

   if( (ftwEntryType == FTW_D) && (ftwBuf->level >= FILERESYNCGATHER_DENTRY_DEPTH) )
   { // directory too deep to be interesting for us
      LogContext(logContext).log(Log_SPAM, std::string("Skipping subtree: ") + path);

      return FTW_SKIP_SUBTREE;
   }

   // continue to next entry if this is not a file at the right depth

   if( (ftwEntryType != FTW_F) || (ftwBuf->level != FILERESYNCGATHER_DENTRY_DEPTH) )
   { // not a file or a file at the wrong depth level
      if(ftwEntryType != FTW_D) // don't spam log with all hash dirs (only print other entry types)
         LogContext(logContext).log(
            Log_SPAM, std::string("Skipping entry: ") + path + "; "
            "type: " + getFtwEntryTypeStr(ftwEntryType) );

      return FTW_CONTINUE;
   }

   // we found a dentry file at the right depth...

   App* app = Program::getApp();
   MetaStore *metaStore = app->getMetaStore();

   thisStatic->numEntriesDiscovered.increase();

   // reference parent dir (and find out whether this dentry refers to a file or a dir)

   std::string parentPath = StorageTk::getPathDirname(path);
   std::string parentDirID = StorageTk::getPathBasename(parentPath); // contents dir name equals ID

   DirInode* dirInode = metaStore->referenceDir(parentDirID, true);
   if(!dirInode)
   { // appearently, the dir has just been removed by user => ignore
      LogContext(logContext).log(Log_SPAM,
         std::string("Contents dir vanished before referencing: ") + path);

      return FTW_CONTINUE;
   }

   // got the parent dir inode, now get the entryInfo

   UInt16Set* resyncMirrorTargets = &thisStatic->resyncMirrorTargets;

   EntryInfo fileEntryInfo;
   FileInode* fileInode;
   StripePattern* pattern;
   bool gotMatch;

   bool getFileDentryRes = dirInode->getFileEntryInfo(entryName, fileEntryInfo);
   if(!getFileDentryRes)
   { // ether not a file or it was just moved/deleted
      LogContext(logContext).log(Log_SPAM,
         std::string("Discovered dentry not a file or vanished before it could be read: ") + path);

      goto release_parent;
   }

   // check if this is an inlined inode (non-inlined inodes will be handled in separate walk)

   if(! (fileEntryInfo.getIsInlined() ) )
      goto release_parent;

   // dentry refers to a file => try to reference it

   fileInode = metaStore->referenceFile(&fileEntryInfo);
   if(!fileInode)
   { // file was just moved/deleted
      LogContext(logContext).log(Log_SPAM,
         std::string("Discovered file vanished before it could be referenced: ") + path);

      goto release_parent;
   }

   // got the file reference => check whether it matches our search pattern

   pattern = fileInode->getStripePattern();

   gotMatch = testTargetMatch(resyncMirrorTargets, pattern);
   if(!gotMatch)
      goto release_file;

   thisStatic->numFilesMatched.increase();

   // TODO: this file is a sync candidate


   // clean up

release_file:
   metaStore->releaseFile(fileEntryInfo.getParentEntryID(), fileInode);

release_parent:
   metaStore->releaseDir(parentDirID);


   return FTW_CONTINUE;
}

/**
 * This is the static nftw() callback, which is called for every file/subdir within
 * "<storageDir>/inodes".
 *
 * Note: This method gets access to the FileResyncerGatherSlave instance through the "thisStatic"
 * member.
 *
 * @param path full path to file
 * @param statBuf normal struct stat contents (as in "man stat(2)")
 * @param ftwEntryType FTW_... (e.g. FTW_F for files)
 * @param ftwBuf ftwBuf->level is depth relative to nftw search path; ftwBuf->base is index in
 * path where filename starts (typically used as "path+ftwBuf->base").
 */
int FileResyncerGatherSlave::handleDiscoveredInodesSubentry(const char* path,
   const struct stat* statBuf, int ftwEntryType, struct FTW* ftwBuf)
{
   const char* logContext = "FileResyncerGatherSlave (handle inodes subentry)";

   const char* entryName = path + ftwBuf->base;

   // skip too deep dirs (though there actually shouldn't be any)

   if( (ftwEntryType == FTW_D) && (ftwBuf->level >= FILERESYNCGATHER_DENTRY_DEPTH) )
   { // directory too deep to be interesting for us
      LogContext(logContext).log(Log_SPAM, std::string("Skipping subtree: ") + path);

      return FTW_SKIP_SUBTREE;
   }

   // continue to next entry if this is not a file at the right depth

   if( (ftwEntryType != FTW_F) || (ftwBuf->level != FILERESYNCGATHER_DENTRY_DEPTH) )
   { // not a file or a file at the wrong depth level
      if(ftwEntryType != FTW_D) // don't spam log with all hash dirs (only print other entry types)
         LogContext(logContext).log(
            Log_SPAM, std::string("Skipping entry: ") + path + "; "
            "type: " + getFtwEntryTypeStr(ftwEntryType) );

      return FTW_CONTINUE;
   }

   // we found an inode file at the right depth...

   App* app = Program::getApp();
   MetaStore *metaStore = app->getMetaStore();

   thisStatic->numEntriesDiscovered.increase();

   // find out whether it is a file inode

   DirInode* dirInode = NULL;
   FileInode* fileInode = NULL;

   bool referenceRes = metaStore->referenceInode(entryName, &fileInode, &dirInode);
   if(!referenceRes)
   {
      LogContext(logContext).log(Log_DEBUG, std::string("Inode loading failed: ") + path);

      return FTW_CONTINUE;
   }

   if(dirInode)
   { // skip dir inodes
      LogContext(logContext).log(Log_SPAM,
         std::string("Skipping discovered dir inode: ") + path);

      metaStore->releaseDir(entryName);
      return FTW_CONTINUE;
   }

   if(unlikely(!fileInode) )
      return FTW_CONTINUE; // (should actually never happen, because referenceRes==true here)


   // file inode => check if it matches our search criteria

   StripePattern* pattern = fileInode->getStripePattern();

   bool gotMatch = testTargetMatch(&thisStatic->resyncMirrorTargets, pattern);
   if(!gotMatch)
      goto release_file;

   thisStatic->numFilesMatched.increase();

   // TODO: this file is a sync candidate


release_file:
   // (note: parentID not relevant for non-inlined inodes)
   metaStore->releaseFile("RESYNCER_NO_PARENTID", fileInode);

   return FTW_CONTINUE;
}

/**
 * Test whether any of the mirror targets in pattern is contained in given resyncMirrorTargets.
 *
 * Empty resyncMirrorTargets is a special case that always matches if pattern refers to a mirrored
 * file.
 *
 * @return true if match found
 */
bool FileResyncerGatherSlave::testTargetMatch(UInt16Set* resyncMirrorTargets,
   StripePattern* pattern)
{
   const UInt16Vector* mirrorTargetIDs = pattern->getMirrorTargetIDs();

   if(!mirrorTargetIDs)
      return false; // not a mirrored file => no match

   if(resyncMirrorTargets->empty() )
      return true; // empty means "all" => match

   // check for each pattern mirror target if it is contained in resyncMirrorTargets

   for(UInt16VectorConstIter iter = mirrorTargetIDs->begin();
       iter != mirrorTargetIDs->end();
       iter++)
   {
      if(resyncMirrorTargets->find(*iter) != resyncMirrorTargets->end() )
         return true; // found a match
   }

   return false;
}

/**
 * @param ftwEntryType FTW_... (e.g. FTW_D)
 */
std::string FileResyncerGatherSlave::getFtwEntryTypeStr(int ftwEntryType)
{
   switch(ftwEntryType)
   {
      case FTW_F: // file
         return "file";

      case FTW_D: // directory
         return "directory";

      case FTW_DP: // completely processed directory
         return "completed directory";

      case FTW_SL: // (unfollowed) symlink
         return "symlink";

      case FTW_DNR: // directory not readable
         return "unreadable directory";

      case FTW_NS: // stat failed on non-symlink
         return "unstatable entry";

      case FTW_SLN: // dangling symlink
         return "dangling symlink";
         break;


      default:
         return "<unknown(" + StringTk::uintToStr(ftwEntryType) + ")>";
   }

}

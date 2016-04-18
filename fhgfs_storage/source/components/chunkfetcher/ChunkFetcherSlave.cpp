#include "ChunkFetcherSlave.h"

#include <program/Program.h>

#include <libgen.h>

Mutex ChunkFetcherSlave::staticMapsMutex;
std::map<std::string, uint16_t> ChunkFetcherSlave::currentTargetIDs;
std::map<std::string, uint16_t> ChunkFetcherSlave::currentBuddyGroupIDs;
std::map<std::string, size_t> ChunkFetcherSlave::basePathLenghts;

ChunkFetcherSlave::ChunkFetcherSlave(uint16_t targetID) throw(ComponentInitException):
   PThread("ChunkFetcherSlave-" + StringTk::uintToStr(targetID))
{
   log.setContext(this->getName());

   this->parent = Program::getApp()->getChunkFetcher();

   this->isRunning = false;

   this->targetID = targetID;

   SafeMutexLock staticMapsLock(&staticMapsMutex);

   currentTargetIDs[this->getName()] = this->targetID;
   basePathLenghts[this->getName()] = 0;

   staticMapsLock.unlock();
}

ChunkFetcherSlave::~ChunkFetcherSlave()
{
}

void ChunkFetcherSlave::run()
{
   setIsRunning(true);

   try
   {
      registerSignalHandler();

      walkAllChunks();

      log.log(4, "Component stopped.");
   }
   catch(std::exception& e)
   {
      PThread::getCurrentThreadApp()->handleComponentException(e);
   }

   setIsRunning(false);
}

/*
 * walk over all chunks in that target
 */
void ChunkFetcherSlave::walkAllChunks()
{
   const unsigned maxOpenFDsNum = 20; // max open FDs

   App* app = Program::getApp();

   log.log(Log_DEBUG, "Starting chunks walk...");

   std::string targetPath;
   app->getStorageTargets()->getPath(this->targetID, &targetPath);

   // walk over "normal" chunks (i.e. no mirrors)
   std::string walkPath = targetPath + "/" + CONFIG_CHUNK_SUBDIR_NAME;

   SafeMutexLock staticMapsLock(&staticMapsMutex);

   currentBuddyGroupIDs[PThread::getCurrentThreadName()] = 0;
   basePathLenghts[PThread::getCurrentThreadName()] = strlen(walkPath.c_str());

   staticMapsLock.unlock();

   int nftwTargetRes = nftw(walkPath.c_str(), handleDiscoveredChunk, maxOpenFDsNum,
      FTW_ACTIONRETVAL);
   if(nftwTargetRes == -1)
   { // error occurred
      log.logErr("Error during chunks walk. SysErr: " + System::getErrString());
   }

   // let's find out if this target is part of a buddy mirror group and if it is the primary
   // target; if it is, walk over buddy mirror directory
   bool isPrimaryTarget;
   uint16_t buddyGroupID = app->getMirrorBuddyGroupMapper()->getBuddyGroupID(this->targetID,
      &isPrimaryTarget);

   if (isPrimaryTarget)
   {
      walkPath = targetPath + "/" CONFIG_BUDDYMIRROR_SUBDIR_NAME;

      SafeMutexLock staticMapsLock(&staticMapsMutex);

      currentBuddyGroupIDs[PThread::getCurrentThreadName()] = buddyGroupID;
      basePathLenghts[PThread::getCurrentThreadName()] = strlen(walkPath.c_str());

      staticMapsLock.unlock();

      int nftwTargetRes = nftw(walkPath.c_str(), handleDiscoveredChunk, maxOpenFDsNum,
         FTW_ACTIONRETVAL);
      if(nftwTargetRes == -1)
      { // error occurred
         log.logErr("Error during buddy-mirror chunks walk. SysErr: " + System::getErrString());
      }
   }

   log.log(Log_DEBUG, "End of chunks walk.");
}

/**
 * This is the static nftw() callback, which is called for every chunk found on that target
 *
 * Note: This method gets access to the ChunkFetcherSlave instance through the "thisStatic"
 * member.
 *
 * @param path full path to file
 * @param statBuf normal struct stat contents (as in "man stat(2)")
 * @param ftwEntryType FTW_... (e.g. FTW_F for files)
 * @param ftwBuf ftwBuf->level is depth relative to nftw search path; ftwBuf->base is index in
 * path where filename starts (typically used as "path+ftwBuf->base").
 */
int ChunkFetcherSlave::handleDiscoveredChunk(const char* path, const struct stat* statBuf,
   int ftwEntryType, struct FTW* ftwBuf)
{
   if (ftwEntryType != FTW_F) // not a file, skip
      return FTW_CONTINUE;

   const char* entryName = path + ftwBuf->base;

   // strip the path of the target itself (+ the slash), so we have a relative path
   SafeMutexLock staticMapsLock(&staticMapsMutex);

   size_t basePathLen = basePathLenghts[PThread::getCurrentThreadName()];
   uint16_t targetID = currentTargetIDs[PThread::getCurrentThreadName()];
   uint16_t buddyGroupID = currentBuddyGroupIDs[PThread::getCurrentThreadName()];

   staticMapsLock.unlock();

   // get only the dirname part of the path
   char* tmpPathCopy = strdup(path);
   Path savedPath(
      std::string(dirname(tmpPathCopy + basePathLen + 1)));

   free(tmpPathCopy);

   int64_t fileSize = (int64_t)statBuf->st_size;
   uint64_t usedBlocks = (uint64_t)statBuf->st_blocks;
   int64_t creationTime = (int64_t)statBuf->st_ctime;
   int64_t modificationTime = (int64_t)statBuf->st_mtime;
   int64_t lastAccessTime = (int64_t)statBuf->st_atime;
   unsigned userID = (unsigned)statBuf->st_uid;
   unsigned groupID = (unsigned)statBuf->st_gid;

   FsckChunk fsckChunk(entryName, targetID, savedPath, fileSize, usedBlocks, creationTime,
      modificationTime, lastAccessTime, userID, groupID, buddyGroupID);

   Program::getApp()->getChunkFetcher()->addChunk(fsckChunk);

   return FTW_CONTINUE;
}

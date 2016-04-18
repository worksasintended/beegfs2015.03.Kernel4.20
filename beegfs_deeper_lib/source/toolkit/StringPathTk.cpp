#include "StringPathTk.h"



/**
 * checks if a path point to a file/directory on the global BeeGFS, requires an absolute path
 *
 * @param path the path to test
 * @param deeperLibConfig the config of the beegfs deeper lib
 * @return true if the path point to a file/directory on the global BeeGFS, false if not
 */
bool isGlobalPath(std::string& path, Config* deeperLibConfig)
{
   std::string globalPath = deeperLibConfig->getSysMountPointGlobal();
   size_t startPos = path.find(globalPath);

   if(startPos == 0)
      return true;

   return false;
}

/**
 * checks if a path point to a file/directory on the cache BeeGFS, requires an absolute path
 *
 * @param path the path to test
 * @param deeperLibConfig the config of the beegfs deeper lib
 * @return true if the path point to a file/directory on the global BeeGFS, false if not
 */
bool isCachePath(std::string& path, Config* deeperLibConfig)
{
   std::string cachePath = deeperLibConfig->getSysMountPointCache();
   size_t startPos = path.find(cachePath);

   if(startPos == 0)
      return true;

   return false;
}

/**
 * replaces the mount point path of the global BeeGFS with the mount point path of the cache BeeGFS
 *
 * @param inOutPath the path to a file or directory on the global BeeGFS
 * @param deeperLibConfig the config of the beegfs deeper lib
 * @param log the logger to log error messages
 * @return true on success, false if the path is not pointing to the global BeeGFS
 */
bool CachePathTk::globalPathToCachePath(std::string& inOutPath, Config* deeperLibConfig,
   Logger* log)
{
#ifdef BEEGFS_DEBUG
      log->logErr(__FUNCTION__, "path: " + inOutPath);
#endif

   std::string cachePath = deeperLibConfig->getSysMountPointCache();
   std::string globalPath = deeperLibConfig->getSysMountPointGlobal();

   return replaceRootPath(inOutPath, globalPath, cachePath, log);
}

/**
 * replaces the mount point path of the cache BeeGFS with the mount point path of the global BeeGFS
 *
 * @param inOutPath the path to a file or directory on the cache BeeGFS
 * @param deeperLibConfig the config of the beegfs deeper lib
 * @param log the logger to log error messages
 * @return true on success, false if the path is not pointing to the cache BeeGFS
 */
bool CachePathTk::cachePathToGlobalPath(std::string& inOutPath, Config* deeperLibConfig,
   Logger* log)
{
#ifdef BEEGFS_DEBUG
      log->logErr(__FUNCTION__, "path: " + inOutPath);
#endif

   std::string cachePath = deeperLibConfig->getSysMountPointCache();
   std::string globalPath = deeperLibConfig->getSysMountPointGlobal();

   return replaceRootPath(inOutPath, cachePath, globalPath, log);
}

/**
 * replaces a part of the given path beginning from the root, it also converts a relative path to
 * an absolute path
 *
 * @param inOutPath the path to modify
 * @param origPath the expected root part of the path which needs to replaced
 * @param newPath the new root part of the path
 * @param log the logger to log error messages
 * @return true on success, false if the root path part is not matching to the expected root path
 *         part
 */
bool CachePathTk::replaceRootPath(std::string& inOutPath, std::string& origPath,
   std::string& newPath, Logger* log)
{
   if(inOutPath[0] != '/')
   {
      if(!makePathAbsolute(inOutPath, log) )
         return false;
   }

   if(origPath[origPath.length()-1] != '/')
      origPath += "/";
   if(newPath[newPath.length()-1] != '/')
      newPath += "/";

   size_t startPos = inOutPath.find(origPath);

   if(startPos == 0)
      inOutPath.replace(startPos, origPath.length(), newPath);
   else
   {
      log->logErr(__FUNCTION__, "The given path " + inOutPath + " is not located on the "
         "mount " + origPath);

      return false;
   }

#ifdef BEEGFS_DEBUG
      log->logErr(__FUNCTION__, "path: " + inOutPath);
#endif

   return true;
}

/**
 * modifies the given path from a relative path to an absolute path
 *
 * @param inOutPath the relative path, after this method the path is absolute
 * @param log the logger to log error messages
 * @return true on success, false on error, ernno is set
 */
bool CachePathTk::makePathAbsolute(std::string& inOutPath, Logger* log)
{
   bool retVal = true;
   char* realPath = (char*) malloc(PATH_MAX);

#ifdef BEEGFS_DEBUG
      log->logErr(__FUNCTION__, "in path: " + inOutPath);
#endif

   char *res = realpath(inOutPath.c_str(), realPath);
   if(res)
   {
      inOutPath.clear();
      inOutPath.insert(0, (const char *) realPath);

#ifdef BEEGFS_DEBUG
      log->logErr(__FUNCTION__, "out path: " + inOutPath);
#endif
   }
   else
   if(errno == ENOENT)
   {
      // remove trailing /
      if(inOutPath[inOutPath.size() -1] == '/')
         inOutPath.resize(inOutPath.size() -1);

      // remove last path element from path and store it for later use
      size_t indexLast = inOutPath.find_last_of("/");
      std::string removedPart(inOutPath.substr(indexLast + 1) );
      inOutPath.resize(indexLast + 1);

      // find a valid parent with recursive method calls
      if(makePathAbsolute(inOutPath, log) )
         inOutPath.append("/" + removedPart); // rebuild path after recursive method calls
      else
         log->logErr(__FUNCTION__, "Couldn't convert given path to an absolute path. "
            "Given path: " + inOutPath + " (" + System::getErrString(errno) + ")");
   }
   else
   {
      log->logErr(__FUNCTION__, "Couldn't convert given path to an absolute path. "
         "Given path: " + inOutPath + " (" + System::getErrString(errno) + ")");
      retVal = false;
   }

   SAFE_FREE(realPath);

   return retVal;
}

/**
 * Create path directories. The owner and the mode is used from a source directory.
 *
 * Note: Existing directories are ignored (i.e. not an error).
 *
 * @param destPath the path to create
 * @param sourceMount the mount point of the source path to clone (keep mode and owner)
 * @param destMount the mount point of the path to create
 * @param excluseLastElement true if the last element should not be created.
 * @param log the logger to log error messages
 * @return true on success, false otherwise and errno is set from mkdir() in this case.
 */
bool CachePathTk::createPath(std::string destPath, std::string sourceMount,
   std::string destMount, bool excludeLastElement, Logger* log)
{
#ifdef BEEGFS_DEBUG
      log->logErr(__FUNCTION__, "path: " + destPath);
#endif

   struct stat sourceStat;

   if(destMount[destMount.size() -1] != '/')
      destMount += "/";
   if(sourceMount[sourceMount.size() -1] != '/')
      sourceMount += "/";

   // remove mount points from the path to get a relative path from mount
   destPath.erase(0, destMount.size() );

   StringList pathElems;
   StringTk::explode(destPath,'/', &pathElems);

   int mkdirErrno = 0;

   for(StringListConstIter iter = pathElems.begin(); iter != pathElems.end(); iter++)
   {
      sourceMount += *iter;
      destMount += *iter;

      int statRes = stat(sourceMount.c_str(), &sourceStat);
      if(statRes)
      {
         log->logErr(__FUNCTION__, "Couldn't stat source directory: " + sourceMount +
            " (" + System::getErrString(errno) + ")");
         return false;
      }

      int mkRes = mkdir(destMount.c_str(), sourceStat.st_mode);
      mkdirErrno = errno;
      if(mkRes && (mkdirErrno == EEXIST) )
      { // path exists already => check whether it is a directory
         struct stat statStruct;
         int statRes = stat(destMount.c_str(), &statStruct);
         if(statRes || !S_ISDIR(statStruct.st_mode) )
         {
            log->logErr(__FUNCTION__, "A element of the path is not a directory: " +
               destMount + " (" + System::getErrString(errno) + ")");
            return false;
         }

      }
      else
      if(mkRes)
      {
         log->logErr(__FUNCTION__, "Couldn't not create path: " + destMount +
            " (" + System::getErrString(errno) + ")");
         return false;
      }

      int chownRes = chown(destMount.c_str(), sourceStat.st_uid, sourceStat.st_gid);
      if(chownRes && (errno != EPERM) )
      { // EPERM happens when the process is not running as root, this can be ignored
         log->logErr(__FUNCTION__, "Couldn't not change owner of directory: " +
            destMount + " (" + System::getErrString(errno) + ")");
         return false;
      }

      // prepare next loop
      sourceMount += "/";
      destMount += "/";
   }

   if (mkdirErrno == EEXIST)
   {
      // ensure the right path permissions for the last element
      int chmodRes = chmod(destMount.c_str(), sourceStat.st_mode);
      if(chmodRes)
         log->log(Log_DEBUG, __FUNCTION__, "Couldn't not change mode of directory: "
            + destMount + " (" + System::getErrString(errno) + ")");
      // ensure the right path owner for the last element
      int chownRes = chown(destMount.c_str(), sourceStat.st_uid, sourceStat.st_gid);
      if(chownRes && (errno != EPERM) )
         log->log(Log_DEBUG, __FUNCTION__, "Couldn't not change owner of directory: " +
            destMount + " (" + System::getErrString(errno) + ")");
   }

   return true;
}

/**
 * Create path directories. The owner and the mode is used from a source directory.
 *
 * Note: Existing directories are ignored (i.e. not an error).
 *
 * @param destPath the path to create
 * @param sourceMount the mount point of the source path to clone (keep mode and owner)
 * @param destMount the mount point of the path to create
 * @param excluseLastElement true if the last element should not be created.
 * @param log the logger to log error messages
 * @return true on success, false otherwise and errno is set from mkdir() in this case.
 */
bool CachePathTk::createPath(const char* destPath, std::string sourceMount, std::string destMount,
   bool excludeLastElement, Logger* log)
{
   std::string newDestPath(destPath);

   return CachePathTk::createPath(newDestPath, sourceMount, destMount, excludeLastElement, log);
}

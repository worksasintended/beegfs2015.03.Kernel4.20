#include <common/threading/SafeRWLock.h>
#include <common/toolkit/StringTk.h>
#include <deeper/deeper_cache.h>
#include <toolkit/StringPathTk.h>
#include "DeeperCache.h"


#include <ftw.h>
#include <sys/stat.h>


#ifndef BEEGFS_VERSION
   #error BEEGFS_VERSION undefined
#endif

#if !defined(BEEGFS_VERSION_CODE) || (BEEGFS_VERSION_CODE == 0)
   #error BEEGFS_VERSION_CODE undefined
#endif


static pthread_key_t nftwPathkey;
static pthread_once_t nftwKeyOncePathKey = PTHREAD_ONCE_INIT;

static pthread_key_t nftwFlushDiscard;
static pthread_once_t nftwKeyOnceFlushDiscard = PTHREAD_ONCE_INIT;


// static data member for singleton which is visible to external programs which use this lib
DeeperCache* DeeperCache::cacheInstance = NULL;

/**
 * constructor
 */
DeeperCache::DeeperCache()
{
   char* argvLib[1];
   argvLib[0] = (char*) malloc(20);
   strcpy(argvLib[0], "beegfs_deeper_lib");
   int argcLib = 1;

   this->cfg = new Config(argcLib, argvLib);
   this->logger = new Logger(this->cfg);
   this->logger->log(Log_DEBUG, "beegfs-deeper-lib", std::string("BeeGFS DEEP-ER Cache library "
      "- Version: ") + BEEGFS_VERSION + " - Cache ID: " +
      StringTk::uint64ToStr(this->cfg->getSysCacheID() ) );

   free(argvLib[0]);
}

/**
 * destructor
 */
DeeperCache::~DeeperCache()
{
   SAFE_DELETE(logger);
   SAFE_DELETE(cfg);
}

/**
 * Initialize or get a deeper-cache object as a singleton
 */
DeeperCache* DeeperCache::getInstance()
{
   if(!cacheInstance)
      cacheInstance = new DeeperCache();

   return cacheInstance;
}

/**
 * cleanup method to call the destructor in the lib
 */
void DeeperCache::cleanup()
{
   SAFE_DELETE(cacheInstance);
}


/**
 * Create a new directory on the cache layer of the current cache domain.
 *
 * @param path path and name of new directory.
 * @param mode permission bits, similar to POSIX mkdir (S_IRWXU etc.).
 * @return 0 on success, -1 and errno set in case of error.
 */
int DeeperCache::cache_mkdir(const char *path, mode_t mode)
{
   int retVal = DEEPER_RETVAL_ERROR;

   std::string newPath(path);

#ifdef BEEGFS_DEBUG
   this->logger->logErr(__FUNCTION__, "path: " + newPath);
#endif

   if(CachePathTk::globalPathToCachePath(newPath, this->cfg, this->logger) )
      retVal = mkdir(newPath.c_str(), mode);
   else
      errno = EINVAL;

   return retVal;
}

/**
 * Remove a directory from the cache layer.
 *
 * @param path path and name of directory, which should be removed.
 * @return 0 on success, -1 and errno set in case of error.
 */
int DeeperCache::cache_rmdir(const char *path)
{
   int retVal = DEEPER_RETVAL_ERROR;
   std::string newPath(path);

#ifdef BEEGFS_DEBUG
   this->logger->logErr(__FUNCTION__, "path: " + newPath);
#endif

   if(CachePathTk::globalPathToCachePath(newPath, this->cfg, this->logger) )
      retVal = rmdir(newPath.c_str() );
   else
      errno = EINVAL;

   return retVal;
}

/**
 * Open a directory stream for a directory on the cache layer of the current cache domain and
 * position the stream at the first directory entry.
 *
 * @param name path and name of directory on cache layer.
 * @return pointer to directory stream, NULL and errno set in case of error.
 */
DIR* DeeperCache::cache_opendir(const char *name)
{
   DIR* retVal = NULL;
   std::string newName(name);

#ifdef BEEGFS_DEBUG
   this->logger->logErr(__FUNCTION__, "path: " + newName);
#endif

   if(CachePathTk::globalPathToCachePath(newName, this->cfg, this->logger) )
      retVal = opendir(newName.c_str() );
   else
      errno = EINVAL;

   return retVal;
}

/**
 * Close directory stream.
 *
 * @param dirp directory stream, which should be closed.
 * @return 0 on success, -1 and errno set in case of error.
 */
int DeeperCache::cache_closedir(DIR *dirp)
{
   return closedir(dirp);
}

/**
 * Open (and possibly create) a file on the cache layer.
 *
 * Any following write()/read() on the returned file descriptor will write/read data to/from the
 * cache layer. Data on the cache layer will be visible only to those nodes that are part of the
 * same cache domain.
 *
 * @param path path and name of file, which should be opened.
 * @param oflag access mode flags, similar to POSIX open (O_WRONLY, O_CREAT, etc).
 * @param mode the permissions of the file, similar to POSIX open. When oflag contains a file
 *        creation flag (O_CREAT, O_EXCL, O_NOCTTY, and O_TRUNC) the mode flag is required in other
 *        cases this flag is ignored.
 * @param deeper_open_flags zero or a combination of the following flags:
 *        DEEPER_OPEN_FLUSHONCLOSE to automatically flush written data to global
 *           storage when the file is closed, asynchronously;
 *        DEEPER_OPEN_FLUSHWAIT to make DEEPER_OPEN_FLUSHONCLOSE a synchronous operation, which
 *        means the close operation will only return after flushing is complete;
 *        DEEPER_OPEN_DISCARD to remove the file from the cache layer after it has been closed.
 * @return file descriptor as non-negative integer on success, -1 and errno set in case of error.
 */
int DeeperCache::cache_open(const char* path, int oflag, mode_t mode, int deeper_open_flags)
{
   int retFD = DEEPER_RETVAL_ERROR;
   std::string newName(path);

#ifdef BEEGFS_DEBUG
   this->logger->logErr(__FUNCTION__, "path: " + newName);
#endif

   if(!CachePathTk::globalPathToCachePath(newName, this->cfg, this->logger) )
   {
      errno = EINVAL;
      return retFD;
   }

   retFD = open(newName.c_str(), oflag, mode);

   if( (retFD >= 0) && (deeper_open_flags != DEEPER_OPEN_NONE ) )
      addEntryToSessionMap(retFD, deeper_open_flags, path);

   return retFD;
}

/**
 * Close a file.
 *
 * @param fildes file descriptor of open file.
 * @return 0 on success, -1 and errno set in case of error.
 */
int DeeperCache::cache_close(int fildes)
{
   int retVal = DEEPER_RETVAL_ERROR;
   bool doFlush = false;

   DeeperCacheSession session(fildes);
   getAndRemoveEntryFromSessionMap(session);

   int deeperFlushFlags = DEEPER_FLUSH_NONE;

   if( (session.getDeeperOpenFlags() & DEEPER_OPEN_FLUSHONCLOSE) != 0 )
      doFlush = true;

   if( (session.getDeeperOpenFlags() & DEEPER_OPEN_FLUSHWAIT) != 0 )
   {
      doFlush = true;
      deeperFlushFlags |= DEEPER_FLUSH_WAIT;
   }

   if( (session.getDeeperOpenFlags() & DEEPER_OPEN_DISCARD) != 0 )
      deeperFlushFlags |= DEEPER_FLUSH_DISCARD;

   retVal = close(fildes);

   if(doFlush)
      retVal = cache_flush(session.getPath().c_str(), deeperFlushFlags);

   if( (!doFlush) && ( (session.getDeeperOpenFlags() & DEEPER_OPEN_DISCARD) != 0) )
   {
      std::string cachePath(session.getPath() );
      if(!CachePathTk::globalPathToCachePath(cachePath, this->cfg, this->logger) )
         retVal = DEEPER_RETVAL_ERROR;
      else
         retVal = unlink(cachePath.c_str() );
   }

   return retVal;
}

/**
 * Prefetch a file or directory (including contained files) from global storage to the current
 * cache domain of the cache layer, asynchronously.
 * Contents of existing files with the same name on the cache layer will be overwritten.
 *
 * @param path path to file or directory, which should be prefetched.
 * @param deeper_prefetch_flags zero or a combination of the following flags:
 *        DEEPER_PREFETCH_SUBDIRS to recursively copy all subdirs, if given path leads to a
 *           directory;
 *        DEEPER_PREFETCH_WAIT to make this a synchronous operation.
 * @return 0 on success, -1 and errno set in case of error.
 */
int DeeperCache::cache_prefetch(const char* path, int deeper_prefetch_flags)
{
   int retVal = DEEPER_RETVAL_ERROR;

   std::string cachePath(path);

#ifdef BEEGFS_DEBUG
   this->logger->logErr(__FUNCTION__, "path: " + cachePath);
#endif

   if(!CachePathTk::globalPathToCachePath(cachePath, this->cfg, this->logger) )
   {
      errno = EINVAL;
      return retVal;
   }

   if(deeper_prefetch_flags & DEEPER_PREFETCH_SUBDIRS)
      retVal = copyDir(path, cachePath.c_str(), true, false);
   else
      retVal = copyFile(path, cachePath.c_str(), NULL, false);

   return retVal;
}

/**
 * Prefetch a file similar to deeper_cache_prefetch(), but prefetch only a certain range, not the
 * whole file.
 *
 * @param path path to file, which should be prefetched.
 * @param pos start position (offset) of the byte range that should be flushed.
 * @param num_bytes number of bytes from pos that should be flushed.
 * @param deeper_prefetch_flags zero or the following flag:
  *        DEEPER_PREFETCH_WAIT to make this a synchronous operation.
 * @return 0 on success, -1 and errno set in case of error.
 */
int DeeperCache::cache_prefetch_range(const char* path, off_t start_pos, size_t num_bytes,
   int deeper_prefetch_flags)
{
   int retVal = DEEPER_RETVAL_ERROR;

   std::string cachePath(path);

#ifdef BEEGFS_DEBUG
   this->logger->logErr(__FUNCTION__, "path: " + cachePath);
#endif

   if(!CachePathTk::globalPathToCachePath(cachePath, this->cfg, this->logger) )
   {
      errno = EINVAL;
      return retVal;
   }

   retVal = copyFileRange(path, cachePath.c_str(), NULL, &start_pos, num_bytes);

   return retVal;
}

/**
 * Wait for an ongoing prefetch operation from deeper_cache_prefetch[_range]() to complete.
 *
 * @param path path to file, which has been submitted for prefetch.
 * @param deeper_prefetch_flags zero or a combination of the following flags:
 *        DEEPER_PREFETCH_SUBDIRS to recursively wait contents of all subdirs, if given path leads
 *           to a directory.
 * @return 0 on success, -1 and errno set in case of error.
 */
int DeeperCache::cache_prefetch_wait(const char* path, int deeper_prefetch_flags)
{
   // not implemented in the synchronous implementation
   return DEEPER_RETVAL_SUCCESS;
}

/**
 * Flush a file from the current cache domain to global storage, asynchronously.
 * Contents of an existing file with the same name on global storage will be overwritten.
 *
 * @param path path to file, which should be flushed.
 * @param deeper_flush_flags zero or a combination of the following flags:
 *        DEEPER_FLUSH_WAIT to make this a synchronous operation, which means return only after
 *           flushing is complete;
 *        DEEPER_FLUSH_SUBDIRS to recursively copy all subdirs, if given path leads to a
 *        directory;
 *        DEEPER_FLUSH_DISCARD to remove the file from the cache layer after it has been flushed.
 * @return 0 on success, -1 and errno set in case of error.
 */
int DeeperCache::cache_flush(const char* path, int deeper_flush_flags)
{
   int retVal = DEEPER_RETVAL_ERROR;

   std::string cachePath(path);

#ifdef BEEGFS_DEBUG
   this->logger->logErr(__FUNCTION__, "path: " + cachePath);
#endif

   if(!CachePathTk::globalPathToCachePath(cachePath, this->cfg, this->logger) )
   {
      errno = EINVAL;
      return retVal;
   }

   if(deeper_flush_flags & DEEPER_FLUSH_SUBDIRS)
      retVal = copyDir(cachePath.c_str(), path, false, deeper_flush_flags & DEEPER_FLUSH_DISCARD);
   else
      retVal = copyFile(cachePath.c_str(), path, NULL, deeper_flush_flags & DEEPER_FLUSH_DISCARD);

   return retVal;
}

/**
 * Flush a file similar to deeper_cache_flush(), but flush only a certain range, not the whole
 * file.
 *
 * @param path path to file, which should be flushed.
 * @param pos start position (offset) of the byte range that should be flushed.
 * @param num_bytes number of bytes from pos that should be flushed.
 * @param deeper_flush_flags zero or a combination of the following flags:
 *        DEEPER_FLUSH_WAIT to make this a synchronous operation.
 * @return 0 on success, -1 and errno set in case of error.
 */
int DeeperCache::cache_flush_range(const char* path, off_t start_pos, size_t num_bytes,
   int deeper_flush_flags)
{
   int retVal = DEEPER_RETVAL_ERROR;

   std::string cachePath(path);

#ifdef BEEGFS_DEBUG
   this->logger->logErr(__FUNCTION__, "path: " + cachePath);
#endif

   if(!CachePathTk::globalPathToCachePath(cachePath, this->cfg, this->logger) )
   {
      errno = EINVAL;
      return retVal;
   }

   retVal = copyFileRange(cachePath.c_str(), path, NULL, &start_pos, num_bytes);

   return retVal;
}

/**
 * Wait for an ongoing flush operation from deeper_cache_flush[_range]() to complete.
 *
 * @param path path to file, which has been submitted for flush.
 * @param deeper_flush_flags zero or a combination of the following flags:
 *        DEEPER_FLUSH_SUBDIRS to recursively wait contents of all subdirs, if given path leads
 *           to a directory.
 * @return 0 on success, -1 and errno set in case of error.
 */
int DeeperCache::cache_flush_wait(const char* path, int deeper_flush_flags)
{
   // not implemented in the synchronous implementation
   return DEEPER_RETVAL_SUCCESS;
}

/**
 * Return the stat information of a file or directory of the cache domain.
 *
 * @param path to a file or directory on the global file system
 * @param out_stat_data the stat information of the file or directory
 * @return 0 on success, -1 and errno set in case of error.
 */
int DeeperCache::cache_stat(const char *path, struct stat *out_stat_data)
{
   int retVal = DEEPER_RETVAL_ERROR;
   std::string newPath(path);

#ifdef BEEGFS_DEBUG
   this->logger->logErr(__FUNCTION__, "path: " + newPath);
#endif

   if(CachePathTk::globalPathToCachePath(newPath, this->cfg, this->logger) )
      retVal = stat(newPath.c_str(), out_stat_data);
   else
      errno = EINVAL;

   return retVal;
}

/**
 * Return a unique identifier for the current cache domain.
 *
 * @param out_cache_id pointer to a buffer in which the ID of the current cache domain will be
 *        stored on success.
 * @return 0 on success, -1 and errno set in case of error.
 */
int DeeperCache::cache_id(const char* path, uint64_t* out_cache_id)
{
   std::string newPath(path);

#ifdef BEEGFS_DEBUG
   this->logger->logErr(__FUNCTION__, "path: " + newPath);
#endif

   if(newPath[0] != '/')
   {
      if(CachePathTk::makePathAbsolute(newPath, this->logger) )
         return DEEPER_RETVAL_ERROR;
   }

   size_t startPos = newPath.find(cfg->getSysMountPointGlobal() );
   if(startPos != 0)
   {
      this->logger->logErr(__FUNCTION__, "The given path doesn't point to a global BeeGFS: " +
         std::string(path) );
      errno = EINVAL;
      return DEEPER_RETVAL_ERROR;
   }

   *out_cache_id = this->cfg->getSysCacheID();

   return DEEPER_RETVAL_SUCCESS;
}



/**
 * add file descriptor with the deeper_open_flags to the map
 *
 * @param fd the fd of the file
 * @param deeper_open_flags the deeper_open_flags which was used during the open
 * @param path the path of the file
 */
void DeeperCache::addEntryToSessionMap(int fd, int deeper_open_flags, std::string path)
{
   SafeMutexLock lock(&fdMapMutex);                         // W R I T E L O C K

   DeeperCacheSession session(fd, deeper_open_flags, path);
   std::pair<DeeperCacheSessionMapIter, bool> pair = this->sessionMap.insert(
      DeeperCacheSessionMapVal(fd, session) );

   if(!(pair.second) )
      this->logger->logErr(__FUNCTION__, "Could not add FD to session map: " +
         StringTk::intToStr(fd) + " (" + path + ")");

   lock.unlock();                                           // U N L O C K
}

/**
 * get the DeeperCacheSession for a given file descriptor and remove the session from the map
 *
 * @param inOutSession the session with the searched FD and after calling this method it contains
 *                     all required information
 */
void DeeperCache::getAndRemoveEntryFromSessionMap(DeeperCacheSession& inOutSession)
{
   SafeMutexLock lock(&fdMapMutex);                         // W R I T E L O C K

   DeeperCacheSessionMapIter iter = this->sessionMap.find(inOutSession.getFD() );
   if(iter != this->sessionMap.end() )
   {
      inOutSession.setDeeperOpenFlags(iter->second.getDeeperOpenFlags() );
      inOutSession.setPath(iter->second.getPath() );

      this->sessionMap.erase(iter);
   }
   else
      this->logger->log(Log_DEBUG, __FUNCTION__, "Could not find FD in session map: " +
         StringTk::intToStr(inOutSession.getFD() ) );

   lock.unlock();                                           // U N L O C K
}



/**
 * copy a file, set the mode and owner of the source file to the destination file
 *
 * @param sourcePath the path to the source file
 * @param destPath the path to the destination file
 * @param statSource the stat of the source file, if this pointer is NULL this function stats the
 *        source file
 * @param deleteSource true if the file should be deleted after the file was copied
 * @return 0 on success, -1 and errno set in case of error.
 */
int DeeperCache::copyFile(const char* sourcePath, const char* destPath,
   const struct stat* statSource, bool deleteSource)
{
   int retVal = DEEPER_RETVAL_ERROR;

#ifdef BEEGFS_DEBUG
   this->logger->logErr(__FUNCTION__, "source path: " + std::string(sourcePath) +
      " - dest path: " + destPath);
#endif

   int openFlagSource = O_RDONLY;
   int openFlagDest = O_WRONLY | O_CREAT | O_TRUNC;

   bool error = false;
   int saveErrno = 0;
   int dest;

   int readSize;
   int writeSize;

   size_t bufferSize = 8192;
   char buf[bufferSize];

   int source = open(sourcePath, openFlagSource, 0);
   if(source == DEEPER_RETVAL_ERROR)
   {
      this->logger->logErr(__FUNCTION__, "Could not open source file: " +
         std::string(sourcePath) + " Errno: " + System::getErrString(errno) );
      return retVal;
   }

   struct stat newStatSource;

   if(!statSource)
   {
      retVal = fstat(source, &newStatSource);
      if(retVal == DEEPER_RETVAL_ERROR)
      {
         saveErrno = errno;
         error = true;

         this->logger->logErr(__FUNCTION__, "Could not stat source file: " +
            std::string(sourcePath) + " Errno: " + System::getErrString(saveErrno) );

         goto close_source;
      }

      // open with the stat infomation from this message
      dest = open(destPath, openFlagDest, newStatSource.st_mode);
   }
   else // open with mode from external stat information
      dest = open(destPath, openFlagDest, statSource->st_mode);

   if(dest == DEEPER_RETVAL_ERROR)
   {
      saveErrno = errno;
      error = true;

      this->logger->logErr(__FUNCTION__, "Could not open destination file: " +
         std::string(destPath) + " Errno: " + System::getErrString(saveErrno) );

      goto close_source;
   }


   while ( (readSize = read(source, buf, bufferSize) ) > 0)
   {
      writeSize = write(dest, buf, readSize);

      if(unlikely(writeSize != readSize) )
      {
         saveErrno = errno;
         error = true;

         this->logger->logErr(__FUNCTION__, "Could not copy file: " + std::string(sourcePath)
            + " to " + std::string(destPath) + " Errno: " + System::getErrString(saveErrno) );

         goto close_dest;
      }
   }


   if(!statSource)
      retVal = fchown(dest, newStatSource.st_uid, newStatSource.st_gid);
   else
      retVal = fchown(dest, statSource->st_uid, statSource->st_gid);

   if( (retVal == DEEPER_RETVAL_ERROR) && (errno != EPERM) )
   {
      saveErrno = errno;
      error = true;

      this->logger->logErr(__FUNCTION__, "Could not set owner of file: " +
         std::string(destPath) + " Errno: " + System::getErrString(saveErrno) );
   }


close_dest:
   retVal = close(dest);
   if(!error && (retVal == DEEPER_RETVAL_ERROR) )
   {
      saveErrno = errno;
      error = true;
   }

close_source:
   retVal = close(source);
   if(!error && (retVal == DEEPER_RETVAL_ERROR) )
   {
      saveErrno = errno;
      error = true;
   }

   if(error)
   {
      errno = saveErrno;
      retVal = DEEPER_RETVAL_ERROR;
   }
   else
   if(deleteSource)
   {
      if(unlink(sourcePath) )
         this->logger->logErr(__FUNCTION__, "Could not delete file after copy " +
            std::string(sourcePath) + " Errno: " + System::getErrString(errno) );
   }

   return retVal;
}

/**
 * copy a range of file, set the mode and owner of the source file to the destination file
 *
 * @param sourcePath the path to the source file
 * @param destPath the path to the destination file
 * @param statSource the stat of the source file, if this pointer is NULL this function stats the
 *        source file
 * @param offset the start offset to copy the file, required if copyRange is true, it returns
 *        the new offset for the next read, ...
 * @param numBytes the number of bytes to copy starting by the offset, required if copyRange is true
 * @return 0 on success, -1 and errno set in case of error.
 */
int DeeperCache::copyFileRange(const char* sourcePath, const char* destPath,
   const struct stat* statSource, off_t* offset, size_t numBytes)
{
   int retVal = DEEPER_RETVAL_ERROR;

#ifdef BEEGFS_DEBUG
   this->logger->logErr(__FUNCTION__, "source path: " + std::string(sourcePath) +
      " - dest path: " + destPath);
#endif

   int openFlagsSource = O_RDONLY;
   int openFlagsDest = O_WRONLY | O_CREAT;

   bool error = false;
   int saveErrno = 0;
   int dest;

   int readSize;
   int writeSize;
   size_t nextReadSize;
   size_t dataToRead = numBytes;

   size_t bufferSize = 8192;
   char buf[bufferSize];


   int source = open(sourcePath, openFlagsSource, 0);
   if(source == DEEPER_RETVAL_ERROR)
   {
      this->logger->logErr(__FUNCTION__, "Could not open source file: " +
         std::string(sourcePath) + " Errno: " + System::getErrString(errno) );
      return retVal;
   }

   struct stat newStatSource;

   if(!statSource)
   {
     retVal = fstat(source, &newStatSource);
     if(retVal == DEEPER_RETVAL_ERROR)
     {
        saveErrno = errno;
        error = true;

        this->logger->logErr(__FUNCTION__, "Could not stat source file: " +
           std::string(sourcePath) + " Errno: " + System::getErrString(saveErrno) );

        goto close_source;
     }

     // open with the stat infomation from this message
     dest = open(destPath, openFlagsDest, newStatSource.st_mode);
   }
   else // open with mode from external stat information
     dest = open(destPath, openFlagsDest, statSource->st_mode);

   if(dest == DEEPER_RETVAL_ERROR)
   {
     saveErrno = errno;
     error = true;

     this->logger->logErr(__FUNCTION__, "Could not open destination file: " +
        std::string(destPath) + " Errno: " + System::getErrString(saveErrno) );

     goto close_source;
   }

   retVal = lseek(source, *offset, SEEK_SET);
   if(retVal != *offset)
   {
      saveErrno = errno;
      error = true;

      this->logger->logErr(__FUNCTION__, "Could not seek in source file: " +
         std::string(sourcePath) + " Errno: " + System::getErrString(saveErrno) );

      goto close_dest;
   }

   retVal = lseek(dest, *offset, SEEK_SET);
   if(retVal != *offset)
   {
      saveErrno = errno;
      error = true;

      this->logger->logErr(__FUNCTION__, "Could not seek in destination file: " +
         std::string(destPath) + " Errno: " + System::getErrString(saveErrno) );

      goto close_dest;
   }

   nextReadSize = std::min(dataToRead, bufferSize);
   while(nextReadSize > 0)
   {
      readSize = read(source, buf, nextReadSize);

      if(readSize == -1)
      {
         saveErrno = errno;
         error = true;

         this->logger->logErr(__FUNCTION__, "Could not copy file: " +
            std::string(sourcePath) + " to " + std::string(destPath) + " Errno: " +
            System::getErrString(saveErrno) );

         goto close_dest;
      }

      writeSize = write(dest, buf, readSize);

      if(unlikely(writeSize != readSize) )
      {
         saveErrno = errno;
         error = true;

         this->logger->logErr(__FUNCTION__, "Could not read data from file: " +
            std::string(sourcePath) + " Errno: " + System::getErrString(saveErrno) );

         goto close_dest;
      }

      nextReadSize -= readSize;
   }

   if(!statSource)
      retVal = fchown(dest, newStatSource.st_uid, newStatSource.st_gid);
   else
      retVal = fchown(dest, statSource->st_uid, statSource->st_gid);

   if( (retVal == DEEPER_RETVAL_ERROR) && (errno != EPERM) )
   {
      saveErrno = errno;
      error = true;

      this->logger->logErr(__FUNCTION__, "Could not set owner of file: " +
         std::string(destPath) + " Errno: " + System::getErrString(saveErrno) );
   }


close_dest:
   retVal = close(dest);
   if(!error && (retVal == DEEPER_RETVAL_ERROR) )
   {
      saveErrno = errno;
      error = true;
   }

close_source:
   retVal = close(source);
   if(!error && (retVal == DEEPER_RETVAL_ERROR) )
   {
      saveErrno = errno;
      error = true;
   }

   if(error)
   {
      errno = saveErrno;
      retVal = DEEPER_RETVAL_ERROR;
   }

   return retVal;
}


/**
 * copies a directory to an other directory using nftw()
 *
 * @param source the path to the source directory
 * @param dest the path to the destination directory
 * @param copyToCache true if the data should be copied from the global BeeGFS to the cache BeeGFS,
 *        false if the data should be copied from the cache BeeGFS to the global BeeGFS
 * @param deleteSource true if the directory should be deleted after the directory was copied
 * @return 0 on success, -1 and errno set in case of error.
 */
int DeeperCache::copyDir(const char* source, const char* dest, bool copyToCache, bool deleteSource)
{
   int retVal = DEEPER_RETVAL_ERROR;

#ifdef BEEGFS_DEBUG
   this->logger->logErr(__FUNCTION__, "source path: " + std::string(source) +
      " - dest path: " + dest);
#endif

   Config* cfg = this->getConfig();

   const unsigned maxOpenFDsNum = 100; // max open FDs
   /**
    * used nftw flags copy directories
    * FTW_ACTIONRETVAL  handles the return value from the fn(), fn() must return special values
    * FTW_PHYS          do not follow symbolic links
    * FTW_MOUNT         stay within the same file system
    */
   const int nftwFlagsCopy = FTW_ACTIONRETVAL | FTW_PHYS | FTW_MOUNT;
   /**
    * used nftw flags for delete directories
    * FTW_ACTIONRETVAL  handles the return value from the fn(), fn() must return special values
    * FTW_PHYS          do not follow symbolic links
    * FTW_MOUNT         stay within the same file system
    * FTW_DEPTH         do a post-order traversal, call fn() for the directory itself after handling
    *                   the contents of the directory and its subdirectories. By default, each
    *                   directory is handled before its contents.
    */
   const int nftwFlagsDelete = FTW_ACTIONRETVAL | FTW_PHYS | FTW_MOUNT | FTW_DEPTH;

   std::string mountOfSource;
   std::string mountOfDest;
   if(copyToCache)
   {
      mountOfSource = cfg->getSysMountPointGlobal();
      mountOfDest = cfg->getSysMountPointCache();
   }
   else
   {
      mountOfSource = cfg->getSysMountPointCache();
      mountOfDest = cfg->getSysMountPointGlobal();
   }

   struct stat statSource;
   int funcError = stat(source, &statSource);
   if(funcError != 0)
   {
      this->logger->logErr(__FUNCTION__, "Could not stat source file/directory " +
         std::string(source) + " Errno: " + System::getErrString(errno) );
      retVal = DEEPER_RETVAL_ERROR;
   }
   else
   if(S_ISREG(statSource.st_mode) )
   {
      retVal = copyFile(source, dest, &statSource, deleteSource);
   }
   else
   {
      if(!CachePathTk::createPath(dest, mountOfSource, mountOfDest, false, this->logger) )
         return retVal;

      this->setThreadKeyNftwPath(source);
      this->setThreadKeyNftwFlushDiscard(deleteSource);

      if(copyToCache)
         retVal = nftw(source, nftwCopyFileToCache, maxOpenFDsNum, nftwFlagsCopy);
      else
         retVal = nftw(source, nftwCopyFileToGlobal, maxOpenFDsNum, nftwFlagsCopy);

      if(deleteSource && (retVal == DEEPER_RETVAL_SUCCESS) )
      { // now delete all empty directories, files are deleted during the first nftw run
         retVal = nftw(source, nftwDeleteDirectories, maxOpenFDsNum, nftwFlagsDelete);
         if(retVal) // delete the directory only when the directory is empty
         {
            retVal = rmdir(source);
            if(retVal)
               this->logger->logErr("copyDir", "Could not delete directory after "
                  "copy " + std::string(source) + " Errno: " + System::getErrString(errno) );
         }
      }
   }

   return retVal;
}

/**
 * copies a file from global BeeGFS to the cache BeeGFS
 *
 * NOTE: This function is used by nftw , the interface of this function can not be changed.
 *
 * @param fpath the pathname of the entry relative to the parameter dirpath of the nftw(...) call
 * @param sb is a pointer to the stat structure returned by a call to stat for fpath
 * @param tflag typeflag of the entry which is given to this function by nftw, details in man page
 *        of nftw
 * @param ftwbuf controll flags for nftw, details in man page of nftw
 * @return 0 on success, -1 and errno set in case of error.
 */
int DeeperCache::nftwCopyFileToCache(const char *fpath, const struct stat *sb, int tflag,
   struct FTW *ftwbuf)
{
   int retVal = DEEPER_RETVAL_ERROR;
   bool funcRetVal;

   DeeperCache* cache = DeeperCache::getInstance();

   std::string sourcePath(fpath);

   std::string destPath(fpath);
   funcRetVal = CachePathTk::globalPathToCachePath(destPath, cache->getConfig(),
      cache->getLogger() );
   if(unlikely(!funcRetVal) )
   {
      errno = ENOENT;
      return retVal;
   }

   bool deleteSource = cache->getThreadKeyNftwFlushDiscard();
   retVal = cache->nftwHandleFlags(sourcePath, destPath, sb, tflag, true, deleteSource);

   return retVal;
}

/**
 * copies a file from cache BeeGFS to the global BeeGFS
 *
 * NOTE: This function is used by nftw , the interface of this function can not be changed.
 *
 * @param fpath the pathname of the entry relative to the parameter dirpath of the nftw(...) call
 * @param sb is a pointer to the stat structure returned by a call to stat for fpath
 * @param tflag typeflag of the entry which is given to this function by nftw, details in man page
 *        of nftw
 * @param ftwbuf controll flags for nftw, details in man page of nftw
 * @return 0 on success, -1 and errno set in case of error.
 */
int DeeperCache::nftwCopyFileToGlobal(const char *fpath, const struct stat *sb, int tflag,
   struct FTW *ftwbuf)
{
   int retVal = FTW_CONTINUE;
   bool funcRetVal = DEEPER_RETVAL_ERROR;

   DeeperCache* cache = DeeperCache::getInstance();

   std::string sourcePath(fpath);

   std::string destPath(fpath);
   funcRetVal = CachePathTk::cachePathToGlobalPath(destPath, cache->getConfig(),
      cache->getLogger() );
   if(unlikely(!funcRetVal) )
   {
      errno = ENOENT;
      return retVal;
   }

   bool deleteSource = cache->getThreadKeyNftwFlushDiscard();
   retVal = cache->nftwHandleFlags(sourcePath, destPath, sb, tflag, false, deleteSource);

   return retVal;
}

/**
 * handles the nftw type flags
 *
 * NOTE: this function is called by the nftw functions
 *
 * @param sourcePath the source path
 * @param destPath the destination path
 * @param sb is a pointer to the stat structure returned by a call to stat for the entry which was
 *        given to to a function by nftw, details in man page of nftw
 * @param tflag typeflag of the entry which was given to a function by nftw, details in man page of
 *        nftw
 * @param copyToCache true if the data should be copied from the global BeeGFS to the cache BeeGFS,
 *        false if the data should be copied from the cache BeeGFS to the global BeeGFS
 * @param deleteSource true if the file/directory should be deleted after the file/directory was
 *        copied
 * @return 0 on success, -1 and errno set in case of error.
 */
int DeeperCache::nftwHandleFlags(std::string sourcePath, std::string destPath,
   const struct stat *sb, int tflag, bool copyToCache, bool deleteSource)
{
   int retVal = FTW_CONTINUE;

   if(tflag == FTW_F)
   {
#ifdef BEEGFS_DEBUG
      this->logger->logErr(__FUNCTION__, "source path: " + sourcePath + " - type: file");
#endif

      retVal = this->copyFile(sourcePath.c_str(), destPath.c_str(), sb, deleteSource);
      if(retVal == DEEPER_RETVAL_ERROR)
      {
         this->logger->logErr("handleFlags", "Could not copy file from " +
            sourcePath + " to " + destPath + " Errno: " + System::getErrString(errno) );

         return FTW_STOP;
      }
   }
   else
   if( (tflag == FTW_D) || (tflag == FTW_DP) )
   {
#ifdef BEEGFS_DEBUG
      this->logger->logErr(__FUNCTION__, "source path: " + sourcePath + " - type: directory");
#endif

      retVal = mkdir(destPath.c_str(), sb->st_mode);
      if( (retVal == DEEPER_RETVAL_ERROR) && (errno != EEXIST) )
      {
         this->logger->logErr("handleFlags", "Could not create directory " +
            destPath + " Errno: " + System::getErrString(errno) );

         return FTW_STOP;
      }

      retVal = chown(destPath.c_str(), sb->st_uid, sb->st_gid);
      if( (retVal == DEEPER_RETVAL_ERROR) && (errno != EPERM) )
      { // EPERM happens when the process is not running as root, this can be ignored
         this->logger->logErr("handleFlags", "Could not change owner of directory " +
            destPath + " Errno: " + System::getErrString(errno) );

         return FTW_STOP;
      }

      /**
       * the flag deleteSource need to be handled in a second nftw tree walk, because at this point
       * the directories are not empty
       */
   }
   else
   if(tflag == FTW_SL)
   {
#ifdef BEEGFS_DEBUG
      this->logger->logErr(__FUNCTION__, "source path: " + sourcePath + " - type: link");
#endif

      char* linkname = (char*) malloc(sb->st_size + 1);

      if(linkname != NULL)
         retVal = readlink(sourcePath.c_str(), linkname, sb->st_size + 1);
      else
      {
         this->logger->logErr("handleFlags", "Could not malloc buffer for readlink " +
            sourcePath + " Errno: " + System::getErrString(errno) );

         return FTW_STOP;
      }

      if(retVal < 0)
      {
         SAFE_FREE(linkname);

         this->logger->logErr("handleFlags", "Could not readlink data " +
            sourcePath + " Errno: " + System::getErrString(errno) );

         return FTW_STOP;
      }
      else if(retVal > sb->st_size)
      {
         SAFE_FREE(linkname);

         this->logger->logErr("handleFlags", "symlink increased in size between lstat() "
            "and readlink()" + std::string(sourcePath) );

         return FTW_STOP;
      }
      else
         linkname[sb->st_size] = '\0';

      std::string linkDest(linkname);

      if(copyToCache)
      {
         CachePathTk::globalPathToCachePath(linkDest, this->cfg, this->logger);
      }
      else
      {
         CachePathTk::cachePathToGlobalPath(linkDest, this->cfg, this->logger);
      }

      if(retVal == DEEPER_RETVAL_ERROR)
         return FTW_STOP;

      SAFE_FREE(linkname);

      retVal = symlink(linkDest.c_str(), destPath.c_str() );
      if(retVal == DEEPER_RETVAL_ERROR)
      {
         this->logger->logErr("handleFlags", "Could not create symlink " +
            destPath + " to " + linkDest + " Errno: " + System::getErrString(errno) );

         return FTW_STOP;
      }

      if(deleteSource)
      {
         if(unlink(sourcePath.c_str() ) )
            this->logger->logErr("handleFlags", "Could not delete symlink after copy " +
               sourcePath + " Errno: " + System::getErrString(errno) );
      }
   }
   else
   if( (tflag == FTW_DNR) || (tflag == FTW_NS) || (tflag == FTW_SLN) )
      this->logger->logErr("handleFlags", "unexpected typeflag value" );

   return retVal;
}

/**
 * deletes the directory
 *
 * NOTE: This function is used by nftw , the interface of this function can not be changed.
 *
 * @param fpath the pathname of the entry relative to the parameter dirpath of the nftw(...) call
 * @param sb is a pointer to the stat structure returned by a call to stat for fpath
 * @param tflag typeflag of the entry which is given to this function by nftw, details in man page
 *        of nftw
 * @param ftwbuf controll flags for nftw, details in man page of nftw
 * @return 0 on success, -1 and errno set in case of error.
 */
int DeeperCache::nftwDeleteDirectories(const char *fpath, const struct stat *sb, int tflag,
   struct FTW *ftwbuf)
{
   int retVal = FTW_CONTINUE;

   if( (tflag == FTW_D) || (tflag == FTW_DP) )
   {
      if(rmdir(fpath) )
      {
         DeeperCache::getInstance()->getLogger()->logErr("deleteDirectories", "Could not "
            "delete directory after copy " + std::string(fpath) + " Errno: " +
            System::getErrString(errno) );
         retVal = FTW_STOP;
      }
   }
   else
   if( (tflag == FTW_F) || (tflag == FTW_SL) )
      DeeperCache::getInstance()->getLogger()->logErr("deleteDirectories",
         "found a file or symlink: " + std::string(fpath) + " all files and symlinks should be "
         "deleted");
   else
   if( (tflag == FTW_DNR) || (tflag == FTW_NS) || (tflag == FTW_SLN) )
      DeeperCache::getInstance()->getLogger()->logErr("deleteDirectories",
         "unexpected typeflag value");

   return retVal;
}

/**
 * constructor for the thread variable of the path from the last nftw(...) call
 */
void DeeperCache::makeThreadKeyNftwPath()
{
   pthread_key_create(&nftwPathkey, freeThreadKeyNftwPath);
}

/**
 * destructor of the thread variable of the path from the last nftw(...) call
 *
 * @param value pointer of the thread variable
 */
void DeeperCache::freeThreadKeyNftwPath(void* value)
{
   SAFE_DELETE_NOSET((std::string*) value);
}

/**
 * stores the path which was given to nftw(...) as a thread variable
 *
 * @param path the path to save as thread variable
 */
void DeeperCache::setThreadKeyNftwPath(const char* path)
{
   pthread_once(&nftwKeyOncePathKey, makeThreadKeyNftwPath);

   void* ptr;

   if ((ptr = pthread_getspecific(nftwPathkey)) != NULL)
      SAFE_DELETE_NOSET((std::string*) ptr);

   ptr = new std::string(path);
   pthread_setspecific(nftwPathkey, ptr);
}

/**
 * reads the thread variable which contains the path of the last nftw(...) call
 *
 * @return the path which was given to nftw
 */
std::string DeeperCache::getThreadKeyNftwPath()
{
   pthread_once(&nftwKeyOncePathKey, makeThreadKeyNftwPath);

   void* ptr;

   if ((ptr = pthread_getspecific(nftwPathkey)) != NULL)
      return std::string(*(std::string*) ptr);

   return NULL;
}

/**
 * constructor for the thread variable of the flush flag from the last nftw(...) call
 */
void DeeperCache::makeThreadKeyNftwFlushDiscard()
{
   pthread_key_create(&nftwFlushDiscard, freeThreadKeyNftwFlushDiscard);
}

/**
 * destructor of the thread variable of the flush flag from the last nftw(...) call
 *
 * @param value pointer of the thread variable
 */
void DeeperCache::freeThreadKeyNftwFlushDiscard(void* value)
{
   SAFE_DELETE_NOSET((bool*) value);
}

/**
 * stores the flush flags which was given to nftw(...) as a thread variable
 *
 * @param flushFlag the path to save as thread variable
 */
void DeeperCache::setThreadKeyNftwFlushDiscard(bool flushFlag)
{
   pthread_once(&nftwKeyOnceFlushDiscard, makeThreadKeyNftwFlushDiscard);

   void* ptr;

   if ((ptr = pthread_getspecific(nftwFlushDiscard)) != NULL)
      SAFE_DELETE_NOSET((bool*) ptr);

   ptr = new bool(flushFlag);
   pthread_setspecific(nftwFlushDiscard, ptr);
}

/**
 * reads the thread variable which contains the flush flags of the last nftw(...) call
 *
 * @return flush flags of the last nftw(...) call
 */
bool DeeperCache::getThreadKeyNftwFlushDiscard()
{
   pthread_once(&nftwKeyOnceFlushDiscard, makeThreadKeyNftwFlushDiscard);

   void* ptr;

   if ((ptr = pthread_getspecific(nftwFlushDiscard)) != NULL)
      return bool(*(bool*) ptr);

   return false; // not the best solution, but doesn't destroy the data
}

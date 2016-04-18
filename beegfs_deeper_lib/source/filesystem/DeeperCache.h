#ifndef FILESYSTEM_DEEPERCACHE_H_
#define FILESYSTEM_DEEPERCACHE_H_


#include <app/config/Config.h>
#include <common/threading/Mutex.h>
#include <common/Common.h>
#include <common/app/log/Logger.h>
#include <session/DeeperCacheSession.h>

#include <dirent.h>


typedef std::map<int, DeeperCacheSession> DeeperCacheSessionMap;
typedef DeeperCacheSessionMap::iterator DeeperCacheSessionMapIter;
typedef DeeperCacheSessionMap::value_type DeeperCacheSessionMapVal;


class DeeperCache
{
   public:
      int cache_mkdir(const char *path, mode_t mode);
      int cache_rmdir(const char *path);

      DIR* cache_opendir(const char *name);
      int cache_closedir(DIR *dirp);

      int cache_open(const char* path, int oflag, mode_t mode, int deeper_open_flags);
      int cache_close(int fildes);

      int cache_prefetch(const char* path, int deeper_prefetch_flags);
      int cache_prefetch_range(const char* path, off_t start_pos, size_t num_bytes,
         int deeper_prefetch_flags);
      int cache_prefetch_wait(const char* path, int deeper_prefetch_flags);

      int cache_flush(const char* path, int deeper_flush_flags);
      int cache_flush_range(const char* path, off_t start_pos, size_t num_bytes,
         int deeper_flush_flags);
      int cache_flush_wait(const char* path, int deeper_flush_flags);

      int cache_stat(const char *path, struct stat *out_stat_data);

      int cache_id(const char* path, uint64_t* out_cache_id);


   private:
      DeeperCache();
      ~DeeperCache();
      DeeperCache(DeeperCache const&);       // not allowed for a singelton
      void operator=(DeeperCache const&);    // not allowed for a singelton

      Config* cfg;
      Logger* logger;

      Mutex fdMapMutex;
      DeeperCacheSessionMap sessionMap;

      Mutex prefetchMapMutex;
      StringList prefetchList;

      Mutex flushMapMutex;
      StringList flushList;

      static DeeperCache* cacheInstance;

      static void makeThreadKeyNftwPath();
      static void freeThreadKeyNftwPath(void* value);
      void setThreadKeyNftwPath(const char* path);
      std::string getThreadKeyNftwPath();

      static void makeThreadKeyNftwFlushDiscard();
      static void freeThreadKeyNftwFlushDiscard(void* value);
      void setThreadKeyNftwFlushDiscard(bool flushFlag);
      bool getThreadKeyNftwFlushDiscard();


      static int nftwCopyFileToCache(const char *fpath, const struct stat *sb, int tflag,
         struct FTW *ftwbuf);
      static int nftwCopyFileToGlobal(const char *fpath, const struct stat *sb, int tflag,
         struct FTW *ftwbuf);
      int nftwHandleFlags(std::string sourcePath, std::string destPath, const struct stat *sb,
         int tflag, bool copyToCache, bool deleteSource);
      static int nftwDeleteDirectories(const char *fpath, const struct stat *sb, int tflag,
         struct FTW *ftwbuf);

      void addEntryToSessionMap(int fd, int deeper_open_flags, std::string path);
      void getAndRemoveEntryFromSessionMap(DeeperCacheSession& inOutSession);

      int copyFile(const char* sourcePath, const char* destPath, const struct stat* statSource,
         bool deleteSource);
      int copyFileRange(const char* sourcePath, const char* destPath, const struct stat* statSource,
         off_t* offset, size_t numBytes);
      int copyDir(const char* source, const char* dest, bool copyToCache, bool deleteSource);



   public:
      static DeeperCache* getInstance();
      static void cleanup();

      Config* getConfig()
      {
         return this->cfg;
      }

      Logger* getLogger()
      {
         return this->logger;
      }
};

#endif /* FILESYSTEM_DEEPERCACHE_H_ */

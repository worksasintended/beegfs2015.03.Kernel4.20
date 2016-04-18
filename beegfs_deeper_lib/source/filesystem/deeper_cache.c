#include <filesystem/DeeperCache.h>

#include "deeper_cache.h"


int deeper_cache_mkdir(const char *path, mode_t mode)
{
   return DeeperCache::getInstance()->cache_mkdir(path, mode);
}

int deeper_cache_rmdir(const char *path)
{
   return DeeperCache::getInstance()->cache_rmdir(path);
}

DIR *deeper_cache_opendir(const char *name)
{
   return DeeperCache::getInstance()->cache_opendir(name);
}

int deeper_cache_closedir(DIR *dirp)
{
   return DeeperCache::getInstance()->cache_closedir(dirp);
}

int deeper_cache_open(const char* path, int oflag, mode_t mode, int deeper_open_flags)
{
   return DeeperCache::getInstance()->cache_open(path, oflag, mode, deeper_open_flags);
}

int deeper_cache_close(int fildes)
{
   return DeeperCache::getInstance()->cache_close(fildes);
}

int deeper_cache_prefetch(const char* path, int deeper_prefetch_flags)
{
   return DeeperCache::getInstance()->cache_prefetch(path, deeper_prefetch_flags);
}

int deeper_cache_prefetch_range(const char* path, off_t start_pos, size_t num_bytes,
   int deeper_prefetch_flags)
{
   return DeeperCache::getInstance()->cache_prefetch_range(path, start_pos, num_bytes,
      deeper_prefetch_flags);
}

int deeper_cache_prefetch_wait(const char* path, int deeper_prefetch_flags)
{
   return DeeperCache::getInstance()->cache_prefetch_wait(path, deeper_prefetch_flags);
}

int deeper_cache_flush(const char* path, int deeper_flush_flags)
{
   return DeeperCache::getInstance()->cache_flush(path, deeper_flush_flags);
}

int deeper_cache_flush_range(const char* path, off_t start_pos, size_t num_bytes, 
   int deeper_flush_flags)
{
   return DeeperCache::getInstance()->cache_flush_range(path, start_pos, num_bytes,
      deeper_flush_flags);
}

int deeper_cache_flush_wait(const char* path, int deeper_flush_flags)
{
   return DeeperCache::getInstance()->cache_flush_wait(path, deeper_flush_flags);
}

int deeper_cache_stat(const char *path, struct stat *out_stat_data)
{
   return DeeperCache::getInstance()->cache_stat(path, out_stat_data);
}

int deeper_cache_id(const char* path, uint64_t* out_cache_id)
{
   return DeeperCache::getInstance()->cache_id(path, out_cache_id);
}

void deeper_cache_init()
{
   DeeperCache::getInstance();
}

void deeper_cache_destruct()
{
   DeeperCache::cleanup();
}

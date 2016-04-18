#ifndef TOOLKIT_STRINGPATHTK_H_
#define TOOLKIT_STRINGPATHTK_H_


#include <app/config/Config.h>
#include <common/app/log/Logger.h>
#include <common/Common.h>


class CachePathTk
{
   public:
      static bool isGlobalPath(std::string* path, Config* deeperLibConfig);
      static bool isCachePath(std::string* path, Config* deeperLibConfig);
      static bool globalPathToCachePath(std::string& inOutPath, Config* deeperLibConfig,
         Logger* log);
      static bool cachePathToGlobalPath(std::string& inOutPath, Config* deeperLibConfig,
         Logger* log);

      static bool makePathAbsolute(std::string& inOutPath, Logger* log);

      static bool createPath(std::string destPath, std::string sourceMount,
         std::string destMount, bool setModeFromSource, Logger* log);
      static bool createPath(const char* destPath, std::string sourceMount, std::string destMount,
         bool excludeLastElement, Logger* log);

   private:
      CachePathTk();

      static bool replaceRootPath(std::string& inOutPath, std::string& origPath,
         std::string& newPath, Logger* log);
};

#endif /* TOOLKIT_STRINGPATHTK_H_ */

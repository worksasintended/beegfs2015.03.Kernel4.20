#ifndef STORAGETK_H_
#define STORAGETK_H_

#include <common/Common.h>
#include <common/app/config/InvalidConfigException.h>
#include <common/storage/Path.h>
#include <common/storage/PathVec.h>
#include <common/storage/PathInfo.h>
#include <common/storage/striping/ChunkFileInfo.h>
#include <common/storage/Storagedata.h>
#include <common/storage/striping/StripePattern.h>
#include <common/storage/StatData.h>
#include <common/threading/Atomics.h>
#include <common/threading/Mutex.h>
#include <common/threading/SafeMutexLock.h>

#include <dirent.h>


#define STORAGETK_FILE_MAX_LINE_LENGTH       1024
#define STORAGETK_FILE_COMMENT_CHAR          '#'
#define STORAGETK_ORIGINALNODEID_FILENAME    "originalNodeID" /* contains first-run nodeID */
#define STORAGETK_NODEID_FILENAME            "nodeID" /* to force a certain nodeID */
#define STORAGETK_NODENUMID_FILENAME         "nodeNumID" /* contains first-run numeric node ID */
#define STORAGETK_TARGETID_FILENAME          "targetID" /* contains first-run targetID */
#define STORAGETK_TARGETNUMID_FILENAME       "targetNumID" /* contains first-run targetNumID */
#define STORAGETK_SESSIONS_BACKUP_FILE_NAME  "sessions" /* contains session backup information */


#define STORAGETK_FILEID_TIMESTAMP_SHIFTBITS  32 /* timestamp is shifted by this number of bits */

#define STORAGETK_CHUNKDIR_V2_VEC_SIZE         2 // old V2 format (hashDir1/hashDir2/)
#define STORAGETK_CHUNKDIR_VEC_SIZE            4

// positions within 'PathVec chunkDirPath'
#define STORAGETK_CHUNKDIR_VEC_UIDPOS              0

/* RPOS -> reverse pos, so from the end of the string, 16^n below, as this is char 'F' time stamps
 * We use fixed string positions, which only approximately correspond to the right time,
 * STORAGETK_TIMESTAMP_YEAR_RPOS changes approximately every 1/2 year instead of every year, but
 * the goal is not to make it exact, but to get object storage directories based on the string.
 * Note: string counting starts with 0, but size = pos + 1 and we need 16^size
 * */
#define STORAGETK_TIMESTAMP_DAY_RPOS     3 // 1d = 86400s; 16^4 = 65,536
#define STORAGETK_TIMESTAMP_MONTH_RPOS   4 // 1m = 30d  = 2,529,000s; 16^5  = 1,048,576
#define STORAGETK_TIMESTAMP_YEAR_RPOS    5 // 1y = 365d = 31,536,000s; 16^6 = 16,777,216


class StorageTk
{
   public:
      static void initHashPaths(Path& basePath, int maxLevel1, int maxLevel2);

      static bool createPathOnDisk(Path& path, bool excludeLastElement, mode_t* mode = NULL);
      static bool createPathOnDisk(int fd, Path& relativePath, bool excludeLastElement,
         mode_t* mode = NULL);
      static bool removePathDirsFromDisk(Path& path, unsigned keepDepth);

      static bool statStoragePath(const std::string path, int64_t* outSizeTotal,
         int64_t* outSizeFree, int64_t* outInodesTotal, int64_t* outInodesFree);
      static bool statStoragePath(Path& path, bool excludeLastElement, int64_t* outSizeTotal,
         int64_t* outSizeFree, int64_t* outInodesTotal, int64_t* outInodesFree);
      static bool statStoragePathOverride(std::string path, int64_t* outSizeFree,
         int64_t* outInodesFree);

      static std::string getPathDirname(std::string path);
      static std::string getPathBasename(std::string path);

      static int lockWorkingDirectory(std::string path);
      static void unlockWorkingDirectory(int fd);
      static int createAndLockPIDFile(std::string path, bool writePIDToFile);
      static bool updateLockedPIDFile(int fd);
      static void unlockPIDFile(int fd);

      static bool createStorageFormatFile(const std::string pathStr, int formatVersion,
         StringMap* formatProperties = NULL);
      static void checkAndUpdateStorageFormatFile(const std::string pathStr, int minVersion,
         int currentVersion, StringMap* outFormatProperties = NULL) throw(InvalidConfigException);
      static bool checkStorageFormatFileExists(const std::string pathStr);
      static bool checkStorageSessionsFileExists(const std::string& pathStr);

      static void checkOrCreateOrigNodeIDFile(const std::string pathStr, std::string currentNodeID)
         throw(InvalidConfigException);

      static void readTargetIDFile(const std::string pathStr, std::string* outTargetID)
         throw(InvalidConfigException);
      static void readOrCreateTargetIDFile(const std::string pathStr, uint16_t localNodeID,
         std::string* outTargetID) throw(InvalidConfigException);
      static void readNumIDFile(const std::string pathStr,  const std::string filename,
         uint16_t* outTargetNumID) throw(InvalidConfigException);
      static void createNumIDFile(const std::string pathStr, const std::string filename,
         uint16_t targetID) throw(InvalidConfigException);

      static struct dirent* readdirFiltered(DIR* dirp);
      static struct dirent* readdirFilteredEx(DIR* dirp, bool filterDots, bool filterFSIDsDir);
      static void readCompleteDir(const char* path, StringList* outNames)
         throw(InvalidConfigException);

      static void updateDynamicFileInodeAttribs(ChunkFileInfoVec& fileInfoVec,
         StripePattern* stripePattern, StatData* outStatData);

      static void getChunkDirChunkFilePath(const PathInfo* pathInfo, std::string entryID,
         bool hasOrigFeature, PathVec& outChunkDirPath, std::string& outCompleteChunkFilePathStr);

      static bool rmDirContentsRec(int fd, std::string relativeDirPath);
      static bool rmDirContentsRec(std::string& absoluteDirPath);

   private:
      StorageTk() {}

      static AtomicUInt64 idCounter; // high 32bit are timestamp, low 32bits are sequential counter


      static bool removePathDirsRec(StringListConstIter dirNameIter, std::string pathStr,
         unsigned currentDepth, unsigned numPathElems, unsigned keepDepth);


   public:
      // inliners

      /**
       * Check if given path exists.
       */
      static bool pathExists(std::string path)
      {
         int statRes = access(path.c_str(), F_OK);

         return statRes ? false : true;
      }

      /*
       * Check if given path exists, but path is relative to a file descriptor
       */
      static bool pathExists(int dirfd, std::string relativePath)
      {
         int statRes = faccessat(dirfd, relativePath.c_str(), F_OK, 0);

         return statRes ? false : true;
      }

      /*
       * check if a path contains anything
       *
       * @return true if path has any contents, false if not or if it is not a dir or doesn't exist
       */
      static bool pathHasChildren(const std::string& path)
      {
         bool retVal;
         struct dirent *d;

         DIR *dir = opendir(path.c_str());

         if (dir == NULL) // not a directory or doesn't exist
           return false;

         if  ((d = readdirFilteredEx(dir, true, false)) != NULL)
            retVal = true; // something is there
         else
            retVal = false;

         closedir(dir);

         return retVal;
      }

      /**
       * Generate ID for new fs entry (i.e. file or dir).
       *
       * Note: Currently also used for targetIDs.
       */
       static std::string generateFileID(uint16_t localNodeID)
      {
         /* note: we assume here that the clock doesn't jump backwards between restarts of
            the daemon (and that there always is at least one second between restarts) and that we
            don't need more than 2^32 IDs per second (sustained) */

         uint64_t nextID = idCounter.increase();

         // note on idCounter value: high 32bits are timestamp, low 32bits are sequential counter

         /* note on switching time/counter in string representation: having the timestamp first is
           bad for strcmp() and such things, which the underlying fs might need to do - because in
           that order, the first characters of entryIDs/filenames would always be similar. */

         uint32_t counterPart = (uint32_t)nextID;
         uint32_t timestampPart = (uint32_t)(nextID >> STORAGETK_FILEID_TIMESTAMP_SHIFTBITS);

         return StringTk::uintToHexStr(counterPart) + "-" +
            StringTk::uintToHexStr(timestampPart) + "-" +
            StringTk::uint16ToHexStr(localNodeID);
      }

       /**
        * Set the id counter to the current time stamp and reset the sequential counter to 0
        *
        * Note: time is only updated if the clock didn't jump backwards, according to the previous
        * id counter value.
        *
        * Note: make sure that this is only called by a single thread and only used very carefully
        * to avoid races and ID collisions.
        */
       static void resetIDCounterToNow()
       {
          uint32_t currentSysTimeSecs = System::getCurrentTimeSecs();

          uint32_t idCounterTimestampPart =
             (uint32_t)(idCounter.read() >> STORAGETK_FILEID_TIMESTAMP_SHIFTBITS);

          if(unlikely(currentSysTimeSecs <= idCounterTimestampPart) )
             return; // don't do anything if system clock seems to have jumped backwards

          idCounter.set( (uint64_t)currentSysTimeSecs << STORAGETK_FILEID_TIMESTAMP_SHIFTBITS);
       }

      /**
       * According to linux-src/include/linux/fs.h we can convert from the file mode as provided in
       * 'struct stat' by the field 'st_mode' to the file type as given by
       * 'struct dirent' in the field 'd_type' by some bit magic operations
       */
      static int modeToDentryType(int mode)
      {
         int dentryType = (mode >> 12) & 15;

         return dentryType;
      }

      static void getHashes(const std::string hashStr, size_t numHashesLevel1,
          size_t numHashesLevel2, uint16_t& outHashLevel1, uint16_t& outHashLevel2)
      {
         uint32_t checksum = StringTk::strChecksum(hashStr.c_str(), hashStr.length() );

         outHashLevel1 =  ((uint16_t)(checksum >> 16) ) % numHashesLevel1;
         outHashLevel2 =  ((uint16_t) checksum)         % numHashesLevel2;
      }


      static unsigned getChunkHash(const std::string id,
         size_t numHashesLevel1, size_t numHashesLevel2)
      {
         uint16_t hashLevel1;
         uint16_t hashLevel2;

         getHashes(id, numHashesLevel1, numHashesLevel2, hashLevel1, hashLevel2);

         unsigned outHash = ( (unsigned (hashLevel1)) << 16) + hashLevel2;

         return outHash;
      }

      /**
       * Get complete path to entryID for chunk files.
       *
       * Note: computes hash based on entryID.
       *
       * @param outChunkDirPathVec may be NULL
       * @return hashDir1/hashDir2/entryID
       */
      static std::string getFileChunkPathV2(const std::string entryID, PathVec* outChunkDirPathVec)
      {
         std::string outChunkFilePath = getBaseHashPath(entryID,
            CONFIG_CHUNK_LEVEL1_SUBDIR_NUM, CONFIG_CHUNK_LEVEL2_SUBDIR_NUM);

         if (outChunkDirPathVec)
         {
            outChunkDirPathVec->reserve(STORAGETK_CHUNKDIR_V2_VEC_SIZE);

            // convert the path into a vector
            outChunkDirPathVec->parseStr(outChunkFilePath);
            outChunkDirPathVec->popLastElem();
         }

         return outChunkFilePath;
      }

      static void timeStampToPath(std::string& timeStamp,
          std::string& outYearMonthStr, std::string& outDayChar)
      {
         const size_t timeStampLastPos = timeStamp.length() - 1;
         const ssize_t timeStampDayPos = timeStampLastPos - STORAGETK_TIMESTAMP_DAY_RPOS;

         if (unlikely(timeStampDayPos < 0) )
            outDayChar = "0";
         else
            outDayChar = timeStamp.substr(timeStampLastPos - STORAGETK_TIMESTAMP_DAY_RPOS, 1);

         if (timeStampDayPos == 0)
            outYearMonthStr = "0";
         else
         {
            // note: timeStampDayPos == timeStampYearMonthLen
            outYearMonthStr = timeStamp.substr(0, timeStampDayPos);
         }
      }


      /**
       * Get complete path to entryID for chunk files.
       *
       * Note: computes hash based on pathInfo and entryID.
       *
       * @return
       *    chunkPathV2 (no orig feature):
       *       hashDir1/hashDir2/entryID
       *    chunkPathV3:
       *       uidX/level1/level2/parentID/entryID
       */
      static std::string getFileChunkPath(const PathInfo* pathInfo, const std::string entryID)
      {
         std::string chunkPath;

         if (!pathInfo->hasOrigFeature() )
         { // 2012.10 style hash dir
            chunkPath = getFileChunkPathV2(entryID, NULL); // old storage format
         }
         else
         { // 2014.01 style path with user and timestamp dirs

            std::string uidStr = StringTk::uintToHexStr(pathInfo->getOrigUID() ); // uid-string

            std::string timeStamp;
            StringTk::timeStampFromEntryID(pathInfo->getOrigParentEntryID(), timeStamp);

            std::string dayChar;
            std::string yearMonthStr;
            timeStampToPath(timeStamp, yearMonthStr, dayChar);

            chunkPath =
               CONFIG_CHUNK_UID_PREFIX + uidStr + "/" +
               yearMonthStr                     + "/" + // level 1 - approximately yymm
               dayChar + "/"                          + // level 2 - approximately dd
               pathInfo->getOrigParentEntryID() + "/" +
               entryID;
         }

         return chunkPath;
      }



      /**
       * Fill in outPath with the V3 chunk base path elements.
       * (uid + time1Str + time2Str + parentID)
       */
      static void getChunkDirV3PathVec(const PathInfo* pathInfo, PathVec& outPath)
      {
         outPath.reserve(STORAGETK_CHUNKDIR_VEC_SIZE);

         std::string uidStr = StringTk::uintToHexStr(pathInfo->getOrigUID() ); // uid-string

         std::string timeStamp;
         StringTk::timeStampFromEntryID(pathInfo->getOrigParentEntryID(), timeStamp);

         std::string dayChar;
         std::string yearMonthStr;
         timeStampToPath(timeStamp, yearMonthStr, dayChar);

         /* Note: Do not forget to update STORAGETK_CHUNKDIR_VEC_SIZE if you change the number of
          *       appends. */

         outPath.appendElem(CONFIG_CHUNK_UID_PREFIX + uidStr);
         outPath.appendElem(yearMonthStr);
         outPath.appendElem(dayChar);
         outPath.appendElem(pathInfo->getOrigParentEntryID() );
      }


      /**
       * @return path/hashDir1/hashDir2/fileName
       */
      static std::string getHashPath(const std::string path, const std::string entryID,
         size_t numHashesLevel1, size_t numHashesLevel2)
      {
         return path + "/" + getBaseHashPath(entryID, numHashesLevel1, numHashesLevel2);
      }

      /**
       * @return hashDir1/hashDir2/entryID
       */
      static std::string getBaseHashPath(const std::string entryID,
         size_t numHashesLevel1, size_t numHashesLevel2)
      {
         uint16_t hashLevel1;
         uint16_t hashLevel2;

         getHashes(entryID, numHashesLevel1, numHashesLevel2, hashLevel1, hashLevel2);

         return StringTk::uint16ToHexStr(hashLevel1) + "/" + StringTk::uint16ToHexStr(hashLevel2) +
            "/" + entryID;
      }

      static void splitHashDirs(unsigned hashDirs, unsigned* firstLevelHashDirOut,
         unsigned* secondLevelHashDirOut)
      {
         // from the incoming hash dir, we create a first level hash dir and a second level hash
         // dir, by splitting the 32-bit integer into two halfs
         *firstLevelHashDirOut = hashDirs >> 16;
         *secondLevelHashDirOut = hashDirs & 65535; // cut of at 16-bit
      }

      static unsigned mergeHashDirs(unsigned firstLevelhashDirNum, unsigned secondLevelHashDirNum)
      {
         // 32 bit integer; first 16 bit for first level hash, second 16 bit for second level
         // this is enough space for the currently used hash dirs
         unsigned hashDirNum = firstLevelhashDirNum << 16; // add first level
         hashDirNum = hashDirNum + secondLevelHashDirNum; // add second level

         return hashDirNum;
      }


      /*
       * convert from a entry type given from struct dirent to a BeeGFS DirEntryType
       */
      static DirEntryType direntToDirEntryType(unsigned char d_type)
      {
         DirEntryType entryType;

         switch(d_type)
         {
            case DT_DIR:
               entryType = DirEntryType_DIRECTORY; break;
            case DT_REG:
               entryType = DirEntryType_REGULARFILE; break;
            case DT_BLK:
               entryType = DirEntryType_BLOCKDEV; break;
            case DT_CHR:
               entryType = DirEntryType_CHARDEV; break;
            case DT_FIFO:
               entryType = DirEntryType_FIFO; break;
            case DT_LNK:
               entryType = DirEntryType_SYMLINK; break;
            case DT_SOCK:
               entryType = DirEntryType_SOCKET; break;
            default:
               entryType = DirEntryType_INVALID; break;
         }

         return entryType;
      }

      /**
       * @param outFileExisted may be NULL, will be set to true if the file was actually created,
       *    false on error or if it already existed before.
       */
      static bool createFile(std::string path, int* outErrno = NULL, bool* outFileCreated = NULL)
      {
         SAFE_ASSIGN(outFileCreated, false);

         int fd = open(path.c_str(), O_CREAT | O_EXCL, S_IRWXU | S_IRGRP | S_IWGRP | S_IROTH);

         if(fd == -1)
         { // error
            SAFE_ASSIGN(outErrno, errno);

            if(errno == EEXIST) // exists is no error
               return true;

            return false;
         }

         close(fd);

         SAFE_ASSIGN(outFileCreated, true);

         return true;
      }

};

#endif /*STORAGETK_H_*/

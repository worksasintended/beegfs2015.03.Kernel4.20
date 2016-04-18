#ifndef IOCTLTK_H_
#define IOCTLTK_H_

#include <common/system/System.h>
#include <common/Common.h>
#include <beegfs/beegfs.h>


struct BeegfsIoctl_MkFile_Arg; // forward declaration


class IoctlTk
{
   public:
      /**
       * Note: Use isFDValid() afterwards to check if open() was successful.
       */
      IoctlTk(std::string path)
      {
         this->fd = open(path.c_str(), O_RDONLY );
         if (this->fd < 0)
            this->errorMsg = "Failed to open " + path + ": " + System::getErrString(errno);
      }

      IoctlTk(int fd)
      {
         if (fd < 0)
         {
            this->errorMsg = "Invalid file descriptor given";
            this->fd = -1;
            return;
         }

         this->fd = dup(fd);
         if(this->fd < 0)
         {
            this->errorMsg = "Duplicating the file descriptor failed: " +
               System::getErrString(errno);
         }
      }

      ~IoctlTk(void)
      {
         if (this->fd >= 0)
            close(this->fd);
      }

      bool getMountID(std::string* outMountID);
      bool getCfgFile(std::string* outCfgFile);
      bool getRuntimeCfgFile(std::string* outCfgFile);
      bool createFile(BeegfsIoctl_MkFile_Arg* fileData);
      bool testIsFhGFS(void);
      bool getStripeInfo(unsigned* outPatternType, unsigned* outChunkSize, uint16_t* outNumTargets);
      bool getStripeTarget(uint16_t targetIndex, uint16_t* outTargetNumID, uint16_t* outNodeNumID,
         std::string* outNodeStrID);
      bool mkFileWithStripeHints(std::string filename, mode_t mode, unsigned numtargets,
         unsigned chunksize);


   private:
      int fd; // file descriptor
      std::string errorMsg; // if something fails


   public:
      // inliners

      inline bool isFDValid(void)
      {
         if (this->fd < 0)
            return false;

         return true;
      }

      /**
       * Print error message of failures.
       */
      inline void printErrMsg(void)
      {
         std::cerr << this->errorMsg << std::endl;
      }

      /**
       * Get last error message.
       */
      std::string getErrMsg()
      {
         return this->errorMsg;
      }
};


/* used to pass information about file creation through an ioctl.
   note: this interface is too complicated for normal users, that's why we don't have it in the
   public beegfs_ioctl.h */
struct BeegfsIoctl_MkFile_Arg
{
   uint16_t ownerNodeID; // owner node of the parent dir

   const char* parentParentEntryID; // entryID of the parent of the parent (=> grandparent entryID)
   int parentParentEntryIDLen;

   const char* parentEntryID; // entryID of the parent
   int parentEntryIDLen;

   const char* parentName;   // name of parent dir
   int parentNameLen;

   // file information
   const char* entryName; // file name we want to create
   int entryNameLen;
   int fileType; // see linux/fs.h or man 3 readdir, DT_UNKNOWN, DT_FIFO, ...

   const char* symlinkTo;  // Only must be set for symlinks.
   int symlinkToLen; // Length of the symlink name

   int mode; // mode (permission) of the new file

   // only will be used if the current user is root
   uid_t uid; // user ID
   uid_t gid; // group ID

   int numTargets;     // number of targets in prefTargets array (without final 0 element)
   uint16_t* prefTargets;  // array of preferred targets (additional last element must be 0)
   int prefTargetsLen; // raw byte length of prefTargets array (including final 0 element)
};


#endif /* IOCTLTK_H_ */

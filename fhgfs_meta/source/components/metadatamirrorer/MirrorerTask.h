#ifndef MIRRORERTASK_H_
#define MIRRORERTASK_H_

#include <common/storage/StorageErrors.h>
#include <common/Common.h>
#include <storage/MetadataEx.h>
#include <storage/FileInode.h>


class MirrorerTask; // forward declaration for MirrorerTaskList

typedef std::list<MirrorerTask*> MirrorerTaskList; // list of mirrorer tasks
typedef MirrorerTaskList::iterator MirrorerTaskListIter;
typedef MirrorerTaskList::const_iterator MirrorerTaskListConstIter;


enum MirrorerTaskType
{
   MirrorerTaskType_INVALID            = -1,

   MirrorerTaskType_NEWDENTRY          = 0,
   MirrorerTaskType_NEWINODE           = 1,

   MirrorerTaskType_REMOVEDENTRY       = 2,
   MirrorerTaskType_REMOVEINODE        = 3,

   MirrorerTaskType_RENAMEINSAMEDIR    = 4,

   MirrorerTaskType_LINKINSAMEDIR      = 5,
};


/**
 * Tasks for MetadataMirrorer. Contains serializable data for mirroring tasks (i.e.
 * the different mirroring operation types that can be handled by the MetadataMirrorer) and the
 * methods to execute the tasks on the mirror node.
 *
 * Note: The copy constructor of this class if private to avoid accidental usage. The problem is
 * that fileBuf would need to be memcpy()ed (which is inefficient) when a MirrorerTask object is
 * added to one of the STL collection class objects. That's why we only use MirrorerTask pointers
 * with collection classes.
 */
class MirrorerTask
{
   public:
      /**
       * Does not perform any useful initialization. Call one of the specialized init... methods
       * and setMirroredFromNodeID() afterwards for initialization.
       */
      MirrorerTask()
      {
         this->taskType = MirrorerTaskType_INVALID;

         this->cachedSerialLen = 0;

         this->mirroredFromNodeID = 0;

         this->fileBuf = NULL;
         this->fileBufLen = 0;
         this->recvFileBuf = NULL;
         this->recvFileBufLen = 0;
      }

      ~MirrorerTask()
      {
         SAFE_FREE(fileBuf);
      }

      unsigned serialize(char* buf);
      bool deserialize(const char* buf, size_t bufLen, unsigned* outLen);

      FhgfsOpsErr executeTask();


   private:

      MirrorerTask(const MirrorerTask& other)
      {
         // private to avoid accidental usage (see class comments for reason).
      }

      MirrorerTask& operator=(const MirrorerTask& rhs)
      {
         // private to avoid accidental usage (see class comments for reason).
         return *this;
      }


      MirrorerTaskType taskType;

      unsigned cachedSerialLen; // cached len from calcSerialLen to avoid double calculations

      std::string parentDirID;
      std::string entryName;
      std::string newEntryName; // for rename() (and this->entryName is the old name in this case)
      std::string entryID;
      bool hasEntryIDHardlink;
      bool hasContentsDir;
      uint16_t mirroredFromNodeID;

      char* fileBuf; // for contents of metadata file; alloc'ed on demand, free'd in destructor)
      unsigned fileBufLen; // length of fileBuf
      const char* recvFileBuf; // points to received msg buf (=> no freeing)
      unsigned recvFileBufLen; // length of recvFileBuf

      unsigned calcSerialLen();

      bool loadFileBufIfNeeded();
      bool loadFileBufFromXattr(std::string path, const char* xattrName);
      bool loadFileBufFromContents(std::string path);

      FhgfsOpsErr saveRecvFileBuf(std::string path);
      FhgfsOpsErr linkFile(std::string fromPath, std::string toPath);
      FhgfsOpsErr unlinkFile(std::string path);
      FhgfsOpsErr removeContentsDir(std::string path);
      FhgfsOpsErr renameFile(std::string fromPath, std::string toPath);

   public:
      // getters & setters

      void setMirroredFromNodeID(uint16_t mirroredFromNodeID)
      {
         this->mirroredFromNodeID = mirroredFromNodeID;
      }

      // inliners

      unsigned serialLen()
      {
         if(!cachedSerialLen)
            cachedSerialLen = calcSerialLen();

         return cachedSerialLen;
      }

      /**
       * @param entryID leave empty if only a dentry name file should be created (e.g. for dir
       * dentries).
       * @return false on error (don't use task in case of init error)
       */
      bool initAddNewDentry(std::string parentDirID, std::string entryName, std::string entryID,
         bool hasEntryIDHardlink)
      {
         this->taskType = MirrorerTaskType_NEWDENTRY;

         this->parentDirID = parentDirID;
         this->entryName = entryName;
         this->entryID = entryID;
         this->hasEntryIDHardlink = hasEntryIDHardlink;

         // load fileBuf if required for this type of task

         bool loadRes = loadFileBufIfNeeded();
         if(unlikely(!loadRes) )
            return false; // unable to load file

         return true;
      }

      /**
       * @return false on error (don't use task in case of init error)
       */
      bool initAddNewInode(std::string entryID)
      {
         this->taskType = MirrorerTaskType_NEWINODE;

         this->entryID = entryID;

         // load fileBuf if required for this type of task

         bool loadRes = loadFileBufIfNeeded();
         if(unlikely(!loadRes) )
            return false; // unable to load file

         return true;
      }

      /**
       * @param entryName may be empty if only entryID link should be deleted and not the name
       * dentry file (e.g. after the dentry name file was overwritten by a user's move operation).
       * @param entryID may be empty if only entryName link should be deleted and not the ID
       * dentry file (e.g. for dirs that don't have inlined inodes).
       * @return false on error (don't use task in case of init error)
       */
      bool initRemoveDentry(std::string parentDirID, std::string entryName, std::string entryID)
      {
         this->taskType = MirrorerTaskType_REMOVEDENTRY;

         this->parentDirID = parentDirID;
         this->entryName = entryName;
         this->entryID = entryID;

         // load fileBuf if required for this type of task

         bool loadRes = loadFileBufIfNeeded();
         if(unlikely(!loadRes) )
            return false; // unable to load file

         return true;
      }

      /**
       * @return false on error (don't use task in case of init error)
       */
      bool initRemoveInode(std::string entryID, bool hasContentsDir)
      {
         this->taskType = MirrorerTaskType_REMOVEINODE;

         this->entryID = entryID;
         this->hasContentsDir = hasContentsDir;

         // load fileBuf if required for this type of task

         bool loadRes = loadFileBufIfNeeded();
         if(unlikely(!loadRes) )
            return false; // unable to load file

         return true;
      }

      /**
       * @return false on error (don't use task in case of init error)
       */
      bool initRenameDentryInSameDir(std::string parentDirID, std::string oldEntryName,
         std::string newEntryName)
      {
         this->taskType = MirrorerTaskType_RENAMEINSAMEDIR;

         this->parentDirID = parentDirID;
         this->entryName = oldEntryName;
         this->newEntryName = newEntryName;

         // load fileBuf if required for this type of task

         bool loadRes = loadFileBufIfNeeded();
         if(unlikely(!loadRes) )
            return false; // unable to load file

         return true;
      }

      /**
       * @return false on error (don't use task in case of init error)
       */
      bool initHardLinkDentryInSameDir(std::string parentDirID, std::string fromName,
         FileInode* fromInode, std::string toName)
      {
         this->taskType = MirrorerTaskType_LINKINSAMEDIR;

         this->parentDirID  = parentDirID;
         this->entryName    = fromName;
         this->newEntryName = toName;

         /* note: No need to use loadFileBufIfNeeded(), which would load from disk, we already
          *       have the inode and can easily use the in-memory object. */
         this->fileBuf = (char*)malloc(META_SERBUF_SIZE);
         this->fileBufLen = fromInode->serializeMetaData(fileBuf);

         return true;
      }


};




#endif /* MIRRORERTASK_H_ */

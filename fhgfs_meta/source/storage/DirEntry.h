#ifndef DIRENTRY_H_
#define DIRENTRY_H_

#include <common/storage/Metadata.h>
#include <common/storage/StorageDefinitions.h>
#include <common/storage/StorageErrors.h>
#include <common/Common.h>
#include <toolkit/StorageTkEx.h>
#include "DentryStoreData.h"
#include "DiskMetaData.h"
#include "MetadataEx.h"
#include "FileInodeStoreData.h"


#define DIRENTRY_LOG_CONTEXT "DirEntry "

#define DirEntry_UNLINK_ID          1
#define DirEntry_UNLINK_FILENAME    2
#define DirEntry_UNLINK_ID_AND_FILENAME (DirEntry_UNLINK_ID | DirEntry_UNLINK_FILENAME)

/*
 * Class for directory entries (aka "dentries", formerly also referred to as "links"), which
 * contains the filename and information about where to find the inode (e.g. for remote dir
 * inodes).
 *
 * Note on locking: In contrast to files/dirs, dentries are not referenced. Every caller/thread
 * gets its own copy to work with, so dentry instances are not even shared. That's why we don't
 * have a mutex here.
 */
class DirEntry
{
   friend class MetaStore;
   friend class DirEntryStore;
   friend class FileInode;
   friend class GenericDebugMsgEx;
   friend class RecreateDentriesMsgEx;

   public:

      DirEntry(DirEntryType entryType, std::string name, std::string entryID,
         uint16_t ownerNodeID) : dentryDiskData(entryID, entryType, ownerNodeID, 0), name(name)
      {
      }

      /**
       * Note: This constructor does not perform initialization, so use it for
       * metadata loading only.
       */
      DirEntry(std::string entryName) : name(entryName)
      {
         // this->name = entryName; // set in initializer list
      }
      
      static DirEntry* createFromFile(std::string path, std::string entryName);
      static DirEntryType loadEntryTypeFromFile(std::string path, std::string entryName);

   protected:

      bool loadFromID(std::string dirEntryPath, std::string entryID);

   private:

      DentryStoreData dentryDiskData; // data stored on disk

      FileInodeStoreData inodeData;

      std::string name; // the user-friendly name, note: not set on reading entries anymore

      FhgfsOpsErr storeInitialDirEntryID(const char* logContext, const std::string& path);
      static FhgfsOpsErr storeInitialDirEntryName(const char* logContext, const std::string& idPath,
         const std::string& namePath, bool isDir);
      bool storeUpdatedDirEntryBuf(std::string idStorePath, char* buf, unsigned bufLen);
      bool storeUpdatedDirEntryBufAsXAttr(std::string idStorePath, char* buf, unsigned bufLen);
      bool storeUpdatedDirEntryBufAsContents(std::string idStorePath, char* buf,
         unsigned bufLen);
      bool storeUpdatedDirEntry(std::string dirEntryPath, bool useDirEntryName = false);
      FhgfsOpsErr storeUpdatedInode(std::string dirEntryPath);

      static FhgfsOpsErr removeDirEntryName(const char* logContext, std::string filePath);
      FhgfsOpsErr removeBusyFile(std::string dirEntryBasePath, std::string entryID,
         std::string entryName, unsigned unlinkTypeFlags);

      FileInode* createInodeByID(std::string dirEntryPath, EntryInfo* entryInfo);

      bool loadFromFileName(std::string path, std::string entryName);
      bool loadFromFile(std::string path);
      bool loadFromFileXAttr(std::string path);
      bool loadFromFileContents(std::string path);
      
      static DirEntryType loadEntryTypeFromFileXAttr(const std::string& path,
         const std::string& entryName);
      static DirEntryType loadEntryTypeFromFileContents(const std::string& path,
         const std::string& entryName);

      FhgfsOpsErr storeInitialDirEntry(std::string dirEntryPath);

   public:

      // inliners

      /**
       * Remove the dirEntrID file.
       */
      static FhgfsOpsErr removeDirEntryID(std::string dirEntryPath, std::string entryID)
      {
         const char* logContext = DIRENTRY_LOG_CONTEXT "(remove stored dirEntryID)";

         std::string idPath = MetaStorageTk::getMetaDirEntryIDPath(dirEntryPath) +  entryID;

         FhgfsOpsErr idUnlinkRes = removeDirEntryName(logContext, idPath);

         if (likely(idUnlinkRes == FhgfsOpsErr_SUCCESS) )
            LOG_DEBUG(logContext, 4, "Dir-Entry ID metadata deleted: " + idPath);

         return idUnlinkRes;
      }

      /**
       * Remove file dentries.
       *
       * If the argument is a directory the caller already must have checked if the directory
       * is empty.
       */
      static FhgfsOpsErr removeFileDentry(std::string dirEntryPath, std::string entryID,
         std::string entryName, unsigned unlinkTypeFlags)
      {
         const char* logContext = DIRENTRY_LOG_CONTEXT "(remove stored file dentry)";
         FhgfsOpsErr retVal;

         // first we delete entry-by-name and use this retVal as return code
         if (unlinkTypeFlags & DirEntry_UNLINK_FILENAME)
         {
            std::string namePath = dirEntryPath + '/' + entryName;

            retVal = removeDirEntryName(logContext, namePath);

            if (retVal == FhgfsOpsErr_SUCCESS && (unlinkTypeFlags & DirEntry_UNLINK_ID) )
            {
               // once the dirEntry-by-name was successfully unlinked, unlink dirEntry-by-ID
               removeDirEntryID(dirEntryPath, entryID); // error code is ignored
            }
            else
            {
               /* We must not try to delete the ID file on  FhgfsOpsErr_NOTEXISTS, as during a race
                * (possible locking issue) the file may have been renamed and so the ID might be
                * still valid.
                */
            }

         }
         else
         if (unlinkTypeFlags & DirEntry_UNLINK_ID)
            retVal = removeDirEntryID(dirEntryPath, entryID);
         else
         {
            /* It might happen that the code was supposed to unlink an ID only, but if the inode
             * has a link count > 1, even the ID is not supposed to be unlinked. So unlink
             * is a no-op then. */
             retVal = FhgfsOpsErr_SUCCESS;
         }

         return retVal;
      }

      /**
       * Remove directory dentries.
       *
       * If the argument is a directory the caller already must have checked if the directory
       * is empty.
       */
      static FhgfsOpsErr removeDirDentry(std::string dirEntryPath, std::string entryName)
      {
         const char* logContext = DIRENTRY_LOG_CONTEXT "(remove stored directory dentry)";

         std::string namePath = dirEntryPath + '/' + entryName;

         FhgfsOpsErr retVal = removeDirEntryName(logContext, namePath);

         return retVal;
      }


      // getters & setters


      /**
       * Set a new ownerNodeID, used by fsck or generic debug message.
       */
      bool setOwnerNodeID(std::string dirEntryPath, uint16_t newOwner)
      {
         bool success = true;
         
         uint16_t oldOwner = this->getOwnerNodeID();
         this->dentryDiskData.setOwnerNodeID(newOwner);
         
         if(!storeUpdatedDirEntry(dirEntryPath, true) )
         { // failed to update metadata => restore old value
            this->dentryDiskData.setOwnerNodeID(oldOwner);
            
            success = false;
         }

         return success;
      }

      void setFileInodeData(FileInodeStoreData& inodeData)
      {
          this->inodeData = inodeData;

          unsigned updatedFlags = getDentryFeatureFlags() |
             (DENTRY_FEATURE_INODE_INLINE | DENTRY_FEATURE_IS_FILEINODE);

          setDentryFeatureFlags(updatedFlags);
      }

      void setMirrorNodeID(uint16_t mirrorNodeID)
      {
         this->dentryDiskData.setMirrorNodeID(mirrorNodeID);
      }

      uint16_t getMirrorNodeID()
      {
         return this->dentryDiskData.getMirrorNodeID();
      }

      unsigned getDentryFeatureFlags(void)
      {
         return this->dentryDiskData.getDentryFeatureFlags();
      }

      void setDentryFeatureFlags(unsigned featureFlags)
      {
         this->dentryDiskData.setDentryFeatureFlags(featureFlags);
      }

      // getters

      std::string getEntryID()
      {
         return this->dentryDiskData.getEntryID();

      }

      std::string getID()
      {
         return this->dentryDiskData.getEntryID();
      }

      /**
       * Note: Should not be changed after object init => not synchronized!
       */
      DirEntryType getEntryType()
      {
         return this->dentryDiskData.getDirEntryType();
      }

      std::string getName()
      {
         return this->name;
      }

      uint16_t getOwnerNodeID()
      {
         return this->dentryDiskData.getOwnerNodeID();
      }

      void getEntryInfo(std::string& parentEntryID, int flags, EntryInfo* outEntryInfo)
      {
         if (getIsInodeInlined() )
            flags |= ENTRYINFO_FEATURE_INLINED;

         outEntryInfo->set(getOwnerNodeID(), parentEntryID, getID(), name,
            getEntryType(), flags);
      }


      /**
       * Unset the DENTRY_FEATURE_INODE_INLINE flag
       */
      void unsetInodeInlined()
      {
         uint16_t dentryFeatureFlags = this->dentryDiskData.getDentryFeatureFlags();

         dentryFeatureFlags &= ~(DENTRY_FEATURE_INODE_INLINE);

         this->dentryDiskData.setDentryFeatureFlags(dentryFeatureFlags);
      }
      /**
       * Check if the inode is inlined and no flag is set to indicate the same object
       *  (file-as-hard-link) is also in the inode-hash directories.
       */
      bool getIsInodeInlined(void)
      {
         if (this->dentryDiskData.getDentryFeatureFlags() & DENTRY_FEATURE_INODE_INLINE)
            return true;

         return false;
      }

      unsigned serializeDentry(char* buf)
      {
         DiskMetaData diskMetaData(&this->dentryDiskData, &this->inodeData);
         return diskMetaData.serializeDentry(buf);
      }

      bool deserializeDentry(const char* buf)
      {
         DiskMetaData diskMetaData(&this->dentryDiskData, &this->inodeData);
         return diskMetaData.deserializeDentry(buf);
      }

   protected:

      FileInodeStoreData* getInodeStoreData(void)
      {
         return &this->inodeData;
      }


};


#endif /* DIRENTRY_H_*/

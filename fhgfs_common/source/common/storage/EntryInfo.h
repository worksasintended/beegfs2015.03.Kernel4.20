/*
 * class EntryInfo - required information to find an inode or chunk files
 *
 * NOTE: If you change this file, do not forget to adjust the client side EntryInfo.h
 */

#ifndef ENTRYINFO_H_
#define ENTRYINFO_H_

#include <common/storage/StorageDefinitions.h>
#include <common/storage/EntryOwnerInfo.h>


#define ENTRYINFO_FEATURE_INLINED      1 // indicate inlined inode, might be outdated

#define EntryInfo_PARENT_ID_UNKNOWN    "unknown"


class EntryInfo; // forward declaration


typedef std::list<EntryInfo> EntryInfoList;
typedef EntryInfoList::iterator EntryInfoListIter;
typedef EntryInfoList::const_iterator EntryInfoListConstIter;


/**
 * Information about a file/directory
 */
class EntryInfo
{
   public:
      EntryInfo()
      {
         this->ownerNodeID   = 0;
         this->entryType     = DirEntryType_INVALID;
         this->flags     = 0;
      };

      EntryInfo(uint16_t ownerNodeID, std::string parentEntryID, std::string entryID,
         std::string fileName, DirEntryType entryType, int statFlags) :
         parentEntryID(parentEntryID), entryID(entryID), fileName(fileName)
      {
         this->ownerNodeID   = ownerNodeID;
         //this->parentEntryID = parentEntryID; // set in initializer list
         //this->entryID       = entryID; // set in initializer list
         //this->fileName      = fileName; // set in intializer list
         this->entryType     = entryType;
         this->flags     = statFlags;
      }

      bool compare(const EntryInfo* compareInfo) const;

   private:
      uint16_t ownerNodeID; // nodeID of the metadata server that has the inode
      std::string parentEntryID; // entryID of the parent dir
      std::string entryID; // entryID of the actual entry
      std::string fileName; // file/dir name of the actual entry

      DirEntryType entryType;
      int flags; // additional stat flags (e.g. ENTRYINFO_FEATURE_INLINED)


   public:

      size_t serialize(char* outBuf) const;
      bool deserialize(const char* buf, size_t bufLen, unsigned* outLen);
      unsigned serialLen(void) const;


      // inliners

      void set(uint16_t ownerNodeID, std::string parentEntryID,
         std::string entryID, std::string fileName, DirEntryType entryType, int flags)
      {
         this->ownerNodeID   = ownerNodeID;
         this->parentEntryID = parentEntryID;
         this->entryID       = entryID;
         this->fileName      = fileName;
         this->entryType     = entryType;
         this->flags         = flags;
      }

      void setFromOwnerInfo(EntryOwnerInfo* entryOwnerInfo)
      {
         this->ownerNodeID   = entryOwnerInfo->getOwnerNodeID();
         this->parentEntryID = entryOwnerInfo->getParentEntryID();
         this->entryID       = entryOwnerInfo->getEntryID();
         this->fileName      = entryOwnerInfo->getFileName();
         this->entryType     = entryOwnerInfo->getEntryType();
         this->flags         = entryOwnerInfo->getFlags();
      }

      void set(EntryInfo *newEntryInfo)
      {
         this->ownerNodeID   = newEntryInfo->getOwnerNodeID();
         this->parentEntryID = newEntryInfo->getParentEntryID();
         this->entryID       = newEntryInfo->getEntryID();
         this->fileName      = newEntryInfo->getFileName();
         this->entryType     = newEntryInfo->getEntryType();
         this->flags         = newEntryInfo->getFlags();
      }

      void setParentEntryID(const std::string& newParentEntryID)
      {
         this->parentEntryID = newParentEntryID;
      }

      /**
       * Set or unset the ENTRYINFO_FEATURE_INLINED flag.
       */
      void setInodeInlinedFlag(bool isInlined)
      {
         if (isInlined)
            this->flags |= ENTRYINFO_FEATURE_INLINED;
         else
            this->flags &= ~(ENTRYINFO_FEATURE_INLINED);
      }

      /**
       * Get ownerInfo from entryInfo values
       */
      void getOwnerInfo(int depth, EntryOwnerInfo* outOwnerInfo)
      {
         outOwnerInfo->set(depth, this->ownerNodeID, this->parentEntryID, this->entryID,
            this->fileName, this->entryType, this->flags);
      }

      uint16_t getOwnerNodeID(void) const
      {
         return this->ownerNodeID;
      }

      std::string getParentEntryID(void)
      {
         return this->parentEntryID;
      }

      std::string getEntryID(void)
      {
         return this->entryID;
      }

      std::string getFileName(void)
      {
         return this->fileName;
      }

      DirEntryType getEntryType(void) const
      {
         return this->entryType;
      }

      int getFlags(void) const
      {
         return this->flags;
      }

      bool getIsInlined() const
      {
         return (this->flags & ENTRYINFO_FEATURE_INLINED) ? true : false;
      }
};

#endif /* ENTRYINFO_H_ */

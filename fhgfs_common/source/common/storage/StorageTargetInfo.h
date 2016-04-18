#ifndef STORAGETARGETINFO_H_
#define STORAGETARGETINFO_H_

#include <common/Common.h>
#include <common/nodes/Node.h>
#include <common/nodes/TargetStateInfo.h>
#include <common/storage/StorageErrors.h>

class StorageTargetInfo
{
   public:
      StorageTargetInfo(uint16_t targetID, const std::string& pathStr, int64_t diskSpaceTotal,
            int64_t diskSpaceFree, int64_t inodesTotal, int64_t inodesFree,
            TargetConsistencyState consistencyState)
         : targetID(targetID),
            pathStr(pathStr),
            diskSpaceTotal(diskSpaceTotal),
            diskSpaceFree(diskSpaceFree),
            inodesTotal(inodesTotal),
            inodesFree(inodesFree),
            consistencyState(consistencyState)
      { }

      /**
       * only for deserialization
       */
      StorageTargetInfo()
      { }

      size_t serialize(char* outBuf);
      bool deserialize(const char* buf, size_t bufLen, unsigned* outLen);
      unsigned serialLen();

      static FhgfsOpsErr statStoragePath(Node* node, uint16_t targetID, int64_t* outFree,
         int64_t* outTotal, int64_t* outInodesFree, int64_t* outInodesTotal);

   private:
      uint16_t targetID;
      std::string pathStr;
      int64_t diskSpaceTotal;
      int64_t diskSpaceFree;
      int64_t inodesTotal;
      int64_t inodesFree;
      TargetConsistencyState consistencyState;

   public:
      // getter/setter
      uint16_t getTargetID() const
      {
         return targetID;
      }

      const std::string getPathStr() const
      {
         return pathStr;
      }

      int64_t getDiskSpaceTotal() const
      {
         return diskSpaceTotal;
      }

      int64_t getDiskSpaceFree() const
      {
         return diskSpaceFree;
      }

      int64_t getInodesTotal() const
      {
         return inodesTotal;
      }

      int64_t getInodesFree() const
      {
         return inodesFree;
      }

      TargetConsistencyState getState() const
      {
         return consistencyState;
      }

      // operators
      bool operator<(const StorageTargetInfo& other)
       {
          if ( targetID < other.targetID )
             return true;
          else
             return false;
       }

       bool operator==(const StorageTargetInfo& other)
       {
          if ( targetID != other.targetID )
             return false;
          else
          if ( pathStr != other.pathStr )
             return false;
          else
          if ( diskSpaceTotal != other.diskSpaceTotal )
             return false;
          else
          if ( diskSpaceFree != other.diskSpaceFree )
             return false;
          else
          if ( inodesTotal != other.inodesTotal )
             return false;
          else
          if ( inodesFree != other.inodesFree )
             return false;
          else
          if ( consistencyState != other.consistencyState )
             return false;
          else
             return true;
       }

       bool operator!= (const StorageTargetInfo& other)
       {
          return !(operator==( other ) );
       }
};

typedef std::list<StorageTargetInfo> StorageTargetInfoList;
typedef StorageTargetInfoList::iterator StorageTargetInfoListIter;
typedef StorageTargetInfoList::const_iterator StorageTargetInfoListCIter;

#endif /* STORAGETARGETINFO_H_ */

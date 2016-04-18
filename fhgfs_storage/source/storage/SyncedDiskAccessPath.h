#ifndef SYNCEDDISKACCESSPATH_H_
#define SYNCEDDISKACCESSPATH_H_

// O B S O L E T E (superseded by SyncedStoragePaths)
// O B S O L E T E
// O B S O L E T E


//#include <common/threading/SafeMutexLock.h>
//#include <common/storage/Path.h>
//#include <common/system/System.h>
//#include <toolkit/StorageTkEx.h>
//
//
//class SyncedDiskAccessPath : public Path
//{
//   public:
//      SyncedDiskAccessPath() : Path()
//      {
//         initStorageVersion();
//      }
//
//      SyncedDiskAccessPath(std::string pathStr) : Path(pathStr)
//      {
//         initStorageVersion();
//      }
//
//
//   private:
//      uint64_t storageVersion; // zero is the invalid version!
//      uint64_t highVersion; // high part of storageVersion (high 42 bits)
//      unsigned int lowVersion; // low part of storageVersion (low 22 bits)
//
//      Mutex diskMutex;
//
//      // inliners
//      void initStorageVersion()
//      {
//         uint64_t currentSecs = System::getCurrentTimeSecs();
//         unsigned int minorVersion = 0;
//
//         setStorageVersion(currentSecs, minorVersion);
//      }
//
//      /**
//       * @param high currentTimeSecs (high 42 bits)
//       * @param low minorVersion (low 22 bits)
//       */
//      void setStorageVersion(uint64_t highVersion, unsigned int lowVersion)
//      {
//         this->highVersion = highVersion;
//         this->lowVersion = lowVersion;
//
//         this->storageVersion = (highVersion << 22);
//         this->storageVersion |= ( (lowVersion << 10) >> 10);
//      }
//
//   public:
//      // inliners
//
//      /**
//       * note: remember to call storageUpdateEnd()
//       * @return storageVersion of the current update
//       */
//      void storageUpdateBegin()
//      {
//         diskMutex.lock();
//      }
//
//      void storageUpdateEnd()
//      {
//         diskMutex.unlock();
//      }
//
//
//      /**
//       * Requires a call to storageUpdateBegin before! (and ...End() afterwards)
//       */
//      uint64_t incStorageVersion()
//      {
//         // note: this relies on the fact that the currentTimeSecs never ever decrease!
//
//         uint64_t nextHighVersion;
//         unsigned int nextLowVersion;
//
//         // we try to avoid the getTime sys-call for some minor versions
//         if(lowVersion < 4096)
//         {
//            nextHighVersion = this->highVersion; // remains unchanged
//            nextLowVersion = this->lowVersion+1;
//         }
//         else
//         {
//            // in this case, we have to check that the timeSecs really have increased
//            // before we reset the minor version
//            nextHighVersion = System::getCurrentTimeSecs();
//
//            if(nextHighVersion == this->highVersion)
//            { // no timeSecs increase yet => use the minor version
//               nextLowVersion = this->lowVersion+1;
//            }
//            else
//            { // high version increased => we can reset the minor version
//               nextLowVersion = 0;
//            }
//         }
//
//         setStorageVersion(nextHighVersion, nextLowVersion);
//
//         return storageVersion;
//      }
//
//      bool createSubPathOnDisk(Path& subpath, bool excludeLastElement)
//      {
//         storageUpdateBegin();
//
//         Path completePath(*this, subpath);
//
//         bool createRes = StorageTk::createPathOnDisk(completePath, excludeLastElement);
//
//         storageUpdateEnd();
//
//         return createRes;
//      }
//
//      bool removeSubPathDirsFromDisk(Path& subpath, unsigned subKeepDepth)
//      {
//         storageUpdateBegin();
//
//         Path completePath(*this, subpath);
//
//         bool removeRes = StorageTk::removePathDirsFromDisk(
//            completePath, subKeepDepth + getPathElems()->size() );
//
//         storageUpdateEnd();
//
//         return removeRes;
//      }
//
//};

#endif /*SYNCEDDISKACCESSPATH_H_*/

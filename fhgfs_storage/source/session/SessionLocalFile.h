#ifndef SESSIONLOCALFILE_H_
#define SESSIONLOCALFILE_H_

#include <common/storage/Path.h>
#include <common/threading/Mutex.h>
#include <common/threading/SafeRWLock.h>
#include <common/Common.h>


/**
 * Represents the client session information for an open chunk file.
 */
class SessionLocalFile
{
   public:
      /**
       * @param fileHandleID format: <ownerID>#<fileID>
       * @param openFlags system flags for open()
       * @param serverCrashed true if session was created after a server crash, mark session as
       * dirty
       */
      SessionLocalFile(std::string fileHandleID, uint16_t targetID, std::string fileID,
         int openFlags, bool serverCrashed) : fileHandleID(fileHandleID), targetID(targetID),
         fileID(fileID)
      {
         this->fileDescriptor = -1; // initialize as invalid file descriptor
         this->openFlags = openFlags;
         this->offset = -1; // initialize as invalid offset (will be set on file open)

         this->mirrorNode = NULL;
         this->isMirrorSession = false;

         this->removeOnRelease = false;

         this->writeCounter = 0;
         this->readCounter = 0;
         this->lastReadAheadTrigger = 0;

         this->dataHasChanged = false;

         this->serverCrashed = serverCrashed;
      }

      /**
       * For dezerialisation only
       */
      SessionLocalFile()
      {
         this->fileDescriptor = -1; // initialize as invalid file descriptor
         this->offset = -1; // initialize as invalid offset (will be set on file open)
         this->mirrorNode = NULL;
      }

      unsigned serialize(char* buf);
      bool deserialize(const char* buf, size_t bufLen, unsigned* outLen);
      unsigned serialLen();

      FhgfsOpsErr openFile(int targetFD, PathInfo *pathInfo, bool isWriteOpen,
         SessionQuotaInfo* quotaInfo);

      Node* setMirrorNodeExclusive(Node** mirrorNode);


   private:
      bool removeOnRelease; // remove on last ref drop (for internal use by the sessionstore only!)

      std::string fileHandleID;
      uint16_t targetID;
      std::string fileID;
      int openFlags; // system flags for open()

      int fileDescriptor; // file descriptor -1 for invalid (=not opened yet)
      int64_t offset; // negative value for unspecified/invalid offset
      
      Node* mirrorNode; // the node to which all writes should be mirrored
      bool isMirrorSession; // true if this is the mirror session of a file

      AtomicInt64 writeCounter; // how much sequential data we have written after open/sync_file_range
      AtomicInt64 readCounter; // how much sequential data we have read since open / last seek
      AtomicInt64 lastReadAheadTrigger; // last readCounter value at which read-ahead was triggered

      bool dataHasChanged; /* just indicates if data was written to the file (at the moment only
                              relevant for HSM */

      Mutex sessionMutex;

      bool serverCrashed; // true if session was created after a server crash, mark session as dirty

      FhgfsOpsErr fhgfsErrFromSysErr(int64_t errCode);


   public:

      // getters & setters
      bool getRemoveOnRelease()
      {
         return removeOnRelease;
      }

      void setRemoveOnRelease(bool removeOnRelease)
      {
         this->removeOnRelease = removeOnRelease;
      }

      std::string getFileHandleID() const
      {
         return fileHandleID;
      }

      uint16_t getTargetID() const
      {
         return targetID;
      }

      std::string getFileID() const
      {
         return fileID;
      }

      int getFD()
      {
         if (this->fileDescriptor != -1) // optimization: try without a lock first
            return this->fileDescriptor;

         SafeMutexLock safeMutex(&this->sessionMutex); // L O C K

         int fd = this->fileDescriptor;

         safeMutex.unlock(); // U N L O C K

         return fd;
      }
      
      void setFDUnlocked(int fd)
      {
         this->fileDescriptor = fd;
      }

      int getOpenFlags() const
      {
         return openFlags;
      }

      bool getIsDirectIO() const
      {
         return (this->openFlags & (O_DIRECT | O_SYNC) ) != 0;
      }

      int64_t getOffset()
      {
         SafeMutexLock safeMutex(&this->sessionMutex); // L O C K

         int64_t offset = this->offset;

         safeMutex.unlock(); // U N L O C K

         return offset;
      }

      void setOffset(int64_t offset)
      {
         SafeMutexLock safeMutex(&this->sessionMutex); // L O C K

         setOffsetUnlocked(offset);

         safeMutex.unlock(); // U N L O C K
      }

      void setOffsetUnlocked(int64_t offset)
      {
         this->offset = offset;
      }

      Node* getMirrorNode() const
      {
         return mirrorNode;
      }

      bool getIsMirrorSession() const
      {
         return isMirrorSession;
      }

      void setIsMirrorSession(bool isMirrorSession)
      {
         this->isMirrorSession = isMirrorSession;
      }

      void resetWriteCounter()
      {
         this->writeCounter = 0;
      }

      void incWriteCounter(int64_t size)
      {
         this->writeCounter.increase(size);
      }

      int64_t getWriteCounter()
      {
         return this->writeCounter.read();
      }

      int64_t getReadCounter()
      {
         return this->readCounter.read();
      }

      void resetReadCounter()
      {
         this->readCounter.setZero();
      }

      void incReadCounter(int64_t size)
      {
         this->readCounter.increase(size);
      }

      int64_t getLastReadAheadTrigger()
      {
         return this->lastReadAheadTrigger.read();
      }

      void resetLastReadAheadTrigger()
      {
         this->lastReadAheadTrigger.setZero();
      }

      void setLastReadAheadTrigger(int64_t lastReadAheadTrigger)
      {
         this->lastReadAheadTrigger.set(lastReadAheadTrigger);
      }

      void setDataHasChanged(bool value)
      {
         SafeMutexLock safeMutex(&this->sessionMutex); // L O C K

         this->dataHasChanged = value;

         safeMutex.unlock(); // U N L O C K
      }

      bool getDataHasChanged()
      {
         SafeMutexLock safeMutex(&this->sessionMutex); // L O C K

         bool dataHasChanged = this->dataHasChanged;

         safeMutex.unlock(); // U N L O C K

         return dataHasChanged;
      }

      bool isServerCrashed()
      {
         return this->serverCrashed;
      }
};

#endif /*SESSIONLOCALFILE_H_*/

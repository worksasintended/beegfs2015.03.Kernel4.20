#ifndef DATAFETCHERBUFFER_H_
#define DATAFETCHERBUFFER_H_

#include <common/app/log/LogContext.h>
#include <common/fsck/FsckChunk.h>
#include <common/fsck/FsckContDir.h>
#include <common/fsck/FsckDirEntry.h>
#include <common/fsck/FsckDirInode.h>
#include <common/fsck/FsckFileInode.h>
#include <common/fsck/FsckTargetID.h>
#include <common/threading/SafeMutexLock.h>
#include <database/FsckDB.h>
#include <toolkit/FsckDefinitions.h>

#include <program/Program.h>

template <typename T>
class DataFetcherBuffer
{
   public:
      DataFetcherBuffer()
      {
         this->database = Program::getApp()->getDatabase();
         log.setContext("DataFetcherBuffer");
      }

      virtual ~DataFetcherBuffer() { }

      /*
       * @param elementList
       * @param uniqueElements true if elements in buffer should be unique; this is useful for used
       * targetIDs, where we expect to have a lot of constraint errors; the will be ignored by the
       * database, but nevertheless a "pre-filtering" makes it easier for the DB
       * @return the size of the buffer after inserting
       */
      size_t add(std::list<T>& elementList, bool uniqueElements = false)
      {
         size_t retVal;

         SafeMutexLock mutexLock(&mutex);

         if ( uniqueElements )
         {
            typename std::list<T>::iterator iter;
            for ( iter = elementList.begin(); iter != elementList.end(); iter++ )
            {
               if ( std::find(this->elements.begin(), this->elements.end(), *iter)
                  == this->elements.end() )
               {
                  this->elements.push_back(*iter);
               }
            }
         }
         else
         {
            this->elements.insert(this->elements.end(), elementList.begin(), elementList.end());
         }

         retVal = this->elements.size();

         mutexLock.unlock();

         return retVal;
      }

      bool flush(T& failedInsert, int& errorCode)
      {
         bool insertRes = true;

         SafeMutexLock mutexLock(&mutex);

         if (!this->elements.empty())
         {
            insertRes = this->insertIntoDB(this->elements, failedInsert, errorCode);
            this->elements.clear();
         }

         mutexLock.unlock();

         return insertRes;
      }

      size_t getSize()
      {
         size_t retVal;

         SafeMutexLock mutexLock(&mutex);

         retVal = this->elements.size();

         mutexLock.unlock();

         return retVal;
      }

   private:
      FsckDB* database;
      LogContext log;

      std::list<T> elements;
      Mutex mutex;

      bool insertIntoDB(FsckDirEntryList& elements, FsckDirEntry& failedInsert,
         int& errorCode)
      {
         return this->database->insertDirEntries(elements, failedInsert, errorCode);
      }

      bool insertIntoDB(FsckFileInodeList &elements, FsckFileInode& failedInsert,
         int& errorCode)
      {
         return this->database->insertFileInodes(elements, failedInsert, errorCode);
      }

      bool insertIntoDB(FsckDirInodeList &elements, FsckDirInode& failedInsert,
         int& errorCode)
      {
         return this->database->insertDirInodes(elements, failedInsert, errorCode);
      }

      bool insertIntoDB(FsckChunkList &elements, FsckChunk& failedInsert,
         int& errorCode)
      {
         return this->database->insertChunks(elements, failedInsert, errorCode);
      }

      bool insertIntoDB(FsckContDirList &elements, FsckContDir& failedInsert,
         int& errorCode)
      {
         return this->database->insertContDirs(elements, failedInsert, errorCode);
      }

      bool insertIntoDB(FsckFsIDList &elements, FsckFsID& failedInsert,
         int& errorCode)
      {
         return this->database->insertFsIDs(elements, failedInsert, errorCode);
      }

      bool insertIntoDB(FsckTargetIDList &elements, FsckTargetID& failedInsert,
         int& errorCode)
      {
         return this->database->insertUsedTargetIDs(elements, failedInsert, errorCode);
      }
};

#endif /* DATAFETCHERBUFFER_H_ */

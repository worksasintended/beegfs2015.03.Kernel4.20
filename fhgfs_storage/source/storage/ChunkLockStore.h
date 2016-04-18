#ifndef CHUNKLOCKSTORE_H_
#define CHUNKLOCKSTORE_H_

#include <common/Common.h>


class ChunkLockStoreContents
{
   public:
      StringSet lockedChunks;
      Mutex lockedChunksMutex;
      Condition chunkUnlockedCondition;
};


typedef std::map<uint16_t, ChunkLockStoreContents> ChunkLockStoreMap;
typedef ChunkLockStoreMap::iterator ChunkLockStoreMapIter;
typedef ChunkLockStoreMap::value_type ChunkLockStoreMapVal;


/**
 * Can be used to lock access to a chunk, especially needed for resync; locks will be on a very
 * high, abstract level, as we do only restrict access by chunk ID.
 *
 * "locking" here means we successfully insert an element into a set. If we cannot insert, because
 * an element with the same ID is already present, it means someone else currently holds the lock.
 */
class ChunkLockStore
{
   friend class GenericDebugMsgEx;

   public:
      ChunkLockStore() { };

   private:
      ChunkLockStoreMap targetsMap;
      RWLock targetsLock; // synchronizes insertion into targetsMap

   public:
      void lockChunk(uint16_t targetID, std::string chunkID)
      {
         targetsLock.readLock(); // lock targets map

         ChunkLockStoreMapIter targetsIter = targetsMap.find(targetID);
         if(targetsIter == targetsMap.end() )
         { // slow path: get write lock and insert
            targetsLock.unlock();
            targetsLock.writeLock();

            targetsIter = targetsMap.insert(
               ChunkLockStoreMapVal(targetID, ChunkLockStoreContents() ) ).first;
         }

         ChunkLockStoreContents* targetLockStore = &targetsIter->second;

         SafeMutexLock chunksLock(&targetLockStore->lockedChunksMutex);

         // loop until we can insert the chunk lock
         for( ; ; )
         {
            bool insertRes = targetLockStore->lockedChunks.insert(chunkID).second;

            if(insertRes)
               break; // new lock successfully inserted

            // chunk lock already exists => wait
            targetLockStore->chunkUnlockedCondition.wait(&targetLockStore->lockedChunksMutex);
         }

         chunksLock.unlock();

         targetsLock.unlock(); // unlock targets map
      }

      void unlockChunk(uint16_t targetID, std::string chunkID)
      {
         ChunkLockStoreContents* targetLockStore;
         StringSetIter lockChunksIter;

         targetsLock.readLock(); // lock targets map

         ChunkLockStoreMapIter targetsIter = targetsMap.find(targetID);
         if(unlikely(targetsIter == targetsMap.end() ) )
         { // target not found should never happen here
            LogContext(__func__).log(Log_WARNING,
               "Tried to unlock chunk, but target not found in lock map. Printing backtrace. "
               "targetID: " + StringTk::uintToStr(targetID) );
            LogContext(__func__).logBacktrace();

            goto unlock_targets;
         }

         targetLockStore = &targetsIter->second;

         targetLockStore->lockedChunksMutex.lock();

         lockChunksIter = targetLockStore->lockedChunks.find(chunkID);
         if(unlikely(lockChunksIter == targetLockStore->lockedChunks.end() ) )
         {
            LogContext(__func__).log(Log_WARNING,
               "Tried to unlock chunk, but chunk not found in lock set. Printing backtrace. "
               "targetID: " + StringTk::uintToStr(targetID) + "; "
               "chunkID: " + chunkID);
            LogContext(__func__).logBacktrace();

            goto unlock_chunks;
         }

         targetLockStore->lockedChunks.erase(lockChunksIter);

         targetLockStore->chunkUnlockedCondition.broadcast(); // notify lock waiters

         unlock_chunks:
            targetLockStore->lockedChunksMutex.unlock();

         unlock_targets:
            targetsLock.unlock(); // unlock targets map
      }

   protected:
      // only used for GenericDebugMsg at the moment
      size_t getSize(uint16_t targetID)
      {
         size_t retVal = 0;

         targetsLock.readLock(); // lock targets map

         ChunkLockStoreMapIter targetsIter = targetsMap.find(targetID);
         if(targetsIter != targetsMap.end() ) // map does exist
         {
            ChunkLockStoreContents* targetLockStore = &targetsIter->second;

            SafeMutexLock chunksLock(&targetLockStore->lockedChunksMutex);

            retVal = targetLockStore->lockedChunks.size();

            chunksLock.unlock();
         }

         targetsLock.unlock(); // unlock targets map

         return retVal;
      }

      StringSet getLockStoreCopy(uint16_t targetID)
      {
         StringSet outLockStore;

         targetsLock.readLock(); // lock targets map

         ChunkLockStoreMapIter targetsIter = targetsMap.find(targetID);
         if(targetsIter != targetsMap.end() ) // map does exist
         {
            ChunkLockStoreContents* targetLockStore = &targetsIter->second;

            SafeMutexLock chunksLock(&targetLockStore->lockedChunksMutex);

            outLockStore = targetLockStore->lockedChunks;

            chunksLock.unlock();
         }

         targetsLock.unlock(); // unlock targets map

         return outLockStore;
      }
};

#endif /*CHUNKLOCKSTORE_H_*/

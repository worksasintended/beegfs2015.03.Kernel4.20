#ifndef HSMFILEMETADATA_H
#define HSMFILEMETADATA_H

#include <common/storage/StorageDefinitions.h>
#include <common/Common.h>

typedef uint16_t HsmCollocationID;
typedef std::vector<HsmCollocationID> HsmCollocationIDVector;
typedef HsmCollocationIDVector::iterator  HsmCollocationIDVectorIter;
typedef std::list<HsmCollocationID> HsmCollocationIDList;
typedef HsmCollocationIDList::iterator HsmCollocationIDListIter;

// only for compatibility... will be deleted
typedef HsmCollocationID GamCollocationID;
typedef HsmCollocationIDVector GamCollocationIDVector;
typedef HsmCollocationIDVectorIter GamCollocationIDVectorIter;
typedef HsmCollocationIDList GamCollocationIDList;
typedef HsmCollocationIDListIter GamCollocationIDListIter;

class HsmFileMetaData
{
   public:
      HsmFileMetaData()
      {
         this->collocationID = HSM_COLLOCATIONID_DEFAULT;
         this->offlineChunkCount = 0;
      }

      HsmFileMetaData(HsmCollocationID collocationID)
      {
         this->collocationID = collocationID;
         this->offlineChunkCount = 0;
      }

      HsmFileMetaData(HsmCollocationID collocationID, uint16_t offlineChunkCount)
      {
         this->collocationID = collocationID;
         this->offlineChunkCount = offlineChunkCount;
      }

      size_t serialize(char* outBuf);
      bool deserialize(const char* buf, size_t bufLen, unsigned* outLen);
      unsigned serialLen();

   private:
      HsmCollocationID collocationID; // collocation ID for HSM
      uint16_t offlineChunkCount; // counter for the chunks of a file which are offline in HSM

   public:
      HsmCollocationID getCollocationID() const
      {
         return this->collocationID;
      }

      void setCollocationID(HsmCollocationID collocationID)
      {
         this->collocationID = collocationID;
      }

      uint16_t getOfflineChunkCount() const
      {
         return this->offlineChunkCount;
      }

      void setOfflineChunkCount(uint16_t offlineChunkCount)
      {
         this->offlineChunkCount = offlineChunkCount;
      }

      void incOfflineChunkCount()
      {
         this->offlineChunkCount++;
      }

      void decOfflineChunkCount()
      {
         this->offlineChunkCount--;
      }

      bool hasDefaultValues()
      {
         if ( (! this->offlineChunkCount) && (this->collocationID == HSM_COLLOCATIONID_DEFAULT) )
            return true;
         else
            return false;
      }
};

#endif /*HSMFILEMETADATA_H*/

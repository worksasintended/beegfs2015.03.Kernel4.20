#ifndef CHUNKSYNCCANDIDATEDIR_H
#define CHUNKSYNCCANDIDATEDIR_H

#include <common/Common.h>

class ChunkSyncCandidateDir
{
   public:
      ChunkSyncCandidateDir(std::string relativePath, uint16_t targetID) :
         relativePath(relativePath), targetID(targetID)
      {
         // all initialization done in initializer list
      }

      // copy constructor
      ChunkSyncCandidateDir(const ChunkSyncCandidateDir& syncCandidate) :
         relativePath(syncCandidate.relativePath), targetID(syncCandidate.targetID)
      {
         // all initialization done in initializer list
      }

      ChunkSyncCandidateDir(): relativePath(""), targetID(0)
      {
         // all initialization done in initializer list
      }

   private:
      std::string relativePath;
      uint16_t targetID;

   public:
      std::string getRelativePath() const
      {
         return relativePath;
      }

      uint16_t getTargetID() const
      {
         return targetID;
      }

      void setRelativePath(const std::string relativePath)
      {
         this->relativePath = relativePath;
      }

      void setTargetID(uint16_t targetID)
      {
         this->targetID = targetID;
      }
};

typedef std::list<ChunkSyncCandidateDir> ChunkSyncCandidateDirList;
typedef ChunkSyncCandidateDirList::iterator ChunkSyncCandidateDirListIter;

#endif /* CHUNKSYNCCANDIDATEDIR_H */

#ifndef CHUNKSYNCCANDIDATEFILE_H
#define CHUNKSYNCCANDIDATEFILE_H

#include <common/Common.h>

class ChunkSyncCandidateFile
{
   public:
      ChunkSyncCandidateFile(std::string& relativePath, uint16_t targetID,
         bool onlyAttribs = false) :
         relativePath(relativePath), targetID(targetID), onlyAttribs(onlyAttribs)
      {
         // all initialization done in initializer list
      }

      // copy constructor
      ChunkSyncCandidateFile(const ChunkSyncCandidateFile& syncCandidate) :
         relativePath(syncCandidate.relativePath), targetID(syncCandidate.targetID),
         onlyAttribs(syncCandidate.onlyAttribs)
      {
         // all initialization done in initializer list
      }

      ChunkSyncCandidateFile(): relativePath(""), targetID(0), onlyAttribs(false)
      {
         // all initialization done in initializer list
      }

   private:
      std::string relativePath;
      uint16_t targetID;
      /* onlyAttribs is set if file contents should not be synced, but only attribs;
       * e.g. set if ctime changed, but not mtime */
      bool onlyAttribs;

   public:
      std::string getRelativePath() const
      {
         return relativePath;
      }

      uint16_t getTargetID() const
      {
         return targetID;
      }

      bool getOnlyAttribs() const
      {
         return onlyAttribs;
      }

      void setRelativePath(const std::string relativePath)
      {
         this->relativePath = relativePath;
      }

      void setTargetID(uint16_t targetID)
      {
         this->targetID = targetID;
      }

      void setOnlyAttribs(bool onlyAttribs)
      {
         this->onlyAttribs = onlyAttribs;
      }


};

typedef std::list<ChunkSyncCandidateFile> ChunkSyncCandidateFileList;
typedef ChunkSyncCandidateFileList::iterator ChunkSyncCandidateFileListIter;

#endif /* CHUNKSYNCCANDIDATEFILE_H */

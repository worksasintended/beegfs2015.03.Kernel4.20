#ifndef DYNAMICFILEATTRIBS_H_
#define DYNAMICFILEATTRIBS_H_

#include <common/toolkit/MathTk.h>

/*
 * Note: the optimizations here and in other places rely on chunksize being a power of two.
 */

class DynamicFileAttribs
{
   public:
      DynamicFileAttribs() : storageVersion(0)
      {
         // we only need to init storageVersion with 0, because that means all contained attribs
         //    are invalid
      }

      DynamicFileAttribs(uint64_t storageVersion, int64_t fileSize, uint64_t numBlocks,
         int64_t modificationTimeSecs, int64_t lastAccessTimeSecs) :
         storageVersion(storageVersion),
         fileSize(fileSize),
         numBlocks(numBlocks),
         modificationTimeSecs(modificationTimeSecs),
         lastAccessTimeSecs(lastAccessTimeSecs)
      {
         // nothing to do here (all done in initializer list)
      }


      uint64_t storageVersion; // use 0 as initial version and then only monotonic increase

      int64_t fileSize;
      int64_t numBlocks;   // number of used blocks by this chunk
      int64_t modificationTimeSecs;
      int64_t lastAccessTimeSecs;

      unsigned serialize(char* buf) const;
      bool deserialize(const char* buf, size_t bufLen, unsigned* outLen);
      static unsigned serialLen();

      friend bool dynamicFileAttribsEquals(const DynamicFileAttribs& first,
         const DynamicFileAttribs& second);
};


typedef std::vector<DynamicFileAttribs> DynamicFileAttribsVec;
typedef DynamicFileAttribsVec::iterator DynamicFileAttribsVecIter;
typedef DynamicFileAttribsVec::const_iterator DynamicFileAttribsVecCIter;

#endif /*DYNAMICFILEATTRIBS_H_*/

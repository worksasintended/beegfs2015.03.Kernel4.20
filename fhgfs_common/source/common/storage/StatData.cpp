/*
 * Information provided by stat()
 */

#include <common/toolkit/serialization/Serialization.h>
#include <common/app/log/LogContext.h>
#include <common/storage/StatData.h>

/* storing the actual number of blocks means some overhead for us, so define grace blocks for the
 * actual vs. caculated number of blocks. */
#define STATDATA_SPARSE_GRACEBLOCKS        (8)


/**
 * serialize data for stat()
 *
 * @param isDiskDirInode  on disk data for directories differ from those of files, so if we going to
 *        to serialize data written to disk, we need to now if it is a directory or file.
 *        However, network serialization does not differentiate between files and directories.
 * @param hasFlags  only new inodes don't have StatData flags.
 *        Always on for network serialization/deserialization
 * @param isNet           different values for network and disk serialization / deserialization
 */
size_t StatData::serialize(bool isDiskDirInode, bool hasFlags, bool isNet, char* outBuf)
{
   size_t bufPos = 0;

   if (hasFlags)
   {
      // flags
      bufPos += Serialization::serializeUInt(&outBuf[bufPos], this->flags);

      // mode, moved up to keep 64-bit alignment
      bufPos += Serialization::serializeUInt(&outBuf[bufPos], this->settableFileAttribs.mode);

      if (isNet)
      {  // serialization over network
         bufPos += Serialization::serializeUInt64(&outBuf[bufPos], this->getNumBlocks() );
      }
      else
      {  // serialization to disk

         // chunkBlocks
         if (getIsSparseFile() )
            bufPos += this->chunkBlocksVec.serialize(&outBuf[bufPos]);
      }
   }

   // create time
   bufPos += Serialization::serializeInt64(&outBuf[bufPos], this->creationTimeSecs);

   // atime
   bufPos += Serialization::serializeInt64(&outBuf[bufPos],
      this->settableFileAttribs.lastAccessTimeSecs);

   // mtime
   bufPos += Serialization::serializeInt64(&outBuf[bufPos],
      this->settableFileAttribs.modificationTimeSecs);

   // ctime
   bufPos += Serialization::serializeInt64(&outBuf[bufPos], this->attribChangeTimeSecs);

   if (isDiskDirInode == false)
   {
      // fileSsize
      bufPos += Serialization::serializeInt64(&outBuf[bufPos], this->fileSize);

      // nlink
      bufPos += Serialization::serializeUInt(&outBuf[bufPos], this->nlink);

      // contentsVersion
      bufPos += Serialization::serializeUInt(&outBuf[bufPos], this->contentsVersion);
   }

   // uid
   bufPos += Serialization::serializeUInt(&outBuf[bufPos], this->settableFileAttribs.userID);

   // gid
   bufPos += Serialization::serializeUInt(&outBuf[bufPos], this->settableFileAttribs.groupID);

   if (!hasFlags)
   { // mode
      bufPos += Serialization::serializeUInt(&outBuf[bufPos], this->settableFileAttribs.mode);
   }

   return bufPos;
}

/**
 * Deserialize data for stat()
 *
 * @param isDiskDirInode  see StatData::serialize()
 * @param hasFlags        see StatData::serialize()
 * @param isNet           different values for network and disk serialization / deserialization
 */
bool StatData::deserialize(bool isDiskDirInode, bool hasFlags, bool isNet,
   const char* buf, size_t bufLen, unsigned* outLen)
{
   const char* logContext = "StatData deserialiazation";

   unsigned bufPos = 0;

   if (hasFlags || isNet) // TODO: make it likely after some time
   {
      { // flags
         unsigned flagFieldLen;
         std::string serialType = "flags";

         if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &this->flags,
            &flagFieldLen) )
         {
            LogContext(logContext).logErr("Deserialization failed: " + serialType);
            return false;
         }

         bufPos += flagFieldLen;
      }

      { // mode, moved up to keep 64-bit aligment
         unsigned modeFieldLen;
         std::string serialType = "mode";

         if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos,
            &this->settableFileAttribs.mode, &modeFieldLen) )
         {
            LogContext(logContext).logErr("Deserialization failed: " + serialType);
            return false;
         }

         bufPos += modeFieldLen;
      }

      if (isNet)
      {  // deserialization over network

#if 0 // disabled, as this is currently only be used on the client side
         unsigned fieldLen;
         std::string serialType = "sumChunkBlocks";

         if(!Serialization::deserializeUInt64(&buf[bufPos], bufLen-bufPos, &this->sumChunkBlocks,
            &fieldLen) )
         {
            LogContext(logContext).logErr("Deserialization failed: " + serialType);
            return false;
         }

         bufPos += fieldLen;
#endif

         bufPos += Serialization::serialLenUInt64();
      }
      else
      {  // de-serialization from disk
         if (getIsSparseFile() )
         { // chunkBlocks
            unsigned fieldLen;
            std::string serialType = "chunkBlocks";

            if (!this->chunkBlocksVec.deserialize(&buf[bufPos], bufLen-bufPos, &fieldLen) )
            {
               LogContext(logContext).logErr("Deserialization failed: " + serialType);
               return false;
            }

            bufPos += fieldLen;
         }
      }

   }
   else
      this->flags = 0;


   { // creationTime
      unsigned createTimeFieldLen;
      std::string serialType = "creationTime";

      if(!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos,
         &this->creationTimeSecs, &createTimeFieldLen) )
      {
         LogContext(logContext).logErr("Deserialization failed: " + serialType);
         return false;
      }

      bufPos += createTimeFieldLen;
   }

   { // aTime
      unsigned aTimeFieldLen;
      std::string serialType = "atime";

      if(!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos,
         &this->settableFileAttribs.lastAccessTimeSecs, &aTimeFieldLen) )
      {
         LogContext(logContext).logErr("Deserialization failed: " + serialType);
         return false;
      }

      bufPos += aTimeFieldLen;
   }

   { // mtime
      unsigned mTimeFieldLen;
      std::string serialType = "mtime";

      if(!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos,
         &this->settableFileAttribs.modificationTimeSecs, &mTimeFieldLen) )
      {
         LogContext(logContext).logErr("Deserialization failed: " + serialType);
         return false;
      }

      bufPos += mTimeFieldLen;
   }

   { // ctime
      unsigned cTimeFieldLen;
      std::string serialType = "ctime";

      if(!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos,
         &this->attribChangeTimeSecs, &cTimeFieldLen) )
      {
         LogContext(logContext).logErr("Deserialization failed: " + serialType);
         return false;
      }

      bufPos += cTimeFieldLen;
   }

   if (isDiskDirInode == false)
   {
      { // fileSize
         unsigned sizeFieldLen;
         std::string serialType = "fileSize";

         if(!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos,
            &this->fileSize, &sizeFieldLen) )
         {
            LogContext(logContext).logErr("Deserialization failed: " + serialType);
            return false;
         }

         bufPos += sizeFieldLen;
      }

      { // nlink
         unsigned nlinkFieldLen;
         std::string serialType = "nlink";

         if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos,
            &this->nlink, &nlinkFieldLen) )
         {
            LogContext(logContext).logErr("Deserialization failed: " + serialType);
            return false;
         }

         bufPos += nlinkFieldLen;
      }

      { // contentsVersion
         unsigned contentsFieldLen;
         std::string serialType = "contentsVersion";

         if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos,
            &this->contentsVersion, &contentsFieldLen) )
         {
            LogContext(logContext).logErr("Deserialization failed: " + serialType);
            return false;
         }

         bufPos += contentsFieldLen;
      }

   }

   { // uid
      unsigned uidFieldLen;
      std::string serialType = "uid";

      if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos,
         &this->settableFileAttribs.userID, &uidFieldLen) )
      {
         LogContext(logContext).logErr("Deserialization failed: " + serialType);
         return false;
      }

      bufPos += uidFieldLen;
   }

   { // gid
      unsigned gidFieldLen;
      std::string serialType = "gid";

      if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos,
         &this->settableFileAttribs.groupID, &gidFieldLen) )
      {
         LogContext(logContext).logErr("Deserialization failed: " + serialType);
         return false;
      }

      bufPos += gidFieldLen;
   }

   if (!hasFlags) // TODO: make it unlikely after some time
   { // mode
      unsigned modeFieldLen;
      std::string serialType = "mode";

      if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos,
         &this->settableFileAttribs.mode, &modeFieldLen) )
      {
         LogContext(logContext).logErr("Deserialization failed: " + serialType);
         return false;
      }

      bufPos += modeFieldLen;
   }

   *outLen = bufPos;
   return true;
}

/*
 * Note: only used for network serialization, so without conditionals
 */
unsigned StatData::serialLen(void)
{
   unsigned length =
      Serialization::serialLenUInt()   + // flags
      Serialization::serialLenUInt()   + // mode
      Serialization::serialLenInt64()  + // sumChunkBlocks
      Serialization::serialLenInt64()  + // creationTimeSec
      Serialization::serialLenInt64()  + // lastAccessTimeSecs
      Serialization::serialLenInt64()  + // modificationTimeSecs
      Serialization::serialLenInt64()  + // attribChangeTimeSecs
      Serialization::serialLenInt64()  + // fileSize
      Serialization::serialLenUInt()   + // nlink
      Serialization::serialLenUInt()   + // contentsVersion
      Serialization::serialLenUInt()   + // userID
      Serialization::serialLenUInt();    // groupID

   return length;
}


/**
 * Update values relevant for FileInodes only.
 *
 * Note: This version is compatible with sparse files.
 */
void StatData::updateDynamicFileAttribs(ChunkFileInfoVec& fileInfoVec, StripePattern* stripePattern)
{
   // we use the dynamic attribs only if they have been initialized (=> storageVersion != 0)

   // dynamic attrib algo:
   // walk over all the stripeNodes and...
   // - find last node with max number of (possibly incomplete) chunks
   // - find the latest modification- and lastAccessTimeSecs


   // scan for an initialized index...
   /* note: we assume that in most cases, the user will stat not-opened files (which have
      all storageVersions set to 0), so we optimize for that case. */

   unsigned numValidDynAttribs = 0;

   size_t numStripeTargets = stripePattern->getNumStripeTargetIDs();

   for (size_t i=0; i < fileInfoVec.size(); i++)
   {
      if (fileInfoVec[i].getStorageVersion() )
      {
         numValidDynAttribs++;
      }
   }

   if (numValidDynAttribs == 0)
   { // no valid dyn attrib element found => use static attribs and nothing to do
      return;
   }

   /* we found a valid dyn attrib index => start with first target anyways to avoid false file
      length computations... */

   unsigned lastMaxNumChunksIndex = 0;
   int64_t maxNumChunks = fileInfoVec[0].getNumChunks();

   // the first mtime on the metadata server can be newer then the mtime on the storage server due
   // to minor time difference between the servers, so the ctime should be updated after every
   // close, this issue should be fixed by the usage of the max(...) function
   // note: only use max(MDS, first chunk) if we don't have information from all chunks, because
   // if we have information from all chunks and the info on the MDS differs, info on the MDS must
   // be wrong and therefore should be overwritten
   int64_t oldModificationTimeSecs;
   int64_t newModificationTimeSecs;
   int64_t newLastAccessTimeSecs;
   if (numValidDynAttribs == numStripeTargets)
   {
      oldModificationTimeSecs = fileInfoVec[0].getModificationTimeSecs();
      newModificationTimeSecs = oldModificationTimeSecs; // initialize with the old value

      newLastAccessTimeSecs = fileInfoVec[0].getLastAccessTimeSecs();
   }
   else
   {
      oldModificationTimeSecs = std::max(fileInfoVec[0].getModificationTimeSecs(),
         settableFileAttribs.modificationTimeSecs);
      newModificationTimeSecs = oldModificationTimeSecs; // initialize with the old value

      newLastAccessTimeSecs = std::max(fileInfoVec[0].getLastAccessTimeSecs(),
         settableFileAttribs.lastAccessTimeSecs);
   }

   setTargetChunkBlocks(0, fileInfoVec[0].getNumBlocks(), numStripeTargets);

   int64_t newFileSize;

   for (unsigned target = 1; target < fileInfoVec.size(); target++)
   {
      /* note: we cannot ignore storageVersion==0 here, because that would lead to wrong file
       *       length computations.
       * note2: fileInfoVec will be initialized once we have read meta data from disk, so if a
       *        storage target does not answer or no request was sent to it,
       *        we can still use old data from fileInfoVec
       *
       * */

      int64_t currentNumChunks = fileInfoVec[target].getNumChunks();

      if(currentNumChunks >= maxNumChunks)
      {
         lastMaxNumChunksIndex = target;
         maxNumChunks = currentNumChunks;
      }

      int64_t chunkModificationTimeSecs = fileInfoVec[target].getModificationTimeSecs();
      if(chunkModificationTimeSecs > newModificationTimeSecs)
         newModificationTimeSecs = chunkModificationTimeSecs;

      int64_t currentLastAccessTimeSecs = fileInfoVec[target].getLastAccessTimeSecs();
      if(currentLastAccessTimeSecs > newLastAccessTimeSecs)
         newLastAccessTimeSecs = currentLastAccessTimeSecs;

      setTargetChunkBlocks(target, fileInfoVec[target].getNumBlocks(), numStripeTargets);
   }

   if(maxNumChunks)
   { // we have chunks, so "fileLength > 0"

      // note: we do all this complex filesize calculation stuff here to support sparse files

      // every target before lastMax-target must have the same number of (complete) chunks
      int64_t lengthBeforeLastMaxIndex = lastMaxNumChunksIndex * maxNumChunks *
         stripePattern->getChunkSize();

      // every target after lastMax-target must have exactly one chunk less than the lastMax-target
      int64_t lengthAfterLastMaxIndex = (fileInfoVec.size() - lastMaxNumChunksIndex - 1) *
         (maxNumChunks - 1) * stripePattern->getChunkSize();

      // all the prior calcs are based on the lastMax-target size, so we can simply use that one
      int64_t lengthLastMaxNode = fileInfoVec[lastMaxNumChunksIndex].getFileSize();

      int64_t totalLength = lengthBeforeLastMaxIndex + lengthAfterLastMaxIndex + lengthLastMaxNode;

      newFileSize = totalLength;

      uint64_t calcBlocks = newFileSize >> STATDATA_SIZETOBLOCKSBIT_SHIFT;
      uint64_t usedBlockSum = getNumBlocks();

      if (usedBlockSum + STATDATA_SPARSE_GRACEBLOCKS < calcBlocks)
         setSparseFlag();
      else
         unsetSparseFlag();

   }
   else
   { // no chunks, no filesize
      newFileSize = 0;
   }

   // now update class values

   setLastAccessTimeSecs(newLastAccessTimeSecs);
   setModificationTimeSecs(newModificationTimeSecs);

   int64_t oldFileSize = getFileSize();

   // mtime or fileSize updated, so also ctime needs to be updated
   if (newModificationTimeSecs != oldModificationTimeSecs || oldFileSize != newFileSize)
      setAttribChangeTimeSecs(TimeAbs().getTimeval()->tv_sec);

   setFileSize(newFileSize);
}


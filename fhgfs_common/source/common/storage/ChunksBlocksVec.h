#ifndef CHUNKBLOCKSVEC_H_
#define CHUNKBLOCKSVEC_H_

#include <common/toolkit/serialization/Serialization.h>
#include <common/app/log/LogContext.h>

class ChunksBlocksVec
{
   public:

      ChunksBlocksVec()
      {
         this->numStripeTargets = 0;
      }

      ChunksBlocksVec(size_t numTargets) : chunkBlocksVec(numTargets, 0)
      {
         this->numStripeTargets = numTargets;
      }

   private:

      unsigned numStripeTargets;
      UInt64Vector chunkBlocksVec; // number of blocks for each chunk file


   public:

      // setters

      /**
       * Set the number of used blocks for the chunk-file of this target
       */
      void setNumBlocks(size_t target, uint64_t numBlocks, size_t numStripeTargets)
      {
         const char* logContext = "ChunksBlocksVec set num blocks";

         if (this->numStripeTargets == 0)
         {
            this->numStripeTargets = numStripeTargets;
            this->chunkBlocksVec.resize(numStripeTargets, 0);
         }

         if (unlikely(target > this->numStripeTargets - 1) )
         {
            LogContext(logContext).logErr("Bug: target > vec-size");
            LogContext(logContext).logBacktrace();
            return;
         }

         this->chunkBlocksVec.at(target) = numBlocks;
      }

      // getters

      /**
       * Get the number of blocks for the given target
       */
      uint64_t getNumBlocks(size_t target) const
      {
         const char* logContext = "ChunksBlocksVec get number of blocks";

         if (this->numStripeTargets == 0)
            return 0;

         if (unlikely(target > this->numStripeTargets - 1) )
         {
            LogContext(logContext).logErr("Bug: target number larget than stripes");
            LogContext(logContext).logBacktrace();
            return 0;
         }

         return this->chunkBlocksVec.at(target);
      }

      /**
       * Get the sum of all used blocks of all targets
       */
      uint64_t getBlockSum()
      {
         uint64_t sum = 0;
         UInt64VectorIter iter = this->chunkBlocksVec.begin();

         while (iter != this->chunkBlocksVec.end() )
         {
            sum += *iter;
            iter++;
         }

         return sum;
      }

      // inlined

      size_t serialize(char* buf)
      {
         size_t bufPos = 0;

         // chunkBlocksVec
         bufPos += Serialization::serializeUInt64Vector(buf, &this->chunkBlocksVec);

         return bufPos;
      }

      bool deserialize(const char* buf, size_t bufLen, unsigned* outLen)
      {
         const char* vecStart;
         unsigned vecBufLen;

         unsigned bufPos = 0;

         {  // chunkBlocksVec, needs to be 64-bit aligned
            if(!Serialization::deserializeUInt64VectorPreprocess(&buf[bufPos], bufLen - bufPos,
                  &this->numStripeTargets, &vecStart, &vecBufLen) )
               return false;

            if (!Serialization::deserializeUInt64Vector(vecBufLen, this->numStripeTargets,
               vecStart, &this->chunkBlocksVec) )
               return false;

            bufPos += vecBufLen;
         }

         *outLen = bufPos;

         return true;
      }

};


#endif /* CHUNKBLOCKSVEC_H_ */

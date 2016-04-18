#ifndef BITSTORE_COMMON_H_
#define BITSTORE_COMMON_H_


#include <common/Common.h>

typedef long unsigned int bitstore_store_type;

#define BITSTORE_BLOCK_SIZE         sizeof(bitstore_store_type)   // size of a block in bytes
#define BITSTORE_BLOCK_BIT_COUNT    (BITSTORE_BLOCK_SIZE * 8)     // size of a block in bits
#define BITSTORE_BLOCK_INIT_MASK    0UL                           // mask to zero all bits of block


/**
 * A vector of bits.
 */
class BitStore
{
   friend class TestBitStore;

   public:
      BitStore()
      {
         init(true);
      }

      BitStore(bool setZero)
      {
         init(setZero);
      }

      BitStore(int size)
      {
         init(false);
         setSize(size);
         clearBits();
      }

      ~BitStore()
      {
         SAFE_FREE_NOSET(this->higherBits);
      }

      void setBit(unsigned bitIndex, bool isSet);

      void setSize(unsigned newSize);
      void clearBits();

      bool compare(BitStore* second);
      void copy(const BitStore* other);
      void copyThreadSafe(const BitStore* other);

      unsigned serialLen() const;
      unsigned serialize(char* buf) const;
      static bool deserializePreprocess(const char* buf, size_t bufLen,
         const char** outBitStoreStart, unsigned* outLen);
      void deserialize(const char* buf, unsigned* outLen);

      static unsigned calculateBitBlockCount(unsigned size);


   private:
      unsigned numBits; // max number of bits that this store can hold
      bitstore_store_type lowerBits; // used to avoid overhead of extra alloc for higherBits
      bitstore_store_type* higherBits; // array, which is alloc'ed only when needed

      void freeHigherBits();


   protected:

   public:
      // inliners

      void init(bool setZero)
      {
         this->numBits = BITSTORE_BLOCK_BIT_COUNT;
         this->higherBits = NULL;

         if (setZero)
            this->lowerBits = BITSTORE_BLOCK_INIT_MASK;
      }

      /**
       * Test whether the bit at the given index is set or not.
       *
       * Note: The bit operations in here are non-atomic.
       */
      bool getBitNonAtomic(unsigned bitIndex) const
      {
         size_t index;
         size_t indexInBitBlock;
         bitstore_store_type mask;

         if (bitIndex >= this->numBits)
            return false;

         index = getBitBlockIndex(bitIndex);
         indexInBitBlock = getBitIndexInBitBlock(bitIndex);
         mask = calculateBitMaskInBitBlock(indexInBitBlock);

         if (index == 0)
            return (this->lowerBits & mask) ? true : false;
         else
            return (this->higherBits[index - 1] & mask) ? true : false;
      }

      /**
       * returns the index of the bit block (array index) which contains the searched bit
       */
      static int getBitBlockIndex(unsigned bitIndex)
      {
         return (bitIndex / BITSTORE_BLOCK_BIT_COUNT);
      }

      /**
       * returns the index in a bit block for the searched bit
       */
      static int getBitIndexInBitBlock(unsigned bitIndex)
      {
         return (bitIndex % BITSTORE_BLOCK_BIT_COUNT);
      }

      /**
       * calculates the bit mask for selecting a bit in a bit block, is needed for AND or OR bit
       * operations with a bit block
       */
      static bitstore_store_type calculateBitMaskInBitBlock(unsigned bitIndexInBitBlock)
      {
         if(unlikely(bitIndexInBitBlock >= BITSTORE_BLOCK_BIT_COUNT) )
            return BITSTORE_BLOCK_INIT_MASK;

         bitstore_store_type mask = (1UL << bitIndexInBitBlock);

         return mask;
      }
};

#endif /* BITSTORE_COMMON_H_ */

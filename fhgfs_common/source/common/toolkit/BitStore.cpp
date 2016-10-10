#include <common/toolkit/serialization/Serialization.h>
#include <common/app/log/LogContext.h>
#include "BitStore.h"



/**
 * Set or unset a bit at the given index.
 *
 * Note: The bit operations in here are not atomic.
 *
 * @param isSet true to set or false to unset the corresponding bit.
 */
void BitStore::setBit(unsigned bitIndex, bool isSet)
{
   unsigned index = getBitBlockIndex(bitIndex);
   unsigned indexInBitBlock = getBitIndexInBitBlock(bitIndex);
   bitstore_store_type mask = calculateBitMaskInBitBlock(indexInBitBlock);

   if(unlikely(bitIndex >= this->numBits) )
      return;

   if (index == 0)
   {
      if (isSet)
         this->lowerBits = this->lowerBits | mask;
      else
         this->lowerBits = this->lowerBits & (~mask);
   }
   else
   {
      if (isSet)
         this->higherBits[index - 1] = this->higherBits[index - 1] | mask;
      else
         this->higherBits[index - 1] = this->higherBits[index - 1] & (~mask);
   }
}

/**
 * grow/shrink internal buffers as needed for new size.
 *
 * note: this method does not reset or copy bits, so consider the bits to be uninitialized after
 * calling this method. thus, you might want to call clearBits() afterwards to clear all bits.
 *
 * @param size number of bits
 */
void BitStore::setSize(unsigned newSize)
{
   unsigned oldBitBlockCount = calculateBitBlockCount(this->numBits);
   unsigned newBitBlockCount = calculateBitBlockCount(newSize);

   if(newBitBlockCount != oldBitBlockCount)
   {
      SAFE_FREE(this->higherBits);

      if(newBitBlockCount > 1)
      {
         this->higherBits = (bitstore_store_type*)malloc(
            BITSTORE_BLOCK_SIZE * (newBitBlockCount - 1) );
      }

      this->numBits = newBitBlockCount * BITSTORE_BLOCK_BIT_COUNT;
   }
}

/**
 * reset all bits to 0.
 */
void BitStore::clearBits()
{
   this->lowerBits = BITSTORE_BLOCK_INIT_MASK;

   if(this->higherBits)
   {
      unsigned bitBlockCount = calculateBitBlockCount(this->numBits);

      memset(this->higherBits, 0, (bitBlockCount-1) * BITSTORE_BLOCK_SIZE);
   }
}

/**
 * release the memory of the higher bits and set the size of the bit store to the minimal value
 */
void BitStore::freeHigherBits()
{
   SAFE_FREE(this->higherBits);

   this->numBits = BITSTORE_BLOCK_BIT_COUNT;
}

/**
 * note: serialized format is always one or more full 64bit blocks to keep the format independent
 * of 32bit/64bit archs.
 * note: serialized format is 8 byte aligned.
 */
unsigned BitStore::serialLen() const
{
   unsigned blockCount = calculateBitBlockCount(this->numBits);

   unsigned len = 0;
   len += Serialization::serialLenUInt();             // number of bits
   len += Serialization::serialLenUInt();             // padding for 8byte alignment
   len += (blockCount * BITSTORE_BLOCK_SIZE);         // bits

   // add padding to allow 64bit deserialize from 32bit serialize
   if ((BITSTORE_BLOCK_SIZE == sizeof(uint32_t)) && (blockCount & 1))
   { // odd number of 32bit blocks
      len += Serialization::serialLenUInt();          // padding
   }

   return len;
}

/**
 * note: serialized format is always one or more full 64bit blocks to keep the format independent
 * of 32bit/64bit archs.
 * note: serialized format is 8 byte aligned.
 */
unsigned BitStore::serialize(char* buf) const
{
   unsigned blockCount = calculateBitBlockCount(this->numBits);

   size_t bufPos = 0;

   { // size
      bufPos += Serialization::serializeUInt(&buf[bufPos], this->numBits);
   }

   // padding for 8byte alignment

   bufPos += Serialization::serialLenUInt();

   { // lowerBits
      if (BITSTORE_BLOCK_SIZE == sizeof(uint64_t) )
         bufPos += Serialization::serializeUInt64(&buf[bufPos], this->lowerBits);
      else
      if (BITSTORE_BLOCK_SIZE == sizeof(uint32_t) )
         bufPos += Serialization::serializeUInt(&buf[bufPos], this->lowerBits);
   }

   { // higherBits
      for(unsigned index = 0; index < (blockCount - 1); index++)
      {
         if (BITSTORE_BLOCK_SIZE == sizeof(uint64_t) )
            bufPos += Serialization::serializeUInt64(&buf[bufPos], this->higherBits[index]);
         else
         if (BITSTORE_BLOCK_SIZE == sizeof(uint32_t) )
            bufPos += Serialization::serializeUInt(&buf[bufPos], this->higherBits[index]);
      }
   }

   // add padding to allow 64bit deserialize from 32bit serialize
   if( (BITSTORE_BLOCK_SIZE == sizeof(uint32_t) ) && (blockCount & 1) )
   { // odd number of 32bit blocks
      bufPos += Serialization::serializeUInt(&buf[bufPos], 0);
   }

   return bufPos;
}

/**
 * @param outBitStoreStart deserialization buffer pointer for deserialize()
 */
bool BitStore::deserializePreprocess(const char* buf, size_t bufLen, const char** outBitStoreStart,
   unsigned* outLen)
{
   size_t bufPos = 0;

   unsigned tmpSize = 0;

   *outBitStoreStart = buf;

   { // size
      unsigned bufSizeLen = 0;         // "=0" to mute false compiler warning

      if(unlikely(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &tmpSize,
         &bufSizeLen) ) )
         return false;
      bufPos += bufSizeLen;
   }

   // padding for 8byte alignment

   bufPos += Serialization::serialLenUInt();

   // bit blocks

   unsigned blockCount = calculateBitBlockCount(tmpSize);

   size_t blocksLen;

   if(BITSTORE_BLOCK_SIZE == sizeof(uint64_t) )
      blocksLen = Serialization::serialLenUInt64() * blockCount;
   else
   if(BITSTORE_BLOCK_SIZE == sizeof(uint32_t) )
   {
      // add padding that allows 64bit deserialize from 32bit serialize
      if(blockCount & 1)
         blockCount++;

      blocksLen = Serialization::serialLenUInt() * blockCount;
   }
   else
      return false;

   bufPos += blocksLen;

   // check buffer length

   if(unlikely(bufPos > bufLen) )
      return false;

   *outLen = bufPos;

   return true;
}

void BitStore::deserialize(const char* buf, unsigned* outLen)
{
   const char* logContext = "BitStore deserialize";

   size_t bufPos = 0;
   size_t bufLen = ~0; // fake bufLen to max value (has already been verified during pre-processing)

   unsigned blockCount;
   unsigned bitCount = 0;

   { // size
      unsigned bufSizeLen = 0;         // "=0" to mute false compiler warning

      // store the bit count in a temp variable because setSizeAndReset() below needs old count
      Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &bitCount, &bufSizeLen);
      bufPos += bufSizeLen;
   }

   // padding for 8byte alignment

   bufPos += Serialization::serialLenUInt();

   // clear and alloc memory for the higher bits if needed

   setSize(bitCount);

   { // lowerBits
      unsigned bufLowerBitsLen = 0;    // "=0" to mute false compiler warning

      if (BITSTORE_BLOCK_SIZE == sizeof(uint64_t) )
         Serialization::deserializeUInt64(&buf[bufPos], bufLen-bufPos, (uint64_t*)&this->lowerBits,
            &bufLowerBitsLen);
      else
      if (BITSTORE_BLOCK_SIZE == sizeof(uint32_t) )
         Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, (uint32_t*)&this->lowerBits,
            &bufLowerBitsLen);

      bufPos += bufLowerBitsLen;
   }

   blockCount = calculateBitBlockCount(this->numBits);

   { // higherBits
      unsigned bufHigherBitsLen = 0;   // "=0" to mute false compiler warning

      #ifdef BEEGFS_DEBUG
      if(unlikely( (blockCount > 1) && !this->higherBits) )
      { // sanity check
         LogContext(logContext).logErr("Bug: higherBits==NULL");
         LogContext(logContext).logBacktrace();
      }
      #endif // BEEGFS_DEBUG

      IGNORE_UNUSED_VARIABLE(logContext);

      for(unsigned index = 0; index < (blockCount - 1); index++)
      {
         bitstore_store_type* higherBits = &this->higherBits[index];

         if (BITSTORE_BLOCK_SIZE == sizeof(uint64_t) )
            Serialization::deserializeUInt64(&buf[bufPos], bufLen-bufPos,
               (uint64_t*)higherBits, &bufHigherBitsLen);
         else
         if (BITSTORE_BLOCK_SIZE == sizeof(uint32_t) )
            Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos,
               (uint32_t*)higherBits, &bufHigherBitsLen);

         bufPos += bufHigherBitsLen;
      }
   }

   // add padding that allows 64bit deserialize from 32bit serialize
   if( (BITSTORE_BLOCK_SIZE == sizeof(uint32_t) ) && (blockCount & 1) )
      bufPos += Serialization::serialLenUInt();


   *outLen = bufPos;
}

/**
 * compare two BitStores.
 *
 * @return true if both BitStores are equal
 */
bool BitStore::compare(BitStore* second)
{
   if(this->numBits != second->numBits)
      return false;
   else
   if(this->lowerBits != second->lowerBits)
      return false;
   else
   {
      unsigned blockCount = calculateBitBlockCount(this->numBits);

      for(unsigned index = 0; index < (blockCount - 1); index++)
      {
         if(this->higherBits[index] != second->higherBits[index])
         {
            return false;
         }
      }
   }

   return true;
}

/**
 * Update this store with values from given other store.
 */
void BitStore::copy(const BitStore* other)
{
   setSize(other->numBits);

   this->lowerBits = other->lowerBits;

   if(!this->higherBits)
      return;

   // we have higherBits to copy

   unsigned blockCount = calculateBitBlockCount(this->numBits);

   memcpy(this->higherBits, other->higherBits, (blockCount-1) * BITSTORE_BLOCK_SIZE);
}

/**
 * Update this store with values from given other store.
 *
 * To make this copy thread-safe, this method does not attempt any re-alloc of internal buffers,
 * so the copy will not be complete if the second store contains more blocks.
 */
void BitStore::copyThreadSafe(const BitStore* other)
{
   this->lowerBits = other->lowerBits;

   if(!this->higherBits)
      return;

   // we need to set our higherBits

   unsigned thisBlockCount = calculateBitBlockCount(this->numBits);
   unsigned otherBlockCount = calculateBitBlockCount(other->numBits);

   if(otherBlockCount > 1)
   { // copy as much as we can from other's higherBits
      unsigned minCommonBlockCount = BEEGFS_MIN(thisBlockCount, otherBlockCount);

      memcpy(this->higherBits, other->higherBits, (minCommonBlockCount-1) * BITSTORE_BLOCK_SIZE);
   }

   // (this store must at least have one higherBits block, because higherBits!=NULL)
   if(thisBlockCount > otherBlockCount)
   { // zero remaining higherBits of this store
      unsigned numBlocksDiff = thisBlockCount - otherBlockCount;

      // ("otherBlockCount-1": -1 for lowerBits block)
      memset(&(this->higherBits[otherBlockCount-1]), 0, numBlocksDiff * BITSTORE_BLOCK_SIZE);
   }
}

/**
 * calculates the needed bit block count for the given BitStore size
 *
 * @param size number of bits
 */
unsigned BitStore::calculateBitBlockCount(unsigned size)
{
   unsigned retVal;

   if(size <= BITSTORE_BLOCK_BIT_COUNT)
      return 1; // only first block needed


   retVal = size / BITSTORE_BLOCK_BIT_COUNT;

   // find out whether we use a partial block

   if (size % BITSTORE_BLOCK_BIT_COUNT)
      retVal++; // we need another (partial) block

   return retVal;
}

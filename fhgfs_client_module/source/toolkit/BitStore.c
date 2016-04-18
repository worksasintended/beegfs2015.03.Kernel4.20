#include <common/toolkit/Serialization.h>
#include "BitStore.h"

/**
 * Set or unset a bit at the given index.
 *
 * Note: The bit operations in here are atomic.
 *
 * @param isSet true to set or false to unset the corresponding bit.
 */
void BitStore_setBit(BitStore* this, unsigned bitIndex, fhgfs_bool isSet)
{
   unsigned index = BitStore_getBitBlockIndex(bitIndex);
   unsigned indexInBitBlock = BitStore_getBitIndexInBitBlock(bitIndex);

   if(unlikely(bitIndex >= this->numBits) )
   {
      BEEGFS_BUG_ON(fhgfs_true, "index out of bounds: bitIndex >= this->numBits");
      return;
   }

   if (index == 0)
   {
      if (isSet)
         set_bit(indexInBitBlock, &this->lowerBits);
      else
         clear_bit(indexInBitBlock, &this->lowerBits);
   }
   else
   {
      if (isSet)
         set_bit(indexInBitBlock, &this->higherBits[index - 1]);
      else
         clear_bit(indexInBitBlock, &this->higherBits[index - 1]);
   }
}

/**
 * grow/shrink internal buffers when needed for new size.
 *
 * note: this method does not reset or copy bits, so consider the bits to be uninitialized after
 * calling this method. thus, you might want to call clearBits() afterwards to clear all bits.
 *
 * @param size number of bits
 */
void BitStore_setSize(BitStore* this, unsigned newSize)
{
   unsigned oldBitBlockCount = BitStore_calculateBitBlockCount(this->numBits);
   unsigned newBitBlockCount = BitStore_calculateBitBlockCount(newSize);

   if(newBitBlockCount != oldBitBlockCount)
   {
      SAFE_KFREE(this->higherBits);

      if(newBitBlockCount > 1)
      {
         this->higherBits = (bitstore_store_type*)os_kmalloc(
            BITSTORE_BLOCK_SIZE * (newBitBlockCount - 1) );
      }

      this->numBits = newBitBlockCount * BITSTORE_BLOCK_BIT_COUNT;
   }
}

/**
 * reset all bits to 0
 */
void BitStore_clearBits(BitStore* this)
{
   this->lowerBits = BITSTORE_BLOCK_INIT_MASK;

   if(this->higherBits)
   {
      unsigned bitBlockCount = BitStore_calculateBitBlockCount(this->numBits);

      os_memset(this->higherBits, 0, (bitBlockCount-1) * BITSTORE_BLOCK_SIZE);
   }

}

/**
 * release the memory of the higher bits and set the size of the bit store to the minimal value
 */
void __BitStore_freeHigherBits(BitStore* this)
{
   SAFE_KFREE(this->higherBits);

   this->numBits = BITSTORE_BLOCK_BIT_COUNT;
}


/**
 * note: serialized format is always one or more full 64bit blocks to keep the format independent
 * of 32bit/64bit archs.
 * note: serialized format is 8 byte aligned.
 */
unsigned BitStore_serialLen(const BitStore* this)
{
   unsigned blockCount = BitStore_calculateBitBlockCount(this->numBits);

   unsigned len = 0;
   len += Serialization_serialLenUInt();              // number of bits
   len += Serialization_serialLenUInt();              // padding for 8byte alignment
   len += (blockCount * BITSTORE_BLOCK_SIZE);         // bits

   // add padding to allow 64bit deserialize from 32bit serialize
   if ((BITSTORE_BLOCK_SIZE == sizeof(uint32_t) ) && (blockCount & 1))
   { // odd number of 32bit blocks
      len += BITSTORE_BLOCK_SIZE;           // padding
   }

   return len;
}

/**
 * note: serialized format is always one or more full 64bit blocks to keep the format independent
 * of 32bit/64bit archs.
 * note: serialized format is 8 byte aligned.
 */
unsigned BitStore_serialize(const BitStore* this, char* buf)
{
   unsigned index;
   unsigned blockCount = BitStore_calculateBitBlockCount(this->numBits);

   size_t bufPos = 0;

   // size

   bufPos += Serialization_serializeUInt(&buf[bufPos], this->numBits);

   // padding for 8byte alignment

   bufPos += Serialization_serialLenUInt();

   { // lowerBits
      if (BITSTORE_BLOCK_SIZE == sizeof(uint64_t) )
         bufPos += Serialization_serializeUInt64(&buf[bufPos], this->lowerBits);
      else
      if (BITSTORE_BLOCK_SIZE == sizeof(uint32_t) )
         bufPos += Serialization_serializeUInt(&buf[bufPos], this->lowerBits);
   }

   { // higherBits
      for(index = 0; index < (blockCount - 1); index++)
      {
         if (BITSTORE_BLOCK_SIZE == sizeof(uint64_t) )
            bufPos += Serialization_serializeUInt64(&buf[bufPos], this->higherBits[index]);
         else
         if (BITSTORE_BLOCK_SIZE == sizeof(uint32_t) )
            bufPos += Serialization_serializeUInt(&buf[bufPos], this->higherBits[index]);
      }
   }

   // add padding to allow 64bit deserialize from 32bit serialize
   if( (BITSTORE_BLOCK_SIZE == sizeof(uint32_t) ) && (blockCount & 1) )
   { // odd number of 32bit blocks
      bufPos += Serialization_serializeUInt(&buf[bufPos], 0);
   }

   return bufPos;
}

/**
 * @param outBitStoreStart deserialization buffer pointer for deserialize()
 */
fhgfs_bool BitStore_deserializePreprocess(const char* buf, size_t bufLen,
   const char** outBitStoreStart, unsigned* outLen)
{
   size_t bufPos = 0;

   unsigned tmpSize = 0; // =0, because it is used later and otherwise gcc complains
                         // with "may be uninitialized"
   unsigned blockCount;
   size_t blocksLen;

   *outBitStoreStart = buf;

   {// size
      unsigned bufSizeLen = 0;         // "=0" to mute false compiler warning

      if(unlikely(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, &tmpSize,
         &bufSizeLen) ) )
         return fhgfs_false;
      bufPos += bufSizeLen;
   }

   // padding for 8byte alignment

   bufPos += Serialization_serialLenUInt();

   // bit blocks

   blockCount = BitStore_calculateBitBlockCount(tmpSize);

   if(BITSTORE_BLOCK_SIZE == sizeof(uint64_t) )
      blocksLen = Serialization_serialLenUInt64() * blockCount;
   else
   if(BITSTORE_BLOCK_SIZE == sizeof(uint32_t) )
   {
      // add padding that allows 64bit deserialize from 32bit serialize
      if(blockCount & 1)
         blockCount++;

      blocksLen = Serialization_serialLenUInt() * blockCount;
   }
   else
      return fhgfs_false;

   bufPos += blocksLen;

   // check buffer length

   if(unlikely(bufPos > bufLen) )
      return fhgfs_false;

   *outLen = bufPos;

   return fhgfs_true;
}

void BitStore_deserialize(BitStore* this, const char* buf, unsigned* outLen)
{
   size_t bufPos = 0;
   size_t bufLen = ~0; // fake bufLen to max value (has already been verified during pre-processing)

   unsigned blockCount;
   unsigned bitCount = 0;

   { // size
      unsigned bufSizeLen = 0;         // "=0" to mute false compiler warning

      // store the bit count in a temp variable because setSizeAndReset() below needs old count
      Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, &bitCount, &bufSizeLen);
      bufPos += bufSizeLen;
   }

   // padding for 8byte alignment

   bufPos += Serialization_serialLenUInt();

   // clear and alloc memory for the higher bits if needed

   BitStore_setSize(this, bitCount);

   { // lowerBits
      unsigned bufLowerBitsLen = 0;    // "=0" to mute false compiler warning

      if (BITSTORE_BLOCK_SIZE == sizeof(uint64_t) )
         Serialization_deserializeUInt64(&buf[bufPos], bufLen-bufPos,
            (uint64_t*)&this->lowerBits, &bufLowerBitsLen);
      else
      if (BITSTORE_BLOCK_SIZE == sizeof(uint32_t) )
         Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos,
            (uint32_t*)&this->lowerBits, &bufLowerBitsLen);

      bufPos += bufLowerBitsLen;
   }

   blockCount = BitStore_calculateBitBlockCount(this->numBits);

   { // higherBits
      unsigned bufHigherBitsLen = 0;   // "=0" to mute false compiler warning
      unsigned index;

      BEEGFS_BUG_ON_DEBUG( (blockCount > 1) && !this->higherBits, "Bug: higherBits==NULL");

      for(index = 0; index < (blockCount - 1); index++)
      {
         bitstore_store_type* higherBits = &this->higherBits[index];

         if (BITSTORE_BLOCK_SIZE == sizeof(uint64_t) )
            Serialization_deserializeUInt64(&buf[bufPos], bufLen-bufPos,
               (uint64_t*)higherBits, &bufHigherBitsLen);
         else
         if (BITSTORE_BLOCK_SIZE == sizeof(uint32_t) )
            Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos,
               (uint32_t*)higherBits, &bufHigherBitsLen);

         bufPos += bufHigherBitsLen;
      }
   }

   // add padding that allows 64bit deserialize from 32bit serialize
   if( (BITSTORE_BLOCK_SIZE == sizeof(uint32_t) ) && (blockCount & 1) )
      bufPos += Serialization_serialLenUInt();

   *outLen = bufPos;
}

/**
 * compare two BitStores.
 *
 * @return true if both BitStores are equal
 */
fhgfs_bool BitStore_compare(BitStore* this, BitStore* second)
{
   if(this->numBits != second->numBits)
      return fhgfs_false;
   else
   if(this->lowerBits != second->lowerBits)
      return fhgfs_false;
   else
   {
      unsigned blockCount = BitStore_calculateBitBlockCount(this->numBits);
      unsigned index;

      for(index = 0; index < (blockCount - 1); index++)
      {
         if(this->higherBits[index] != second->higherBits[index])
         {
            return fhgfs_false;
         }
      }
   }

   return fhgfs_true;
}

/**
 * Update this store with values from given other store.
 */
void BitStore_copy(BitStore* this, BitStore* other)
{
   BitStore_setSize(this, other->numBits);

   this->lowerBits = other->lowerBits;

   if(!this->higherBits)
      return;

   // we have higherBits to copy
   {
      unsigned blockCount = BitStore_calculateBitBlockCount(this->numBits);

      os_memcpy(this->higherBits, other->higherBits, (blockCount-1) * BITSTORE_BLOCK_SIZE);
   }
}

/**
 * Update this store with values from given other store.
 *
 * To make this copy thread-safe, this method does not attempt any re-alloc of internal buffers,
 * so the copy will not be complete if the second store contains more blocks.
 */
void BitStore_copyThreadSafe(BitStore* this, const BitStore* other)
{
   unsigned thisBlockCount;
   unsigned otherBlockCount;

   this->lowerBits = other->lowerBits;

   if(!this->higherBits)
      return;

   // copy (or clear) our higherBits

   thisBlockCount = BitStore_calculateBitBlockCount(this->numBits);
   otherBlockCount = BitStore_calculateBitBlockCount(other->numBits);

   if(otherBlockCount > 1)
   { // copy as much as we can from other's higherBits
      unsigned minCommonBlockCount = MIN(thisBlockCount, otherBlockCount);

      os_memcpy(this->higherBits, other->higherBits, (minCommonBlockCount-1) * BITSTORE_BLOCK_SIZE);
   }

   // (this store must at least have one higherBits block, because higherBits!=NULL)
   if(thisBlockCount > otherBlockCount)
   { // zero remaining higherBits of this store
      unsigned numBlocksDiff = thisBlockCount - otherBlockCount;

      // ("otherBlockCount-1": -1 for lowerBits block)
      os_memset(&(this->higherBits[otherBlockCount-1]), 0, numBlocksDiff * BITSTORE_BLOCK_SIZE);
   }
}

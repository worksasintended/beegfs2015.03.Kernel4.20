#ifndef BITSTORE_H_
#define BITSTORE_H_

#include <common/Common.h>


typedef long unsigned int bitstore_store_type;


#define BITSTORE_BLOCK_SIZE         sizeof(bitstore_store_type)   // size of a block in bytes
#define BITSTORE_BLOCK_BIT_COUNT    (BITSTORE_BLOCK_SIZE * 8)     // size of a block in bits
#define BITSTORE_BLOCK_INIT_MASK    0UL                           // mask to zero all bits of block


struct BitStore;
typedef struct BitStore BitStore;


static inline void BitStore_init(BitStore* this, fhgfs_bool setZero);
static inline void BitStore_initWithSizeAndReset(BitStore* this, unsigned size);
static inline BitStore* BitStore_construct(fhgfs_bool setZero);
static inline void BitStore_uninit(BitStore* this);
static inline void BitStore_destruct(BitStore* this);

extern void BitStore_setBit(BitStore* this, unsigned bitIndex, fhgfs_bool isSet);

extern void BitStore_setSize(BitStore* this, unsigned newSize);
extern void BitStore_clearBits(BitStore* this);

extern unsigned BitStore_serialLen(const BitStore* this);
extern unsigned BitStore_serialize(const BitStore* this, char* buf);
extern fhgfs_bool BitStore_deserializePreprocess(const char* buf, size_t bufLen,
   const char** outBitStoreStart, unsigned* outLen);
extern void BitStore_deserialize(BitStore* this, const char* buf, unsigned* outLen);

extern fhgfs_bool BitStore_compare(BitStore* this, BitStore* second);
extern void BitStore_copy(BitStore* this, BitStore* other);
extern void BitStore_copyThreadSafe(BitStore* this, const BitStore* other);

// private

extern void __BitStore_freeHigherBits(BitStore* this);

// public inliners

static inline fhgfs_bool BitStore_getBit(const BitStore* this, unsigned bitIndex);
static inline fhgfs_bool BitStore_getBitNonAtomic(BitStore* this, unsigned bitIndex);

static inline unsigned BitStore_getBitBlockIndex(unsigned bitIndex);
static inline unsigned BitStore_getBitIndexInBitBlock(unsigned bitIndex);
static inline unsigned BitStore_calculateBitBlockCount(unsigned size);
static inline bitstore_store_type BitStore_calculateBitMaskInBitBlock(unsigned bitIndexInBitBlock);


/**
 * A vector of bits.
 */
struct BitStore
{
   unsigned numBits; // max number of bits that this bitstore can hold
   bitstore_store_type lowerBits; // used to avoid overhead of extra alloc for higherBits
   bitstore_store_type* higherBits; // array, which is alloc'ed only when needed
};


void BitStore_init(BitStore* this, fhgfs_bool setZero)
{
   this->numBits = BITSTORE_BLOCK_BIT_COUNT;
   this->higherBits = NULL;

   if (setZero)
      this->lowerBits = BITSTORE_BLOCK_INIT_MASK;
}

void BitStore_initWithSizeAndReset(BitStore* this, unsigned size)
{
   BitStore_init(this, fhgfs_false);
   BitStore_setSize(this, size);
   BitStore_clearBits(this);
}

BitStore* BitStore_construct(fhgfs_bool setZero)
{
   struct BitStore* this = (BitStore*)os_kmalloc(sizeof(*this) );

   BitStore_init(this, setZero);

   return this;
}

void BitStore_uninit(BitStore* this)
{
   SAFE_KFREE_NOSET(this->higherBits);
}

void BitStore_destruct(BitStore* this)
{
   BitStore_uninit(this);

   os_kfree(this);
}

/**
 * Test whether the bit at the given index is set or not.
 *
 * Note: The bit operations in here are atomic.
 */
fhgfs_bool BitStore_getBit(const BitStore* this, unsigned bitIndex)
{
   unsigned index;
   unsigned indexInBitBlock;

   if(unlikely(bitIndex >= this->numBits) )
      return fhgfs_false;

   index = BitStore_getBitBlockIndex(bitIndex);
   indexInBitBlock = BitStore_getBitIndexInBitBlock(bitIndex);

   if (index == 0)
      return test_bit(indexInBitBlock, &this->lowerBits);
   else
      return test_bit(indexInBitBlock, &this->higherBits[index - 1]);
}

/**
 * Test whether the bit at the given index is set or not.
 *
 * Note: The bit operations in here are non-atomic.
 */
fhgfs_bool BitStore_getBitNonAtomic(BitStore* this, unsigned bitIndex)
{
   unsigned index;
   unsigned indexInBitBlock;
   bitstore_store_type mask;

   if(unlikely(bitIndex >= this->numBits) )
      return fhgfs_false;

   index = BitStore_getBitBlockIndex(bitIndex);
   indexInBitBlock = BitStore_getBitIndexInBitBlock(bitIndex);
   mask = BitStore_calculateBitMaskInBitBlock(indexInBitBlock);

   if (index == 0)
      return (this->lowerBits & mask) ? fhgfs_true : fhgfs_false;
   else
      return (this->higherBits[index - 1] & mask) ? fhgfs_true : fhgfs_false;
}


/**
 * returns the index of the bit block (array value) which contains the searched bit
 */
unsigned BitStore_getBitBlockIndex(unsigned bitIndex)
{
   return (bitIndex / BITSTORE_BLOCK_BIT_COUNT);
}

/**
 * returns the index in a bit block for the searched bit
 */
unsigned BitStore_getBitIndexInBitBlock(unsigned bitIndex)
{
   return (bitIndex % BITSTORE_BLOCK_BIT_COUNT);
}

/**
 * calculates the needed bit block count for the given BitStore size
 */
unsigned BitStore_calculateBitBlockCount(unsigned size)
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

/**
 * calculates the bit mask for selecting a bit in a bit block, is needed for AND or OR bit
 * operations with a bit block
 */
bitstore_store_type BitStore_calculateBitMaskInBitBlock(unsigned bitIndexInBitBlock)
{
   bitstore_store_type mask;

   if(unlikely(bitIndexInBitBlock >= BITSTORE_BLOCK_BIT_COUNT) )
      return BITSTORE_BLOCK_INIT_MASK;

   mask = (1UL << bitIndexInBitBlock);

   return mask;
}

#endif /* BITSTORE_H_ */

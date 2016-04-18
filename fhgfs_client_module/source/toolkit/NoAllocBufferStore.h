#ifndef NOALLOCBUFFERSTORE_H_
#define NOALLOCBUFFERSTORE_H_

#include <common/threading/Mutex.h>
#include <common/threading/Condition.h>
#include <common/threading/Thread.h>
#include <common/Common.h>

/**
 * This BufferStore does not use any kind of memory allocation during its normal
 * operation. However, it does (de)allocate the requested number of buffers
 * during (de)initialization.
 */

// Forward declaration only, as it contains a close source part
struct NoAllocBufferStore;
typedef struct NoAllocBufferStore NoAllocBufferStore;

extern void NoAllocBufferStore_init(NoAllocBufferStore* this, size_t numBufs, size_t bufSize);
extern NoAllocBufferStore* NoAllocBufferStore_construct(size_t numBufs, size_t bufSize);
extern void NoAllocBufferStore_uninit(NoAllocBufferStore* this);
extern void NoAllocBufferStore_destruct(NoAllocBufferStore* this);

extern char* NoAllocBufferStore_waitForBuf(NoAllocBufferStore* this);
extern char* NoAllocBufferStore_instantBuf(NoAllocBufferStore* this);
extern void NoAllocBufferStore_addBuf(NoAllocBufferStore* this, char* buf);

// getters & setters
extern size_t NoAllocBufferStore_getNumAvailable(NoAllocBufferStore* this);
extern size_t NoAllocBufferStore_getBufSize(NoAllocBufferStore* this);
extern fhgfs_bool NoAllocBufferStore_getValid(NoAllocBufferStore* this);

#endif /*NOALLOCBUFFERSTORE_H_*/

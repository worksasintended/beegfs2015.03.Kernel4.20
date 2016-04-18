#ifndef POLLLIST_H_
#define POLLLIST_H_

#include <common/net/sock/Socket.h>
#include <common/toolkit/tree/PointerRBTree.h>
#include <common/toolkit/SocketTk.h>
#include <common/Common.h>

struct PollList;
typedef struct PollList PollList;

static inline void PollList_init(PollList* this, unsigned minSizeHint);
static inline PollList* PollList_construct(unsigned minSizeHint);
static inline void PollList_uninit(PollList* this);
static inline void PollList_destruct(PollList* this);

extern void PollList_add(PollList* this, Socket* socket);
extern void PollList_getPollArray(PollList* this, PollSocket** pollArrayOut, unsigned* outLen);

static inline void PollList_remove(PollList* this, Socket* socket);

static inline void __PollList_updatePollArray(PollList* this);


struct PollList
{
   RBTree rbTree; // (keys: Socket*) (values: <none>) 

   struct pollfd* pollArray;
   unsigned arrayLen;
   unsigned minArrayLen;
   fhgfs_bool arrayOutdated;
};

/**
 * @param minSizeHint set this to the expected minimum number of poll sockets
 */
void PollList_init(PollList* this, unsigned minSizeHint)
{
   PointerRBTree_init(&this->rbTree);

   this->arrayOutdated = fhgfs_false;
   this->arrayLen = minSizeHint;
   this->minArrayLen = (minSizeHint ? minSizeHint : 1);
   this->pollArray = (PollSocket*)os_kmalloc(minSizeHint * sizeof(PollSocket) );
}

/**
 * @param numBufs number of buffers to be allocated
 * @param bufSize size of each allocated buffer
 */
struct PollList* PollList_construct(unsigned minSizeHint)
{
   struct PollList* this = (PollList*)os_kmalloc(sizeof(PollList) );
   
   PollList_init(this, minSizeHint);
   
   return this;
}

void PollList_uninit(PollList* this)
{
}

void PollList_destruct(PollList* this)
{
   PollList_uninit(this);
   
   os_kfree(this);
}



#endif /*POLLLIST_H_*/

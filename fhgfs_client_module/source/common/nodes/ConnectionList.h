#ifndef CONNECTIONLIST_H_
#define CONNECTIONLIST_H_

#include <common/Common.h>
#include <common/toolkit/list/PointerList.h>


// forward declaration
struct PooledSocket;

struct ConnectionList;
typedef struct ConnectionList ConnectionList;


static inline void ConnectionList_init(ConnectionList* this);
static inline ConnectionList* ConnectionList_construct(void);
static inline void ConnectionList_uninit(ConnectionList* this);
static inline void ConnectionList_destruct(ConnectionList* this);
static inline void ConnectionList_append(ConnectionList* this, struct PooledSocket* socket);
static inline size_t ConnectionList_length(ConnectionList* this);


struct ConnectionList
{
   PointerList pointerList;
};


void ConnectionList_init(ConnectionList* this)
{
   PointerList_init( (PointerList*)this);
}

struct ConnectionList* ConnectionList_construct(void)
{
   struct ConnectionList* this = (ConnectionList*)os_kmalloc(sizeof(ConnectionList) );

   ConnectionList_init(this);

   return this;
}

void ConnectionList_uninit(ConnectionList* this)
{
   PointerList_uninit( (PointerList*)this);
}

void ConnectionList_destruct(ConnectionList* this)
{
   ConnectionList_uninit(this);

   os_kfree(this);
}

void ConnectionList_append(ConnectionList* this, struct PooledSocket* socket)
{
   PointerList_append( (PointerList*)this, socket);
}

static inline size_t ConnectionList_length(ConnectionList* this)
{
   return PointerList_length( (PointerList*)this);
}


#endif /*CONNECTIONLIST_H_*/

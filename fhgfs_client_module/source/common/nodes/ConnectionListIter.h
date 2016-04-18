#ifndef CONNECTIONLISTITER_H_
#define CONNECTIONLISTITER_H_

#include <common/toolkit/list/PointerListIter.h>
#include "ConnectionList.h"

struct ConnectionListIter;
typedef struct ConnectionListIter ConnectionListIter;

static inline void ConnectionListIter_init(ConnectionListIter* this, ConnectionList* list);
static inline ConnectionListIter* ConnectionListIter_construct(ConnectionList* list);
static inline void ConnectionListIter_uninit(ConnectionListIter* this);
static inline void ConnectionListIter_destruct(ConnectionListIter* this);
static inline fhgfs_bool ConnectionListIter_hasNext(ConnectionListIter* this);
static inline void ConnectionListIter_next(ConnectionListIter* this);
static inline struct PooledSocket* ConnectionListIter_value(ConnectionListIter* this);
static inline fhgfs_bool ConnectionListIter_end(ConnectionListIter* this);
static inline ConnectionListIter ConnectionListIter_remove(ConnectionListIter* this);


struct ConnectionListIter
{
   PointerListIter pointerListIter;
};


void ConnectionListIter_init(ConnectionListIter* this, ConnectionList* list)
{
   PointerListIter_init( (PointerListIter*)this, (PointerList*)list);
}

ConnectionListIter* ConnectionListIter_construct(ConnectionList* list)
{
   ConnectionListIter* this = (ConnectionListIter*)os_kmalloc(
      sizeof(ConnectionListIter) );

   ConnectionListIter_init(this, list);

   return this;
}

void ConnectionListIter_uninit(ConnectionListIter* this)
{
   PointerListIter_uninit( (PointerListIter*)this);
}

void ConnectionListIter_destruct(ConnectionListIter* this)
{
   ConnectionListIter_uninit(this);

   os_kfree(this);
}

fhgfs_bool ConnectionListIter_hasNext(ConnectionListIter* this)
{
   return PointerListIter_hasNext( (PointerListIter*)this);
}

void ConnectionListIter_next(ConnectionListIter* this)
{
   PointerListIter_next( (PointerListIter*)this);
}

struct PooledSocket* ConnectionListIter_value(ConnectionListIter* this)
{
   return (struct PooledSocket*)PointerListIter_value( (PointerListIter*)this);
}

fhgfs_bool ConnectionListIter_end(ConnectionListIter* this)
{
   return PointerListIter_end( (PointerListIter*)this);
}

/**
 * note: the current iterator becomes invalid after the call (use the returned iterator)
 * @return the new iterator that points to the element just behind the erased one
 */
ConnectionListIter ConnectionListIter_remove(ConnectionListIter* this)
{
   ConnectionListIter newIter = *this;

   ConnectionListIter_next(&newIter); // the new iter that will be returned

   PointerListIter_remove( (PointerListIter*)this);

   return newIter;
}


#endif /*CONNECTIONLISTITER_H_*/

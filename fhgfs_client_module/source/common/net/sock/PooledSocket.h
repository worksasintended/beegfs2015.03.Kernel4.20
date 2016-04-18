#ifndef POOLEDSOCKET_H_
#define POOLEDSOCKET_H_

#include <common/net/sock/Socket.h>
#include <common/toolkit/Time.h>


struct PooledSocket;
typedef struct PooledSocket PooledSocket;


static inline void _PooledSocket_init(PooledSocket* this, NicAddrType_t nicType);
static inline PooledSocket* _PooledSocket_construct(NicAddrType_t nicType);
static inline void _PooledSocket_uninit(Socket* this);
static inline void _PooledSocket_destruct(Socket* this);

// inliners
static inline fhgfs_bool PooledSocket_getHasExpired(PooledSocket* this, unsigned expireSecs);

// getters & setters
static inline fhgfs_bool PooledSocket_isAvailable(PooledSocket* this);
static inline void PooledSocket_setAvailable(PooledSocket* this, fhgfs_bool available);
static inline fhgfs_bool PooledSocket_getHasActivity(PooledSocket* this);
static inline void PooledSocket_setHasActivity(PooledSocket* this);
static inline void PooledSocket_resetHasActivity(PooledSocket* this);
static inline fhgfs_bool PooledSocket_getHasExpirationTimer(PooledSocket* this);
static inline void PooledSocket_setExpireTimeStart(PooledSocket* this);
static inline NicAddrType_t PooledSocket_getNicType(PooledSocket* this);


/**
 * This class provides special extensions for sockets in a NodeConnPool.
 */
struct PooledSocket
{
   Socket socket;

   fhgfs_bool available; // == !acquired
   fhgfs_bool hasActivity; // true if channel was not idle (part of channel class in fhgfs_common)
   Time expireTimeStart; // 0 means "doesn't expire", otherwise time when conn was established
   NicAddrType_t nicType; // same as the interface for which this conn was established
};


void _PooledSocket_init(PooledSocket* this, NicAddrType_t nicType)
{
   _Socket_init( (Socket*)this);

   this->available = fhgfs_false;
   this->hasActivity = fhgfs_true; // initially active to avoid immediate disconnection
   Time_initZero(&this->expireTimeStart);
   this->nicType = nicType;
}


PooledSocket* _PooledSocket_construct(NicAddrType_t nicType)
{
   PooledSocket* this = (PooledSocket*)os_kmalloc(sizeof(*this) );

   _PooledSocket_init(this, nicType);

   return this;
}

void _PooledSocket_uninit(Socket* this)
{
   PooledSocket* thisCast = (PooledSocket*)this;

   Time_uninit(&thisCast->expireTimeStart);

   _Socket_uninit(this);
}

void _PooledSocket_destruct(Socket* this)
{
   _PooledSocket_uninit(this);

   os_kfree(this);
}

/**
 * Tests whether this socket is set to expire and whether its expire time has been exceeded.
 *
 * @param expireSecs the time in seconds after which an expire-enabled socket expires.
 * @return fhgfs_true if this socket has expired.
 */
fhgfs_bool PooledSocket_getHasExpired(PooledSocket* this, unsigned expireSecs)
{
   if(likely(Time_getIsZero(&this->expireTimeStart) ) )
      return fhgfs_false;

   if(Time_elapsedMS(&this->expireTimeStart) > (expireSecs*1000) ) // "*1000" for milliseconds
      return fhgfs_true;

   return fhgfs_false;
}

fhgfs_bool PooledSocket_isAvailable(PooledSocket* this)
{
   return this->available;
}

void PooledSocket_setAvailable(PooledSocket* this, fhgfs_bool available)
{
   this->available = available;
}

fhgfs_bool PooledSocket_getHasActivity(PooledSocket* this)
{
   return this->hasActivity;
}

void PooledSocket_setHasActivity(PooledSocket* this)
{
   this->hasActivity = fhgfs_true;
}

void PooledSocket_resetHasActivity(PooledSocket* this)
{
   this->hasActivity = fhgfs_false;
}

fhgfs_bool PooledSocket_getHasExpirationTimer(PooledSocket* this)
{
   return !Time_getIsZero(&this->expireTimeStart);
}

void PooledSocket_setExpireTimeStart(PooledSocket* this)
{
   Time_setToNow(&this->expireTimeStart);
}

NicAddrType_t PooledSocket_getNicType(PooledSocket* this)
{
   return this->nicType;
}

#endif /*POOLEDSOCKET_H_*/

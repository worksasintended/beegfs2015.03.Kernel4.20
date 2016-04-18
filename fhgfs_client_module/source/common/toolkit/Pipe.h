#ifndef PIPE_H_
#define PIPE_H_

#include <common/net/sock/Socket.h>
#include <common/Common.h>

struct Pipe;
typedef struct Pipe Pipe;

static inline void Pipe_init(Pipe* this);
static inline Pipe* Pipe_construct();
static inline void Pipe_uninit(Pipe* this);
static inline void Pipe_destruct(Pipe* this);

// getters & setters
static inline fhgfs_bool Pipe_getValid(Pipe* this);
static inline Socket* Pipe_getReadEnd(Pipe* this);
static inline Socket* Pipe_getWriteEnd(Pipe* this);



struct Pipe
{
   fhgfs_bool valid;
   
   Socket* readEnd;
   Socket* writeEnd;
};


void Pipe_init(Pipe* this)
{
   this->valid = Socket_createSocketPair(PF_UNIX, SOCK_STREAM, 0, &readEnd, &writeEnd);
   if(!this->valid)
   {
      this->readEnd = NULL;
      this->writeEnd = NULL;
   }
}

struct Pipe* Pipe_construct()
{
   struct Pipe* this = (Pipe*)os_kmalloc(sizeof(*this) );
   
   if(likely(this) )
      Pipe_init(this);
   
   return this;
}

void Pipe_uninit(Pipe* this)
{
   if(this->valid)
   {
      Socket_shutdown(*this->writeEnd);
   }
   
   SAFE_DESTRUCT(*this->writeEnd, Socket_destruct);
   SAFE_DESTRUCT(*this->readEnd, Socket_destruct);
}

void Pipe_destruct(Pipe* this)
{
   Pipe_uninit(this);
   
   os_kfree(this);
}

fhgfs_bool Pipe_getValid(Pipe* this)
{
   return this->valid;
}

Socket* Pipe_getReadEnd(Pipe* this)
{
   return this->readEnd;
}

Socket* Pipe_getWriteEnd(Pipe* this)
{
   return this->writeEnd;
}


#endif /*PIPE_H_*/

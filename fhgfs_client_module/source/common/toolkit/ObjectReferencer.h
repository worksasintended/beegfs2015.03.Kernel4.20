#ifndef OBJECTREFERENCER_H_
#define OBJECTREFERENCER_H_

#include <app/log/Logger.h>
#include <app/App.h>
#include <common/Common.h>

/*
 * Note: unlike the c++-ObjectReferencer, this version cannot own the referenced objects,
 * because it does not know how to destruct them. derived classes for specific object references
 * can handle this but will not notice derived referenced objects (so be careful)
 */

struct ObjectReferencer;
typedef struct ObjectReferencer ObjectReferencer;

static inline void ObjectReferencer_init(ObjectReferencer* this, void* referencedObject);
static inline ObjectReferencer* ObjectReferencer_construct(void* referencedObject);
static inline void ObjectReferencer_uninit(ObjectReferencer* this);
static inline void ObjectReferencer_destruct(ObjectReferencer* this);

static inline void* ObjectReferencer_reference(ObjectReferencer* this);
static inline int ObjectReferencer_release(ObjectReferencer* this, App* app);
static inline void* ObjectReferencer_getReferencedObject(ObjectReferencer* this);

// getters & setters
static inline int ObjectReferencer_getRefCount(ObjectReferencer* this);


struct ObjectReferencer
{
   int refCount;
   
   void* referencedObject;
};


void ObjectReferencer_init(ObjectReferencer* this, void* referencedObject)
{
   this->referencedObject = referencedObject;
   
   this->refCount = 0;
}

ObjectReferencer* ObjectReferencer_construct(void* referencedObject)
{
   ObjectReferencer* this = (ObjectReferencer*)os_kmalloc(sizeof(*this) );
   
   if(likely(this) )
      ObjectReferencer_init(this, referencedObject);
   
   return this;
}

void ObjectReferencer_uninit(ObjectReferencer* this)
{
   // nothing to be done here
}

void ObjectReferencer_destruct(ObjectReferencer* this)
{
   ObjectReferencer_uninit(this);
   
   os_kfree(this);
}

void* ObjectReferencer_reference(ObjectReferencer* this)
{
   this->refCount++;
   return this->referencedObject;
}

int ObjectReferencer_release(ObjectReferencer* this, App* app)
{
   #ifdef DEBUG_REFCOUNT
      if(!this->refCount)
      {
         Logger* log = App_getLogger(app);
         const char* logContext = "ObjectReferencer_release";
         
         Logger_logErr(log, logContext, "Bug: refCount is 0 and release() was called "
            "(dumping stack to kernel log...)");

         BEEGFS_BUG_ON(fhgfs_true, "refCount is 0 and release() was called");
         
         return 0;
      }
      else
         return --(this->refCount);
   #else
      return this->refCount ? --(this->refCount) : 0;
   #endif // DEBUG_REFCOUNT
}

/**
 * Be careful: This does not change the reference count!
 */
void* ObjectReferencer_getReferencedObject(ObjectReferencer* this)
{
   return this->referencedObject;
}

int ObjectReferencer_getRefCount(ObjectReferencer* this)
{
   return this->refCount;
}


#endif /*OBJECTREFERENCER_H_*/

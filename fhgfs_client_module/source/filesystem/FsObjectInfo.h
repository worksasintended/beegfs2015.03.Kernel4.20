#ifndef FSOBJECTINFO_H_
#define FSOBJECTINFO_H_

#include <common/Common.h>
#include <app/App.h>


enum FsObjectType
   {FsObjectType_DIRECTORY=1, FsObjectType_FILE=2};
typedef enum FsObjectType FsObjectType;

/**
 * Note: Consider this to be an abstract class. Mind the virtual function pointers.
 */

struct FsObjectInfo;
typedef struct FsObjectInfo FsObjectInfo;

static inline void FsObjectInfo_init(FsObjectInfo* this, App* app, FsObjectType objectType);
static inline FsObjectInfo* FsObjectInfo_construct(App* app, FsObjectType objectType);
static inline void FsObjectInfo_uninit(FsObjectInfo* this);
static inline void FsObjectInfo_destruct(FsObjectInfo* this);

static inline void FsObjectInfo_virtualDestruct(FsObjectInfo* this);


// getters & setters
static inline App* FsObjectInfo_getApp(FsObjectInfo* this);
static inline FsObjectType FsObjectInfo_getObjectType(FsObjectInfo* this);


struct FsObjectInfo
{
   App* app;
   FsObjectType objectType;

   // virtual functions
   void (*uninit) (FsObjectInfo* this);
};


void FsObjectInfo_init(FsObjectInfo* this, App* app, FsObjectType objectType)
{
   this->app = app;
   this->objectType = objectType;

   // clear virtual function pointer
   this->uninit = NULL;
}

struct FsObjectInfo* FsObjectInfo_construct(App* app, FsObjectType objectType)
{
   struct FsObjectInfo* this =
      (FsObjectInfo*)os_kmalloc(sizeof(*this) );

   if(likely(this) )
      FsObjectInfo_init(this, app, objectType);

   return this;
}

void FsObjectInfo_uninit(FsObjectInfo* this)
{
   // nothing to be done here
}

void FsObjectInfo_destruct(FsObjectInfo* this)
{
   FsObjectInfo_uninit(this);

   os_kfree(this);
}

/**
 * Calls the virtual uninit method and kfrees the object.
 */
void FsObjectInfo_virtualDestruct(FsObjectInfo* this)
{
   this->uninit(this);
   os_kfree(this);
}

App* FsObjectInfo_getApp(FsObjectInfo* this)
{
   return this->app;
}

FsObjectType FsObjectInfo_getObjectType(FsObjectInfo* this)
{
   return this->objectType;
}


#endif /*FSOBJECTINFO_H_*/

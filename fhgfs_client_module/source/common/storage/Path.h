#ifndef PATH_H_
#define PATH_H_

#include <common/Common.h>
#include <common/toolkit/StringTk.h>
#include <common/toolkit/list/StrCpyList.h>
#include <common/toolkit/list/StrCpyListIter.h>


struct Path;
typedef struct Path Path;

static inline void Path_init(Path* this);
static inline void Path_initFromString(Path* this, const char* pathStr);
static inline void Path_initFromPaths(Path* this, Path* parentPath, Path* childPath);
static inline Path* Path_construct(void);
static inline Path* Path_constructFromString(char* pathStr);
static inline Path* Path_constructFromPaths(Path* parentPath, Path* childPath);
static inline void Path_uninit(Path* this);
static inline void Path_destruct(Path* this);

static inline void Path_parseStr(Path* this, const char* pathStr);
static inline fhgfs_bool Path_isPathStrAbsolute(const char* pathStr);

static inline void Path_appendElem(Path* this, const char* newElem);

// getters & setters
static inline StrCpyList* Path_getPathElems(Path* this);
static inline char* Path_getPathAsStrCopy(Path* this);
static inline fhgfs_bool Path_isAbsolute(Path* this);
static inline void Path_setAbsolute(Path* this, fhgfs_bool absolute);
static inline const char* Path_getLastElem(Path* this);


struct Path
{
   StrCpyList pathElems;
   fhgfs_bool isPathAbsolute;
};

void Path_init(Path* this)
{
   StrCpyList_init(&this->pathElems);

   this->isPathAbsolute = fhgfs_false;
}

void Path_initFromString(Path* this, const char* pathStr)
{
   Path_init(this);

   this->isPathAbsolute = Path_isPathStrAbsolute(pathStr);
   Path_parseStr(this, pathStr);
}

void Path_initFromPaths(Path* this, Path* parentPath, Path* childPath)
{
   StrCpyListIter iter;

   this->isPathAbsolute = Path_isAbsolute(parentPath);

   // copy parentPath elems
   StrCpyListIter_init(&iter, Path_getPathElems(parentPath) );

   for( ; !StrCpyListIter_end(&iter); StrCpyListIter_next(&iter) )
   {
      char* currentPathElem = StrCpyListIter_value(&iter);
      StrCpyList_append(&this->pathElems, currentPathElem);
   }

   StrCpyListIter_uninit(&iter);


   // copy childPath elems
   StrCpyListIter_init(&iter, Path_getPathElems(childPath) );

   for( ; !StrCpyListIter_end(&iter); StrCpyListIter_next(&iter) )
   {
      char* currentPathElem = StrCpyListIter_value(&iter);
      StrCpyList_append(&this->pathElems, currentPathElem);
   }

   StrCpyListIter_uninit(&iter);
}

Path* Path_construct(void)
{
   struct Path* this = os_kmalloc(sizeof(struct Path) );

   Path_init(this);

   return this;
}

Path* Path_constructFromString(char* pathStr)
{
   struct Path* this = os_kmalloc(sizeof(struct Path) );

   Path_initFromString(this, pathStr);

   return this;
}

Path* Path_constructFromPaths(Path* parentPath, Path* childPath)
{
   struct Path* this = os_kmalloc(sizeof(struct Path) );

   Path_initFromPaths(this, parentPath, childPath);

   return this;
}

void Path_uninit(Path* this)
{
   StrCpyList_uninit(&this->pathElems);
}

void Path_destruct(Path* this)
{
   Path_uninit(this);

   os_kfree(this);
}

void Path_parseStr(Path* this, const char* pathStr)
{
   StringTk_explode(pathStr, '/', &this->pathElems);
}

fhgfs_bool Path_isPathStrAbsolute(const char* pathStr)
{
   return (os_strlen(pathStr) && (pathStr[0] == '/') );
}

void Path_appendElem(Path* this, const char* newElem)
{
   StrCpyList_append(&this->pathElems, newElem);
}

StrCpyList* Path_getPathElems(Path* this)
{
   return &this->pathElems;
}

/**
 * @return string does not end with a slash; string is kalloced and needs to be kfreed by the caller
 */
char* Path_getPathAsStrCopy(Path* this)
{
   char* pathStr;
   StrCpyListIter iter;
   size_t currentPathPos;
   size_t totalPathLen;

   // count total path length
   totalPathLen = Path_isAbsolute(this) ? 1 : 0;

   if(!StrCpyList_length(&this->pathElems) )
   { // (very unlikely)
      totalPathLen = 1; // for terminating zero
   }

   StrCpyListIter_init(&iter, &this->pathElems);

   for( ; !StrCpyListIter_end(&iter); StrCpyListIter_next(&iter) )
   {
      char* currentPathElem = StrCpyListIter_value(&iter);

      totalPathLen += os_strlen(currentPathElem) + 1; // +1 for slash or terminating zero
   }

   StrCpyListIter_uninit(&iter);


   // alloc path buffer
   pathStr = os_kmalloc(totalPathLen);


   // copy elems to path
   if(Path_isAbsolute(this) )
   {
      pathStr[0] = '/';
      currentPathPos = 1;
   }
   else
      currentPathPos = 0;


   StrCpyListIter_init(&iter, &this->pathElems);

   for( ; !StrCpyListIter_end(&iter); StrCpyListIter_next(&iter) )
   {
      char* currentPathElem = StrCpyListIter_value(&iter);
      size_t currentPathElemLen = os_strlen(currentPathElem);

      os_memcpy(&pathStr[currentPathPos], currentPathElem, currentPathElemLen);

      currentPathPos += currentPathElemLen;

      pathStr[currentPathPos] = '/';

      currentPathPos++;
   }

   StrCpyListIter_uninit(&iter);

   // zero-terminate the pathStr
   pathStr[totalPathLen-1] = 0;

   return pathStr;
}

fhgfs_bool Path_isAbsolute(Path* this)
{
   return this->isPathAbsolute;
}

void Path_setAbsolute(Path* this, fhgfs_bool absolute)
{
   this->isPathAbsolute = absolute;
}

const char* Path_getLastElem(Path* this)
{
   return StrCpyList_getTailValue(&this->pathElems);
}


#endif /*PATH_H_*/

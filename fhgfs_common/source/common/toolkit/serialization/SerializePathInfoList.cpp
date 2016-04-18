#include <common/storage/PathInfo.h>
#include "Serialization.h"

// =========== serializePathInfoList ===========

/**
 * Serialization of a PathInfoList.
 *
 * @return 0 on error, used buffer size otherwise
 */
unsigned Serialization::serializePathInfoList(char* buf, PathInfoList* pathInfoList)
{
   unsigned pathInfoListSize = pathInfoList->size();

   size_t bufPos = 0;

   // elem count info field

   bufPos += serializeUInt(&buf[bufPos], pathInfoListSize);

   // serialize each element of the pathInfoList

   PathInfoListIter iter = pathInfoList->begin();

   for ( unsigned i = 0; i < pathInfoListSize; i++, iter++ )
   {
      PathInfo pathInfo = *iter;

      bufPos += pathInfo.serialize(&buf[bufPos]);
   }

   return bufPos;
}

unsigned Serialization::serialLenPathInfoList(PathInfoList* pathInfoList)
{
   unsigned pathInfoListSize = pathInfoList->size();

   size_t bufPos = 0;

   bufPos += serialLenUInt();

   PathInfoListIter iter = pathInfoList->begin();

   for ( unsigned i = 0; i < pathInfoListSize; i++, iter++ )
   {
      PathInfo pathInfo = *iter;

      bufPos += pathInfo.serialLen();
   }

   return bufPos;
}

/**
 * Pre-processes a serialized PathInfoList for deserialization via deserializePathInfoList().
 *
 * @return false on error or inconsistency
 */
bool Serialization::deserializePathInfoListPreprocess(const char* buf, size_t bufLen,
   unsigned *outElemNum, const char** outInfoStart, unsigned* outLen)
{
   // Note: This is a fairly inefficient implementation (requiring redundant pre-processing),
   //    but efficiency doesn't matter for the typical use-cases of this method

   size_t bufPos = 0;

   // elem count info field
   unsigned elemNumFieldLen;

   if ( unlikely(!deserializeUInt(&buf[bufPos], bufLen-bufPos, outElemNum,
         &elemNumFieldLen) ) )
      return false;

   bufPos += elemNumFieldLen;

   *outInfoStart = &buf[bufPos];

   for ( unsigned i = 0; i < *outElemNum; i++ )
   {
      PathInfo pathInfo;
      unsigned pathInfoLen;

      if (unlikely(!pathInfo.deserialize(&buf[bufPos], bufLen-bufPos, &pathInfoLen)))
         return false;

      bufPos += pathInfoLen;
   }

   *outLen = bufPos;

   return true;
}

void Serialization::deserializePathInfoList(unsigned pathInfoListLen,
   unsigned pathInfoListElemNum, const char* pathInfoListStart, PathInfoList *outPathInfoList)
{
   const char* buf = pathInfoListStart;
   size_t bufPos = 0;
   size_t bufLen = ~0;

   for(unsigned i=0; i < pathInfoListElemNum; i++)
   {
      PathInfo pathInfo;
      unsigned pathInfoLen;

      pathInfo.deserialize(&buf[bufPos], bufLen-bufPos, &pathInfoLen);

      bufPos += pathInfoLen;

      outPathInfoList->push_back(pathInfo);
   }
}


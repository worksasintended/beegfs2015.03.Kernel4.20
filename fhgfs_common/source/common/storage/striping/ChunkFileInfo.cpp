#include <common/toolkit/serialization/Serialization.h>
#include "ChunkFileInfo.h"



unsigned ChunkFileInfo::serialize(char* buf) const
{
   size_t bufPos = 0;

   // dynAttribs
   bufPos += dynAttribs.serialize(&buf[bufPos]);

   // chunkSize
   bufPos += Serialization::serializeUInt(&buf[bufPos], this->chunkSize);

   // chunkSizeLog2
   bufPos += Serialization::serializeUInt(&buf[bufPos], this->chunkSizeLog2);

   // numCompleteChunks
   bufPos += Serialization::serializeInt64(&buf[bufPos], this->numCompleteChunks);

   return bufPos;
}

bool ChunkFileInfo::deserialize(const char* buf, size_t bufLen, unsigned* outLen)
{
   size_t bufPos = 0;

   {
      // dynAttribs
      unsigned dynAttribsLen;

      if (!dynAttribs.deserialize(&buf[bufPos], bufLen-bufPos, &dynAttribsLen) )
         return false;

      bufPos += dynAttribsLen;
   }

   {
      // chunkSize
      unsigned chunkSizeLen;

      if (!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &this->chunkSize,
         &chunkSizeLen) )
         return false;

      bufPos += chunkSizeLen;
   }

   {
      // chunkSizeLog2
      unsigned chunkSizeLog2Len;

      if (!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &this->chunkSizeLog2,
         &chunkSizeLog2Len) )
         return false;

      bufPos += chunkSizeLog2Len;
   }

   {
      // numCompleteChunks
      unsigned numCompleteChunksLen;

      if (!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos, &this->numCompleteChunks,
         &numCompleteChunksLen) )
         return false;

      bufPos += numCompleteChunksLen;
   }

   *outLen = bufPos;

   return true;
}

unsigned ChunkFileInfo::serialLen()
{
   unsigned len = 0;

   len += DynamicFileAttribs::serialLen();             // dynAttribs
   len += Serialization::serialLenUInt();              // chunkSize
   len += Serialization::serialLenUInt();              // chunkSizeLog2
   len += Serialization::serialLenInt64();             // numCompleteChunks

   return len;
}


unsigned ChunkFileInfo::serializeVec(char* buf, const ChunkFileInfoVec* vec)
{
   unsigned requiredLen = serialLenVec(vec);

   unsigned vecSize = vec->size();

   size_t bufPos = 0;

   // totalBufLen info field

   bufPos += Serialization::serializeUInt(&buf[bufPos], requiredLen);

   // elem count info field

   bufPos += Serialization::serializeUInt(&buf[bufPos], vecSize);


   // store each element of the vec as a raw zero-terminated string

   ChunkFileInfoVecCIter iter = vec->begin();

   for(unsigned i=0; i < vecSize; i++, iter++)
      bufPos += iter->serialize(&buf[bufPos]);

   return requiredLen;
}

bool ChunkFileInfo::deserializeVecPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outVecStart, unsigned* outLen)
{
   size_t bufPos = 0;

   // totalBufLen info field
   unsigned bufLenFieldLen;
   if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, outLen, &bufLenFieldLen) )
      return false;
   bufPos += bufLenFieldLen;

   // elem count field
   unsigned elemNumFieldLen;
   if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, outElemNum, &elemNumFieldLen) )
      return false;
   bufPos += elemNumFieldLen;

   *outVecStart = &buf[bufPos];

   if(unlikely(
       (*outLen > bufLen) ||
       ( (*outLen - bufPos) != (*outElemNum * ChunkFileInfo::serialLen() ) ) ) )
      return false;

   return true;
}

bool ChunkFileInfo::deserializeVec(unsigned vecBufLen, unsigned elemNum, const char* vecStart,
   ChunkFileInfoVec* outVec)
{
   const char* buf = vecStart;
   size_t bufPos = 0;
   size_t bufLen = ~0;

   for(unsigned i=0; i < elemNum; i++)
   {
      ChunkFileInfo currentElem;
      unsigned chunkFileInfoLen = 0; // just mute compiler warnings on sl5

      if ( unlikely(!currentElem.deserialize(&buf[bufPos], bufLen-bufPos, &chunkFileInfoLen) ) )
         return false;

      bufPos += chunkFileInfoLen;

      outVec->push_back(currentElem);
   }

   return true;
}

unsigned ChunkFileInfo::serialLenVec(const ChunkFileInfoVec* vec)
{
   // bufLen-field + numElems-field
   unsigned requiredLen = Serialization::serialLenUInt() + Serialization::serialLenUInt();

   for(ChunkFileInfoVecCIter iter = vec->begin(); iter != vec->end(); iter++)
      requiredLen += iter->serialLen();

   return requiredLen;
}

bool chunkFileInfoEquals(const ChunkFileInfo& first, const ChunkFileInfo& second)
{
   // dynAttribs
   if(!dynamicFileAttribsEquals(first.dynAttribs, second.dynAttribs) )
      return false;

   // chunkSize
   if(first.chunkSize != second.chunkSize)
      return false;

   // chunkSizeLog2
   if(first.chunkSizeLog2 != second.chunkSizeLog2)
      return false;

   // numCompleteChunks
   if(first.numCompleteChunks != second.numCompleteChunks)
      return false;

   return true;
}

bool ChunkFileInfo::chunkFileInfoVecEquals(const ChunkFileInfoVec& first,
   const ChunkFileInfoVec& second)
{
   if(first.size() != second.size() )
      return false;

   ChunkFileInfoVecCIter firstIter = first.begin();
   ChunkFileInfoVecCIter secondIter = second.begin();
   for(; firstIter != first.end(); firstIter++, secondIter++)
   {
      if(!chunkFileInfoEquals(*firstIter, *secondIter) )
         return false;
   }

   return true;
}

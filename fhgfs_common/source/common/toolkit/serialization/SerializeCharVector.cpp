/*
 * CharVector (de)serialization methods
 */

#include "Serialization.h"
#include <algorithm>

/**
 * @return 0 on error (e.g. vecLen is greater than bufLen), used buffer size otherwise
 */
unsigned Serialization::serializeCharVector(char* buf, CharVector* vec)
{
   unsigned requiredLen = serialLenCharVector(vec);

   unsigned vectorSize = vec->size();

   size_t bufPos = 0;

   // totalBufLen info field
   bufPos += serializeUInt(&buf[bufPos], requiredLen);

   // elem count info field
   bufPos += serializeUInt(&buf[bufPos], vectorSize);

   // copy raw data
   std::copy(vec->begin(), vec->end(), &buf[bufPos]);
   bufPos += vectorSize;

   return requiredLen;
}

/**
 * Pre-processes a serialized CharVector.
 *
 * @return false on error or inconsistency
 */
bool Serialization::deserializeCharVectorPreprocess(const char* buf, size_t bufLen,
      unsigned* outElemNum, const char** outVecStart, unsigned* outVecBufLen)
{
   size_t bufPos = 0;

   // totalBufLen info field
   unsigned bufLenFieldLen;
   if(unlikely(!deserializeUInt(&buf[bufPos], bufLen-bufPos, outVecBufLen, &bufLenFieldLen) ) )
      return false;
   bufPos += bufLenFieldLen;

   // elem count info field
   unsigned elemNumFieldLen;
   if(unlikely(!deserializeUInt(&buf[bufPos], bufLen-bufPos, outElemNum, &elemNumFieldLen) ) )
      return false;
   bufPos += elemNumFieldLen;

   *outVecStart = &buf[bufPos];

   if(unlikely(
         (*outVecBufLen > bufLen) ||
         ( (*outVecBufLen - bufPos) != (*outElemNum * serialLenChar() ) ) ) )
      return false;

   return true;
}

/*
 * Deserializes a CharVector.
 * (requires pre-processing)
 *
 * @return false on error or inconsistency.
 */
bool Serialization::deserializeCharVector(unsigned vecBufLen, unsigned elemNum,
      const char* vecStart, CharVector* outVec)
{
   unsigned elemsBufLen =
         vecBufLen - serialLenUInt() - serialLenUInt(); // bufLenField and numElemsField

   size_t bufPos = 0;

   if(unlikely(elemsBufLen != serialLenChar() * elemNum) )
      return false;

   // copy raw data
   outVec->resize(elemNum);
   std::copy(vecStart, vecStart + elemNum, outVec->begin() );

   bufPos += elemsBufLen;

   return true;
}

unsigned Serialization::serialLenCharVector(CharVector* vec)
{
   // bufLen field + numElems field + numElems * elemSize
   return serialLenUInt() + serialLenUInt() + vec->size() * serialLenChar();
}


/*
 * Short Int (de)serialization methods
 *
 */

#include "Serialization.h"


// =========== serializeUShortVector ===========

/**
 * Serialize an unsigned short vector
 */
unsigned Serialization::serializeUShortVector(char* buf, UShortVector* vec)
{
   unsigned requiredLen = serialLenUShortVector(vec);
   unsigned listSize = vec->size();

   size_t bufPos = 0;

   // totalBufLen info field
   bufPos += serializeUInt(&buf[bufPos], requiredLen);

   // elem count info field
   bufPos += serializeUInt(&buf[bufPos], listSize);

   for (UShortVectorConstIter iter=vec->begin(); iter != vec->end(); iter++)
   {
      bufPos += serializeUShort(&buf[bufPos], *iter);
   }

   return requiredLen;
}

/**
 * Pre-processes a serialized UShortVec().
 *
 * @return false on error or inconsistency
 */
bool Serialization::deserializeUShortVectorPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outVecStart, unsigned* outLen)
{
   size_t bufPos = 0;

   // totalBufLen info field
   unsigned bufLenFieldLen;
   if(unlikely(!deserializeUInt(&buf[bufPos], bufLen-bufPos, outLen, &bufLenFieldLen) ) )
      return false;
   bufPos += bufLenFieldLen;

   // elem count field
   unsigned elemNumFieldLen;
   if(unlikely(!deserializeUInt(&buf[bufPos], bufLen-bufPos, outElemNum, &elemNumFieldLen) ) )
      return false;
   bufPos += elemNumFieldLen;

   *outVecStart = &buf[bufPos];

   if(unlikely(
      (*outLen > bufLen) ||
      ( (*outLen - bufPos) != (*outElemNum * serialLenUShort() ) ) ) )
      return false;

   return true;
}

/**
 * Deserializes an UShortVec.
 * (requires pre-processing)
 *
 * @return false on error or inconsistency
 */
bool Serialization::deserializeUShortVector(unsigned vecBufLen, unsigned elemNum,
   const char* vecStart, UShortVector* outVec)
{
   unsigned elemsBufLen =
      vecBufLen - serialLenUInt() - serialLenUInt(); // bufLenField & numElemsField

   size_t bufPos = 0;

   outVec->reserve(elemNum);

   // read each list element
   for(unsigned i=0; i < elemNum; i++)
   {
      unsigned short value;
      unsigned valueLen = 0;

      if(unlikely(!deserializeUShort(&vecStart[bufPos], elemsBufLen-bufPos, &value, &valueLen) ) )
         return false;

      bufPos += valueLen;

      outVec->push_back(value);
   }

   return true;
}

unsigned Serialization::serialLenUShortVector(UShortVector* vec)
{
   // bufLen-field + numElems-field + numElems*elemSize
   unsigned requiredLen = serialLenUInt() + serialLenUInt() + vec->size()*serialLenUShort();

   return requiredLen;
}

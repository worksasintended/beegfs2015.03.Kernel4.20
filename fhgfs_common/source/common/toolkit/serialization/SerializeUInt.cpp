/*
 * UInt (de)serialization methods
 */

#include "Serialization.h"


// =========== list ===========

/**
 * Serialization of a UIntList.
 */
unsigned Serialization::serializeUIntList(char* buf, UIntList* list)
{
   unsigned requiredLen = serialLenUIntList(list);

   unsigned listSize = list->size();

   size_t bufPos = 0;

   // totalBufLen info field

   bufPos += serializeUInt(&buf[bufPos], requiredLen);

   // elem count info field

   bufPos += serializeUInt(&buf[bufPos], listSize);


   // store each element of the list

   UIntListConstIter iter = list->begin();

   for(unsigned i=0; i < listSize; i++, iter++)
   {
      bufPos += serializeUInt(&buf[bufPos], *iter);
   }

   return requiredLen;
}

/**
 * Pre-processes a serialized UIntList().
 *
 * @return false on error or inconsistency
 */
bool Serialization::deserializeUIntListPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outListBufLen)
{
   size_t bufPos = 0;

   // totalBufLen info field
   unsigned bufLenFieldLen;
   if(unlikely(!deserializeUInt(&buf[bufPos], bufLen-bufPos, outListBufLen, &bufLenFieldLen) ) )
      return false;
   bufPos += bufLenFieldLen;

   // elem count field
   unsigned elemNumFieldLen;
   if(unlikely(!deserializeUInt(&buf[bufPos], bufLen-bufPos, outElemNum, &elemNumFieldLen) ) )
      return false;
   bufPos += elemNumFieldLen;

   *outListStart = &buf[bufPos];

   if(unlikely(
      (*outListBufLen > bufLen) ||
      ( (*outListBufLen - bufPos) != (*outElemNum * serialLenUInt() ) ) ) )
      return false;

   return true;
}

/**
 * Deserializes a UIntList.
 * (requires pre-processing)
 *
 * @return false on error or inconsistency
 */
bool Serialization::deserializeUIntList(unsigned listBufLen, unsigned elemNum,
   const char* listStart, UIntList* outList)
{
   unsigned elemsBufLen =
      listBufLen - serialLenUInt() - serialLenUInt(); // bufLenField & numElemsField

   size_t bufPos = 0;

   // read each list element

   for(unsigned i=0; i < elemNum; i++)
   {
      unsigned value = 0;
      unsigned valueLen = 0;

      if(unlikely(!deserializeUInt(&listStart[bufPos], elemsBufLen-bufPos, &value, &valueLen) ) )
         return false;

      bufPos += valueLen;

      outList->push_back(value);
   }

   // check whether all of the elements were read (=consistency)
   return true;
}

unsigned Serialization::serialLenUIntList(UIntList* list)
{
   // bufLen-field + numElems-field + numElems*elemSize
   unsigned requiredLen = serialLenUInt() + serialLenUInt() + list->size()*serialLenUInt();

   return requiredLen;
}



// =========== vector ===========

/**
 * Serialize an integer vector
 */
unsigned Serialization::serializeUIntVector(char* buf, UIntVector* vec)
{
   unsigned requiredLen = serialLenUIntVector(vec);

   unsigned listSize = vec->size();

   size_t bufPos = 0;

   // totalBufLen info field
   bufPos += serializeUInt(&buf[bufPos], requiredLen);

   // elem count info field
   bufPos += serializeUInt(&buf[bufPos], listSize);

   for (UIntVectorConstIter iter=vec->begin(); iter != vec->end(); iter++)
   {
      bufPos += serializeUInt(&buf[bufPos], *iter);
   }

   return requiredLen;
}

/**
 * Pre-processes a serialized UIntVec().
 *
 * @return false on error or inconsistency
 */
bool Serialization::deserializeUIntVectorPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outVecStart, unsigned* outLen)
{
   return Serialization::deserializeUIntListPreprocess(buf, bufLen, outElemNum, outVecStart, outLen);
}

/**
 * Deserializes an UIntVec.
 * (requires pre-processing)
 *
 * @return false on error or inconsistency
 */
bool Serialization::deserializeUIntVector(unsigned listBufLen, unsigned elemNum,
   const char* listStart, UIntVector* outVec)
{
   unsigned elemsBufLen =
      listBufLen - serialLenUInt() - serialLenUInt(); // bufLenField & numElemsField

   size_t bufPos = 0;

   outVec->reserve(elemNum);

   // read each list element
   for(unsigned i=0; i < elemNum; i++)
   {
      unsigned value;
      unsigned valueLen = 0;

      if(unlikely(!deserializeUInt(&listStart[bufPos], elemsBufLen-bufPos, &value, &valueLen) ) )
         return false;

      bufPos += valueLen;

      outVec->push_back(value);
   }

   return true;
}

unsigned Serialization::serialLenUIntVector(UIntVector* vec)
{
   // bufLen-field + numElems-field + numElems*elemSize
   unsigned requiredLen = serialLenUInt() + serialLenUInt() + vec->size()*serialLenUInt();

   return requiredLen;
}


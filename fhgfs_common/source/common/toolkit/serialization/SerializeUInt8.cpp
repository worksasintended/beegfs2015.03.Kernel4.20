/*
 * UInt8 (de)serialization methods
 */

#include "Serialization.h"


// =========== list ===========

/**
 * Serialization of an UInt8List.
 */
unsigned Serialization::serializeUInt8List(char* buf, UInt8List* list)
{
   unsigned requiredLen = serialLenUInt8List(list);

   unsigned listSize = list->size();

   size_t bufPos = 0;

   // totalBufLen info field
   bufPos += serializeUInt(&buf[bufPos], requiredLen);

   // elem count info field
   bufPos += serializeUInt(&buf[bufPos], listSize);

   // store each element of the list
   for(UInt8ListConstIter iter=list->begin(); iter != list->end(); iter++)
   {
      bufPos += serializeUInt8(&buf[bufPos], *iter);
   }

   return requiredLen;
}

/**
 * Pre-processes a serialized UInt8List().
 *
 * @return false on error or inconsistency
 */
bool Serialization::deserializeUInt8ListPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen)
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

   *outListStart = &buf[bufPos];

   if(unlikely(
      (*outLen > bufLen) ||
      ( (*outLen - bufPos) != (*outElemNum * serialLenUInt8() ) ) ) )
      return false;

   return true;
}

/**
 * Deserializes an UInt8List.
 * (requires pre-processing)
 *
 * @return false on error or inconsistency
 */
bool Serialization::deserializeUInt8List(unsigned listBufLen, unsigned elemNum,
   const char* listStart, UInt8List* outList)
{
   unsigned elemsBufLen =
      listBufLen - serialLenUInt() - serialLenUInt(); // bufLenField & numElemsField

   size_t bufPos = 0;

   for(unsigned i=0; i < elemNum; i++)
   { // read each list element
      uint8_t value = 0; // initilization to prevent wrong compiler warning (unused variable)
      unsigned valueLen = 0;

      if(unlikely(!deserializeUInt8(&listStart[bufPos], elemsBufLen-bufPos, &value, &valueLen) ) )
         return false;

      bufPos += valueLen;

      outList->push_back(value);
   }

   return true;
}

unsigned Serialization::serialLenUInt8List(UInt8List* list)
{
   // bufLen-field + numElems-field + numElems*elemSize
   unsigned requiredLen = serialLenUInt() + serialLenUInt() + list->size()*serialLenUInt8();

   return requiredLen;
}

/**
 * Calculate the size required for serialization. Here we already provide the list size,
 * as std::list->size() needs to iterate over the entire list to compute the size, which is
 * terribly slow, of course.
 */
unsigned Serialization::serialLenUInt8List(size_t size)
{
   // bufLen-field + numElems-field + numElems*elemSize
   unsigned requiredLen = serialLenUInt() + serialLenUInt() + size * serialLenUInt8();

   return requiredLen;
}


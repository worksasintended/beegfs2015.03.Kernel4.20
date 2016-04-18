/*
 * BoolList (de)serialization methods
 */

#include "Serialization.h"

// =========== list ===========

/**
 * Serialization of a BoolList.
 */
unsigned Serialization::serializeBoolList(char* buf, BoolList* list)
{
   unsigned requiredLen = serialLenBoolList(list);

   unsigned listSize = list->size();

   size_t bufPos = 0;

   // totalBufLen info field

   bufPos += serializeUInt(&buf[bufPos], requiredLen);

   // elem count info field

   bufPos += serializeUInt(&buf[bufPos], listSize);

   // store each element of the list

   BoolListConstIter iter = list->begin();

   for(unsigned i=0; i < listSize; i++, iter++)
   {
      bufPos += serializeBool(&buf[bufPos], *iter);
   }

   return requiredLen;
}

/**
 * Pre-processes a serialized BoolList().
 *
 * @return false on error or inconsistency
 */
bool Serialization::deserializeBoolListPreprocess(const char* buf, size_t bufLen,
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
      ( (*outListBufLen - bufPos) != (*outElemNum * serialLenBool() ) ) ) )
      return false;

   return true;
}

/**
 * Deserializes a BoolList.
 * (requires pre-processing)
 *
 * @return false on error or inconsistency
 */
bool Serialization::deserializeBoolList(unsigned listBufLen, unsigned elemNum,
   const char* listStart, BoolList* outList)
{
   unsigned elemsBufLen =
      listBufLen - serialLenUInt() - serialLenUInt(); // bufLenField & numElemsField

   size_t bufPos = 0;

   // read each list element

   for(unsigned i=0; i < elemNum; i++)
   {
      bool value;
      unsigned valueLen = 0;

      if(unlikely(!deserializeBool(&listStart[bufPos], elemsBufLen-bufPos, &value, &valueLen) ) )
         return false;

      bufPos += valueLen;

      outList->push_back(value);
   }

   // check whether all of the elements were read (=consistency)
   return true;
}

unsigned Serialization::serialLenBoolList(BoolList* list)
{
   // bufLen-field + numElems-field + numElems*elemSize
   unsigned requiredLen = serialLenUInt() + serialLenUInt() + list->size()*serialLenBool();

   return requiredLen;
}

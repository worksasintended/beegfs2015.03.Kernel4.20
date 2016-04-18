/*
 * String (de)serialization methods
 *
 */

#include <common/app/log/LogContext.h>
#include "Serialization.h"


// =========== strings ===========

/**
 * @return 0 on error (e.g. strLen is greater than bufLen), used buffer size otherwise
 */
unsigned Serialization::serializeStr(char* buf, unsigned strLen, const char* strStart)
{
   size_t bufPos = 0;

   // write length field
   bufPos += serializeUInt(&buf[bufPos], strLen);

   // write raw string
   memcpy(&buf[bufPos], strStart, strLen);
   bufPos += strLen;

   // write termination char
   buf[bufPos] = 0;

   return serialLenStr(strLen);
}

/**
 * @return false on error (e.g. strLen is greater than bufLen)
 */
bool Serialization::deserializeStr(const char* buf, size_t bufLen,
   unsigned* outStrLen, const char** outStrStart, unsigned* outLen)
{
   // check min length
   if(unlikely(bufLen < serialLenStr(0) ) )
      return false;

   size_t bufPos = 0;

   // length field
   unsigned strLenFieldLen = 0;
   deserializeUInt(&buf[bufPos], bufLen-bufPos, outStrLen, &strLenFieldLen);
   bufPos += strLenFieldLen;

   // string start
   *outStrStart = &buf[bufPos];
   bufPos += *outStrLen;

   // required outLen
   *outLen = serialLenStr(*outStrLen);

   // check length and terminating zero
   if(unlikely( (bufLen < (*outLen) ) || ( (*outStrStart)[*outStrLen] != 0) ) )
      return false;

   return true;
}

/**
 * This version deserializes into a std::string.
 */
bool Serialization::deserializeStr(const char* buf, size_t bufLen,
   std::string* outStr, unsigned* outBufLen)
{
   unsigned strLen;
   const char* strStart;

   bool deserRes = Serialization::deserializeStr(buf, bufLen,
         &strLen, &strStart, outBufLen);
   if(unlikely(!deserRes) )
      return false;

   outStr->reserve(strLen);
   outStr->assign(strStart, strLen);

   return true;
}

unsigned Serialization::serialLenStr(unsigned strLen)
{
   // strLenField + str + terminating zero
   return serialLenUInt() + strLen + 1;
}



// =========== strings with 4-byte aligned padding ===========


/**
 * Note: Adds padding to achieve 4-byte alignment. Requires calling deserializeStrAlign4 for
 * deserialization.
 *
 * @return 0 on error (e.g. strLen is greater than bufLen), used buffer size otherwise
 */
unsigned Serialization::serializeStrAlign4(char* buf, unsigned strLen, const char* strStart)
{
   size_t bufPos = 0;

   // write length field
   bufPos += serializeUInt(&buf[bufPos], strLen);

   // write raw string
   memcpy(&buf[bufPos], strStart, strLen);
   bufPos += strLen;

   // write termination char
   buf[bufPos] = 0;
   bufPos++;

   unsigned alignedLen = serialLenStrAlign4(strLen);

   #ifdef BEEGFS_DEBUG
      unsigned zeroLen = alignedLen - bufPos; // bufPos == rawLen of serialLenStrAlign4()
      if (zeroLen)
      {
         memset(&buf[bufPos], 0, zeroLen);
         bufPos += zeroLen;
      }
   #endif

   return alignedLen;

}

/**
 * @return false on error (e.g. strLen is greater than bufLen)
 */
bool Serialization::deserializeStrAlign4(const char* buf, size_t bufLen,
   unsigned* outStrLen, const char** outStrStart, unsigned* outLen)
{
   const char* logContext = "Deserialize String (align4)";

   // check min length
   if(unlikely(bufLen < serialLenStrAlign4(0) ) )
   {
      LogContext(logContext).logErr("Error: bufLen smaller than minimum");
      return false;
   }

   size_t bufPos = 0;

   { // length field
      unsigned strLenFieldLen = 0;
      deserializeUInt(&buf[bufPos], bufLen-bufPos, outStrLen, &strLenFieldLen);
      bufPos += strLenFieldLen;
   }

   { // string start
      *outStrStart = &buf[bufPos];
      bufPos += *outStrLen;
   }

   { // required outLen (incl alignment padding)
      *outLen = serialLenStrAlign4(*outStrLen);
   }

   // check length
   if(unlikely(bufLen < (*outLen) ) )
   {
      LogContext(logContext).logErr("Error: bufLen smaller than serialization length. " +
         std::string("bufLen: ")  + StringTk::uintToStr((unsigned) bufLen) +
         std::string(" outLen: ") + StringTk::uintToStr(*outLen) );
      return false;
   }

   // check terminating zero
   if (unlikely( (*outStrStart)[*outStrLen] != 0 ) )
   {
      LogContext(logContext).logErr("Error: String not terminated");
      return false;
   }

   return true;
}

/**
 * This version deserializes into a std::string.
 */
bool Serialization::deserializeStrAlign4(const char* buf, size_t bufLen,
   std::string* outStr, unsigned* outBufLen)
{
   unsigned strLen;
   const char* strStart;

   bool deserRes = Serialization::deserializeStrAlign4(buf, bufLen,
         &strLen, &strStart, outBufLen);
   if(unlikely(!deserRes) )
      return false;

   outStr->reserve(strLen);
   outStr->assign(strStart, strLen);

   return true;
}

unsigned Serialization::serialLenStrAlign4(unsigned strLen)
{
   // strLenField + str + terminating zero
   unsigned rawLen = serialLenUInt() + strLen + 1;
   unsigned remainder = rawLen % 4;
   unsigned alignedLen = remainder ? (rawLen + 4 - remainder) : rawLen;

   return alignedLen;
}



// =========== list ===========


/**
 * Serialization of a StringList.
 *
 * Note: We keep serialization format compatible with stringvectors.
 */
unsigned Serialization::serializeStringList(char* buf, const StringList* list)
{
   unsigned requiredLen = serialLenStringList(list);

   unsigned listSize = list->size();

   size_t bufPos = 0;

   // totalBufLen info field

   bufPos += serializeUInt(&buf[bufPos], requiredLen);

   // elem count info field

   bufPos += serializeUInt(&buf[bufPos], listSize);


   // store each element of the list as a raw zero-terminated string

   StringListConstIter iter = list->begin();

   for(unsigned i=0; i < listSize; i++, iter++)
   {
      int serialElemLen = iter->size() + 1; // +1 for the terminating zero

      memcpy(&buf[bufPos], iter->c_str(), serialElemLen);

      bufPos += serialElemLen;
   }

   return requiredLen;
}

/**
 * Pre-processes a serialized StringList().
 *
 * @return false on error or inconsistency
 */
bool Serialization::deserializeStringListPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen)
{
   size_t bufPos = 0;

   // totalBufLen info field
   unsigned bufLenFieldLen;
   if(!deserializeUInt(&buf[bufPos], bufLen-bufPos, outLen, &bufLenFieldLen) )
      return false;
   bufPos += bufLenFieldLen;

   // elem count field
   unsigned elemNumFieldLen;
   if(!deserializeUInt(&buf[bufPos], bufLen-bufPos, outElemNum, &elemNumFieldLen) )
      return false;
   bufPos += elemNumFieldLen;

   *outListStart = &buf[bufPos];

   const char* outListEnd = &buf[*outLen] - 1;

   if(unlikely(
       (*outLen > bufLen) ||
       (*outLen < (serialLenUInt() + serialLenUInt() ) ) || // bufLenField + numElemsField
       ( *outElemNum && (*outListEnd != 0) ) ) )
      return false;

   return true;
}

/**
 * Deserializes a StringList.
 * (requires pre-processing)
 *
 * @return false on error or inconsistency
 */
bool Serialization::deserializeStringList(unsigned listBufLen, unsigned elemNum,
   const char* listStart, StringList* outList)
{
   const char* currentPos = listStart;
   ssize_t elemsBufLen =
      listBufLen - serialLenUInt() - serialLenUInt(); // bufLenField & numElemsField
   const char* listEndPos = listStart + elemsBufLen - 1;

   unsigned lastElemLen;


   // read each list element as a raw zero-terminated string
   // and make sure that we do not read beyond the specified end position

   unsigned i=0;
   for( ; (i < elemNum) && (currentPos <= listEndPos); i++, currentPos += lastElemLen)
   {
      std::string currentElem(currentPos);

      outList->push_back(currentElem);

      lastElemLen = currentElem.size() + 1; // +1 for the terminating zero
   }

   // check whether all of the elements were read (=consistency)
   return ( (i==elemNum) && (currentPos > listEndPos) );
}



unsigned Serialization::serialLenStringList(const StringList* list)
{
   // bufLen-field + numElems-field
   unsigned requiredLen = serialLenUInt() + serialLenUInt();

   for(StringListConstIter iter = list->begin(); iter != list->end(); iter++)
   {
      requiredLen += iter->size() + 1; // +1 for the terminating zero
   }

   return requiredLen;
}


// =========== vector ===========


/**
 * Note: We keep serialization format compatible with stringlists.
 */
unsigned Serialization::serializeStringVec(char* buf, StringVector* vec)
{
   unsigned requiredLen = serialLenStringVec(vec);

   unsigned listSize = vec->size();

   size_t bufPos = 0;

   // totalBufLen info field

   bufPos += serializeUInt(&buf[bufPos], requiredLen);

   // elem count info field

   bufPos += serializeUInt(&buf[bufPos], listSize);


   // store each element of the list as a raw zero-terminated string

   StringVectorConstIter iter = vec->begin();

   for(unsigned i=0; i < listSize; i++, iter++)
   {
      int serialElemLen = iter->size() + 1; // +1 for the terminating zero

      memcpy(&buf[bufPos], iter->c_str(), serialElemLen);

      bufPos += serialElemLen;
   }

   return requiredLen;
}

bool Serialization::deserializeStringVecPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen)
{
   size_t bufPos = 0;

   // totalBufLen info field
   unsigned bufLenFieldLen;
   if(!deserializeUInt(&buf[bufPos], bufLen-bufPos, outLen, &bufLenFieldLen) )
      return false;
   bufPos += bufLenFieldLen;

   // elem count field
   unsigned elemNumFieldLen;
   if(!deserializeUInt(&buf[bufPos], bufLen-bufPos, outElemNum, &elemNumFieldLen) )
      return false;
   bufPos += elemNumFieldLen;

   *outListStart = &buf[bufPos];

   const char* outListEnd = &buf[*outLen] - 1;

   if(unlikely(
       (*outLen > bufLen) ||
       ( *outElemNum && (*outListEnd != 0) ) ) )
      return false;

   return true;
}

bool Serialization::deserializeStringVec(unsigned listBufLen, unsigned elemNum,
   const char* listStart, StringVector* outVec)
{
   const char* currentPos = listStart;
   unsigned elemsBufLen =
      listBufLen - serialLenUInt() - serialLenUInt(); // bufLenField & numElemsField
   const char* listEndPos = listStart + elemsBufLen - 1;

   unsigned lastElemLen;

   outVec->reserve(elemNum);

   // read each list element as a raw zero-terminated string
   // and make sure that we do not read beyond the specified end position

   unsigned i=0;
   for( ; (i < elemNum) && (currentPos <= listEndPos); i++, currentPos += lastElemLen)
   {
      std::string currentElem(currentPos);

      outVec->push_back(currentElem);

      lastElemLen = currentElem.size() + 1; // +1 for the terminating zero
   }

   // check whether all of the elements were read (=consistency)
   return ( (i==elemNum) && (currentPos > listEndPos) );
}

unsigned Serialization::serialLenStringVec(StringVector* vec)
{
   // bufLen-field + numElems-field
   unsigned requiredLen = serialLenUInt() + serialLenUInt();

   for(StringVectorConstIter iter = vec->begin(); iter != vec->end(); iter++)
   {
      requiredLen += iter->size() + 1; // +1 for the terminating zero
   }

   return requiredLen;
}


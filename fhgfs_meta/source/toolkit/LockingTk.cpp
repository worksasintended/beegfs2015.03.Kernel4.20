#include <common/toolkit/serialization/Serialization.h>
#include "LockingTk.h"



unsigned LockingTk::serializeEntryLockDetailsList(char* buf, const EntryLockDetailsList* list)
{
   unsigned requiredLen = serialLenEntryLockDetailsList(list);

   unsigned listSize = list->size();

   size_t bufPos = 0;

   // totalBufLen info field

   bufPos += Serialization::serializeUInt(&buf[bufPos], requiredLen);

   // elem count info field

   bufPos += Serialization::serializeUInt(&buf[bufPos], listSize);

   // store each element of the list

   EntryLockDetailsListCIter iter = list->begin();

   for(unsigned i=0; i < listSize; i++, iter++)
      bufPos += iter->serialize(&buf[bufPos]);

   return requiredLen;
}

bool LockingTk::deserializeEntryLockDetailsListPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen)
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

   *outListStart = &buf[bufPos];

   if(unlikely(
       (*outLen > bufLen) ||
       (*outLen < (Serialization::serialLenUInt() + Serialization::serialLenUInt() ) ) ) )
      return false;

   return true;
}

bool LockingTk::deserializeEntryLockDetailsList(unsigned listBufLen, unsigned elemNum,
   const char* listStart, EntryLockDetailsList* list)
{
   const char* buf = listStart;
   size_t bufPos = 0;
   size_t bufLen = ~0;

   for(unsigned i=0; i < elemNum; i++)
   {
      EntryLockDetails currentElem;
      unsigned entryLockDetailsLen;

      if ( unlikely(!currentElem.deserialize(&buf[bufPos], bufLen-bufPos, &entryLockDetailsLen) ) )
         return false;

      bufPos += entryLockDetailsLen;

      list->push_back(currentElem);
   }

   return true;
}

unsigned LockingTk::serialLenEntryLockDetailsList(const EntryLockDetailsList* list)
{
   // bufLen-field + numElems-field
   unsigned requiredLen = Serialization::serialLenUInt() + Serialization::serialLenUInt();

   for(EntryLockDetailsListCIter iter = list->begin(); iter != list->end(); iter++)
      requiredLen += iter->serialLen();

   return requiredLen;
}



unsigned LockingTk::serializeEntryLockDetailsSet(char* buf, const EntryLockDetailsSet* set)
{
   unsigned requiredLen = serialLenEntryLockDetailsSet(set);

   unsigned setSize = set->size();

   size_t bufPos = 0;

   // totalBufLen info field

   bufPos += Serialization::serializeUInt(&buf[bufPos], requiredLen);

   // elem count info field

   bufPos += Serialization::serializeUInt(&buf[bufPos], setSize);

   // store each element of the set

   EntryLockDetailsSetCIter iter = set->begin();

   for(unsigned i=0; i < setSize; i++, iter++)
      bufPos += iter->serialize(&buf[bufPos]);

   return requiredLen;
}

bool LockingTk::deserializeEntryLockDetailsSetPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outSetStart, unsigned* outLen)
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

   *outSetStart = &buf[bufPos];

   if(unlikely(
       (*outLen > bufLen) ||
       (*outLen < (Serialization::serialLenUInt() + Serialization::serialLenUInt() ) ) ) )
      return false;

   return true;
}

bool LockingTk::deserializeEntryLockDetailsSet(unsigned setBufLen, unsigned elemNum,
   const char* setStart, EntryLockDetailsSet* set)
{
   const char* buf = setStart;
   size_t bufPos = 0;
   size_t bufLen = ~0;

   for(unsigned i=0; i < elemNum; i++)
   {
      EntryLockDetails currentElem;
      unsigned entryLockDetailsLen;

      if ( unlikely(!currentElem.deserialize(&buf[bufPos], bufLen-bufPos, &entryLockDetailsLen) ) )
         return false;

      bufPos += entryLockDetailsLen;

      set->insert(currentElem);
   }

   return true;
}

unsigned LockingTk::serialLenEntryLockDetailsSet(const EntryLockDetailsSet* set)
{
   // bufLen-field + numElems-field
   unsigned requiredLen = Serialization::serialLenUInt() + Serialization::serialLenUInt();

   for(EntryLockDetailsSetCIter iter = set->begin(); iter != set->end(); iter++)
      requiredLen += iter->serialLen();

   return requiredLen;
}



unsigned LockingTk::serializeRangeLockDetailsList(char* buf, const RangeLockDetailsList* list)
{
   unsigned requiredLen = serialLenRangeLockDetailsList(list);

   unsigned listSize = list->size();

   size_t bufPos = 0;

   // totalBufLen info field

   bufPos += Serialization::serializeUInt(&buf[bufPos], requiredLen);

   // elem count info field

   bufPos += Serialization::serializeUInt(&buf[bufPos], listSize);

   // store each element of the list

   RangeLockDetailsListCIter iter = list->begin();

   for(unsigned i=0; i < listSize; i++, iter++)
      bufPos += iter->serialize(&buf[bufPos]);

   return bufPos;
}

bool LockingTk::deserializeRangeLockDetailsListPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen)
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

   *outListStart = &buf[bufPos];

   if(unlikely(
       (*outLen > bufLen) ||
       (*outLen < (Serialization::serialLenUInt() + Serialization::serialLenUInt() ) ) ) )
      return false;

   return true;
}

bool LockingTk::deserializeRangeLockDetailsList(unsigned listBufLen, unsigned elemNum,
   const char* listStart, RangeLockDetailsList* list)
{
   const char* buf = listStart;
   size_t bufPos = 0;
   size_t bufLen = ~0;

   for(unsigned i=0; i < elemNum; i++)
   {
      RangeLockDetails currentElem;
      unsigned rangeLockDetailsLen;

      if ( unlikely(!currentElem.deserialize(&buf[bufPos], bufLen-bufPos, &rangeLockDetailsLen) ) )
         return false;

      bufPos += rangeLockDetailsLen;

      list->push_back(currentElem);
   }

   return true;
}

unsigned LockingTk::serialLenRangeLockDetailsList(const RangeLockDetailsList* list)
{
   // bufLen-field + numElems-field
   unsigned requiredLen = Serialization::serialLenUInt() + Serialization::serialLenUInt();

   for(RangeLockDetailsListCIter iter = list->begin(); iter != list->end(); iter++)
      requiredLen += iter->serialLen();

   return requiredLen;
}



unsigned LockingTk::serializeRangeLockExclSet(char* buf, const RangeLockExclSet* set)
{
   unsigned requiredLen = serialLenRangeLockExclSet(set);

   unsigned setSize = set->size();

   size_t bufPos = 0;

   // totalBufLen info field

   bufPos += Serialization::serializeUInt(&buf[bufPos], requiredLen);

   // elem count info field

   bufPos += Serialization::serializeUInt(&buf[bufPos], setSize);

   // store each element of the set

   RangeLockExclSetCIter iter = set->begin();

   for(unsigned i=0; i < setSize; i++, iter++)
      bufPos += iter->serialize(&buf[bufPos]);

   return bufPos;
}

bool LockingTk::deserializeRangeLockExclSetPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outSetStart, unsigned* outLen)
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

   *outSetStart = &buf[bufPos];

   if(unlikely(
       (*outLen > bufLen) ||
       (*outLen < (Serialization::serialLenUInt() + Serialization::serialLenUInt() ) ) ) )
      return false;

   return true;
}

bool LockingTk::deserializeRangeLockExclSet(unsigned setBufLen, unsigned elemNum,
   const char* setStart, RangeLockExclSet* set)
{
   const char* buf = setStart;
   size_t bufPos = 0;
   size_t bufLen = ~0;

   for(unsigned i=0; i < elemNum; i++)
   {
      RangeLockDetails currentElem;
      unsigned rangeLockDetailsLen;

      if ( unlikely(!currentElem.deserialize(&buf[bufPos], bufLen-bufPos, &rangeLockDetailsLen) ) )
         return false;

      bufPos += rangeLockDetailsLen;

      set->insert(currentElem);
   }

   return true;
}

unsigned LockingTk::serialLenRangeLockExclSet(const RangeLockExclSet* set)
{
   // bufLen-field + numElems-field
   unsigned requiredLen = Serialization::serialLenUInt() + Serialization::serialLenUInt();

   for(RangeLockExclSetCIter iter = set->begin(); iter != set->end(); iter++)
      requiredLen += iter->serialLen();

   return requiredLen;
}



unsigned LockingTk::serializeRangeLockSharedSet(char* buf, const RangeLockSharedSet* set)
{
   unsigned requiredLen = serialLenRangeLockSharedSet(set);

   unsigned setSize = set->size();

   size_t bufPos = 0;

   // totalBufLen info field

   bufPos += Serialization::serializeUInt(&buf[bufPos], requiredLen);

   // elem count info field

   bufPos += Serialization::serializeUInt(&buf[bufPos], setSize);

   // store each element of the set

   RangeLockSharedSetCIter iter = set->begin();

   for(unsigned i=0; i < setSize; i++, iter++)
      bufPos += iter->serialize(&buf[bufPos]);

   return bufPos;
}

bool LockingTk::deserializeRangeLockSharedSetPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outSetStart, unsigned* outLen)
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

   *outSetStart = &buf[bufPos];

   if(unlikely(
       (*outLen > bufLen) ||
       (*outLen < (Serialization::serialLenUInt() + Serialization::serialLenUInt() ) ) ) )
      return false;

   return true;
}

bool LockingTk::deserializeRangeLockSharedSet(unsigned setBufLen, unsigned elemNum,
   const char* setStart, RangeLockSharedSet* set)
{
   const char* buf = setStart;
   size_t bufPos = 0;
   size_t bufLen = ~0;

   for(unsigned i=0; i < elemNum; i++)
   {
      RangeLockDetails currentElem;
      unsigned rangeLockDetailsLen;

      if ( unlikely(!currentElem.deserialize(&buf[bufPos], bufLen-bufPos, &rangeLockDetailsLen) ) )
         return false;

      bufPos += rangeLockDetailsLen;

      set->insert(currentElem);
   }

   return true;
}

unsigned LockingTk::serialLenRangeLockSharedSet(const RangeLockSharedSet* set)
{
   // bufLen-field + numElems-field
   unsigned requiredLen = Serialization::serialLenUInt() + Serialization::serialLenUInt();

   for(RangeLockSharedSetCIter iter = set->begin(); iter != set->end(); iter++)
      requiredLen += iter->serialLen();

   return requiredLen;
}



bool LockingTk::entryLockDetailsListEquals(const EntryLockDetailsList& first,
   const EntryLockDetailsList& second)
{
   if(first.size() != second.size() )
      return false;

   EntryLockDetailsListCIter firstIter = first.begin();
   EntryLockDetailsListCIter secondIter = second.begin();
   for(; firstIter != first.end(); firstIter++, secondIter++)
   {
      if(!entryLockDetailsEquals(*firstIter, *secondIter) )
         return false;
   }

   return true;
}

bool LockingTk::entryLockDetailsSetEquals(const EntryLockDetailsSet& first,
   const EntryLockDetailsSet& second)
{
   if(first.size() != second.size() )
      return false;

   EntryLockDetailsSetCIter firstIter = first.begin();
   EntryLockDetailsSetCIter secondIter = second.begin();
   for(; firstIter != first.end(); firstIter++, secondIter++)
   {
      if(!entryLockDetailsEquals(*firstIter, *secondIter) )
         return false;
   }

   return true;
}

bool LockingTk::rangeLockDetailsListEquals(const RangeLockDetailsList& first,
   const RangeLockDetailsList& second)
{
   if(first.size() != second.size() )
      return false;

   RangeLockDetailsListCIter firstIter = first.begin();
   RangeLockDetailsListCIter secondIter = second.begin();
   for(; firstIter != first.end(); firstIter++, secondIter++)
   {
      if(!rangeLockDetailsEquals(*firstIter, *secondIter) )
         return false;
   }

   return true;
}

bool LockingTk::rangeLockExclSetEquals(const RangeLockExclSet& first,
   const RangeLockExclSet& second)
{
   if(first.size() != second.size() )
      return false;

   RangeLockExclSetCIter firstIter = first.begin();
   RangeLockExclSetCIter secondIter = second.begin();
   for(; firstIter != first.end(); firstIter++, secondIter++)
   {
      if(!rangeLockDetailsEquals(*firstIter, *secondIter) )
         return false;
   }

   return true;
}

bool LockingTk::rangeLockSharedSetEquals(const RangeLockSharedSet& first,
   const RangeLockSharedSet& second)
{
   if(first.size() != second.size() )
      return false;

   RangeLockSharedSetCIter firstIter = first.begin();
   RangeLockSharedSetCIter secondIter = second.begin();
   for(; firstIter != first.end(); firstIter++, secondIter++)
   {
      if(!rangeLockDetailsEquals(*firstIter, *secondIter) )
         return false;
   }

   return true;
}

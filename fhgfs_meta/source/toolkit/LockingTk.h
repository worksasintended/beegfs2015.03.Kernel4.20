#ifndef TOOLKIT_LOCKINGTK_H_
#define TOOLKIT_LOCKINGTK_H_


#include <storage/Locking.h>



class LockingTk
{
   public:
      static unsigned serializeEntryLockDetailsList(char* buf, const EntryLockDetailsList* list);
      static bool deserializeEntryLockDetailsListPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outListStart, unsigned* outLen);
      static bool deserializeEntryLockDetailsList(unsigned listBufLen, unsigned elemNum,
         const char* listStart, EntryLockDetailsList* list);
      static unsigned serialLenEntryLockDetailsList(const EntryLockDetailsList* list);

      static unsigned serializeEntryLockDetailsSet(char* buf, const EntryLockDetailsSet* set);
      static bool deserializeEntryLockDetailsSetPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outSetStart, unsigned* outLen);
      static bool deserializeEntryLockDetailsSet(unsigned setBufLen, unsigned elemNum,
         const char* setStart, EntryLockDetailsSet* set);
      static unsigned serialLenEntryLockDetailsSet(const EntryLockDetailsSet* set);

      static unsigned serializeRangeLockDetailsList(char* buf, const RangeLockDetailsList* list);
      static bool deserializeRangeLockDetailsListPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outListStart, unsigned* outListBufLen);
      static bool deserializeRangeLockDetailsList(unsigned listBufLen, unsigned elemNum,
         const char* listStart, RangeLockDetailsList* list);
      static unsigned serialLenRangeLockDetailsList(const RangeLockDetailsList* list);

      static unsigned serializeRangeLockExclSet(char* buf, const RangeLockExclSet* list);
      static bool deserializeRangeLockExclSetPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outListStart, unsigned* outListBufLen);
      static bool deserializeRangeLockExclSet(unsigned listBufLen, unsigned elemNum,
         const char* listStart, RangeLockExclSet* list);
      static unsigned serialLenRangeLockExclSet(const RangeLockExclSet* list);

      static unsigned serializeRangeLockSharedSet(char* buf, const RangeLockSharedSet* list);
      static bool deserializeRangeLockSharedSetPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outListStart, unsigned* outListBufLen);
      static bool deserializeRangeLockSharedSet(unsigned listBufLen, unsigned elemNum,
         const char* listStart, RangeLockSharedSet* list);
      static unsigned serialLenRangeLockSharedSet(const RangeLockSharedSet* list);

      static bool entryLockDetailsListEquals(const EntryLockDetailsList& first,
         const EntryLockDetailsList& second);
      static bool entryLockDetailsSetEquals(const EntryLockDetailsSet& first,
         const EntryLockDetailsSet& second);
      static bool rangeLockDetailsListEquals(const RangeLockDetailsList& first,
         const RangeLockDetailsList& second);
      static bool rangeLockExclSetEquals(const RangeLockExclSet& first,
         const RangeLockExclSet& second);
      static bool rangeLockSharedSetEquals(const RangeLockSharedSet& first,
         const RangeLockSharedSet& second);

   private:
      LockingTk() {};
};

#endif /* TOOLKIT_LOCKINGTK_H_ */

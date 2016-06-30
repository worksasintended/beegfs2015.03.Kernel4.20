#ifndef STRINGTK_H_
#define STRINGTK_H_

#include <common/Common.h>
#include <common/storage/StorageErrors.h>
#include <common/toolkit/BufferTk.h>


#define STRINGTK_STRING_NOT_FOUND_RET (std::string::npos)
#define STRINGTK_ID_SEPARATOR "-"


class StringTk
{
   public:
      // manipulation
      static std::string trim(const std::string s);
      static void explode(const std::string s, char delimiter, StringList* outList);
      static void explode(const std::string s, char delimiter, StringVector* outVec);
      static void explodeEx(const std::string s, char delimiter, bool useTrim, StringList* outList);
      static void explodeEx(const std::string s, char delimiter, bool useTrim, StringVector* outVec);
      static std::string implode(char delimiter, StringList& inList, bool useTrim);

      // transformation
      static int strToInt(const char* s);
      static unsigned strToUInt(const char* s);
      static unsigned strHexToUInt(const char* s);
      static unsigned strOctalToUInt(const char* s);
      static int64_t strToInt64(const char* s);
      static uint64_t strToUInt64(const char* s);
      static bool strToBool(const char* s);
      static std::string boolToStr(bool b);
      static std::string intToStr(int a);
      static std::string uintToStr(unsigned a);
      static std::string uintToHexStr(unsigned a);
      static std::string uint16ToHexStr(uint16_t a);
      static std::string int64ToStr(int64_t a);
      static std::string uint64ToHexStr(uint64_t a);
      static std::string uint64ToStr(uint64_t a);
      static std::string doubleToStr(double a);
      static std::string doubleToStr(double a, int precision);
      static std::string timespanToStr(int64_t timespan_secs);

      static std::string uint16VecToStr(const UInt16Vector* vec);
      static void strToUint16Vec(std::string& s, UInt16Vector* outVec);

      // inspection
      static bool isNumeric(const std::string testString);

      static bool stringSetEquals(const StringSet& first, const StringSet& second);

      static void genRandomAlphaNumericString(std::string& inOutString, const int length);



      // inline wrappers
      static int strToInt(const std::string s)
      {
         return strToInt(s.c_str() );
      }

      static unsigned strToUInt(const std::string s)
      {
         return strToUInt(s.c_str() );
      }

      static unsigned strOctalToUInt(const std::string s)
      {
         return strOctalToUInt(s.c_str() );
      }

      static unsigned strHexToUInt(const std::string s)
      {
         return strHexToUInt(s.c_str() );
      }

      static int64_t strToInt64(const std::string s)
      {
         return strToInt64(s.c_str() );
      }

      static bool strToBool(const std::string s)
      {
         return strToBool(s.c_str() );
      }

      static double strToDouble(const std::string s)
      {
         return atof(s.c_str() );
      }

   private:
      StringTk() {}

   public:
      // inliners
      static char* strncpyTerminated(char* dest, const char* src, size_t count)
      {
         // Note: The problem with strncpy() is that dest is not guaranteed to be zero-terminated.

         size_t srcLen = strlen(src);

         if(likely(count) )
         {
            size_t copyLen = (srcLen >= count) ? (count - 1) : srcLen;

            memcpy(dest, src, copyLen);
            dest[copyLen] = '\0';
         }

         return dest;
      }

      /**
       * Calculate the hash of the string.
       *
       * @param buf    - string to caculate the hash for
       * @param bufLen - length of buf
       */
      static unsigned strChecksum(const char* buf, int bufLen)
      {
         // Note: We use a 32bit checksum here to get the same result (fast) on x86 and x86_64

         return BufferTk::hash32(buf, bufLen);
      }


      /**
       * Get the time-stamp part from an EntryID
       */
      static FhgfsOpsErr timeStampFromEntryID(const std::string& inEntryID, std::string &outEntryID)
      {
         size_t firstSepPos = inEntryID.find(STRINGTK_ID_SEPARATOR);
         if (firstSepPos == (size_t) STRINGTK_STRING_NOT_FOUND_RET)
         {  // special entryID, such as 'root' and 'lost+found', we return it as it is
            outEntryID = inEntryID;
            return FhgfsOpsErr_SUCCESS;
         }

         size_t secondSepPos = inEntryID.find(STRINGTK_ID_SEPARATOR, firstSepPos + 1);
         if (secondSepPos == (size_t) STRINGTK_STRING_NOT_FOUND_RET)
         {  // That should not happen, return it as it is, but the caller should log an error
            outEntryID = inEntryID;
            return FhgfsOpsErr_INTERNAL;
         }

         // +1 and -1 to remove the separator '-'
         outEntryID = inEntryID.substr(firstSepPos + 1, secondSepPos - firstSepPos - 1);

         return FhgfsOpsErr_SUCCESS;
      }



};

#endif /*STRINGTK_H_*/

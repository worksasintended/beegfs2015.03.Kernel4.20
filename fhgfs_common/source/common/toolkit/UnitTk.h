#ifndef UNITTK_H_
#define UNITTK_H_

#include <common/Common.h>
#include <common/storage/quota/QuotaBlockDevice.h>


class UnitTk
{
   public:
      static int64_t petabyteToByte(double megabyte);
      static int64_t terabyteToByte(double kilobyte);
      static int64_t gigabyteToByte(double gigabyte);
      static int64_t megabyteToByte(double megabyte);
      static int64_t kilobyteToByte(double kilobyte);
      static double byteToMegabyte(double byte);
      static double byteToXbyte(int64_t bytes, std::string *outUnit);
      static double byteToXbyte(int64_t bytes, std::string *outUnit, bool round);
      static double megabyteToXbyte(int64_t megabyte, std::string *unit);
      static double megabyteToXbyte(int64_t megabyte, std::string *unit, bool round);
      static int64_t xbyteToByte(double xbyte, std::string unit);

      static int64_t strHumanToInt64(const char* s);
      static std::string int64ToHumanStr(int64_t a);
      static bool isValidHumanString(std::string humanString);

      static int64_t timeStrHumanToInt64(const char* s);
      static bool isValidHumanTimeString(std::string humanString);
      
      static uint64_t quotaBlockCountToByte(uint64_t quotaBlockCount, QuotaBlockDeviceFsType type);
      static std::string quotaBlockCountToHumanStr(uint64_t quotaBlockCount,
         QuotaBlockDeviceFsType type);

   private:
      UnitTk() {};
      
   
   public:
      // inliners
      static int64_t strHumanToInt64(std::string s)
      {
         return strHumanToInt64(s.c_str() );
      }

      static int64_t timeStrHumanToInt64(std::string s)
      {
         return timeStrHumanToInt64(s.c_str() );
      }
};


#endif /* UNITTK_H_ */

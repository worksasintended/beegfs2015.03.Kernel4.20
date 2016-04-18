#include <common/toolkit/StringTk.h>
#include "UnitTk.h"

#include <math.h>


int64_t UnitTk::petabyteToByte(double petabyte)
{
   int64_t b = (int64_t) (petabyte*1024*1024*1024*1024*1024);
   return b;
}

int64_t UnitTk::terabyteToByte(double petabyte)
{
   int64_t b = (int64_t) (petabyte*1024*1024*1024*1024);
   return b;
}

int64_t UnitTk::gigabyteToByte(double gigabyte)
{
	int64_t b = (int64_t) (gigabyte*1024*1024*1024);
	return b;
}

int64_t UnitTk::megabyteToByte(double megabyte)
{
	int64_t b = (int64_t) (megabyte*1024*1024);
   return b;
}

int64_t UnitTk::kilobyteToByte(double kilobyte)
{
	int64_t b = (int64_t) (kilobyte*1024);
	return b;
}

double UnitTk::byteToMegabyte(double byte)
{
  return (byte/1024/1024);
}

double UnitTk::byteToXbyte(int64_t bytes, std::string *outUnit)
{
   return byteToXbyte(bytes, outUnit, false);
}

/**
 * Convert number of bytes without a unit to the number of bytes with a unit (e.g. MB for megabyte).
 */
double UnitTk::byteToXbyte(int64_t bytes, std::string *outUnit, bool round)
{
	double res = bytes;

	short count = 0;
	while ( (res > 1024) && (count < 6) )
	{
		res = res/1024;
		count++;
	}

   switch(count)
   {
      case 0:
         *outUnit = "Byte";
         break;
      case 1:
         *outUnit = "KB";
         break;
      case 2:
         *outUnit = "MB";
         break;
      case 3:
         *outUnit = "GB";
         break;
      case 4:
         *outUnit = "TB";
         break;
      case 5:
         *outUnit = "PB";
         break;
      case 6:
         *outUnit = "EB";
         break;
   }

   if(round)
   {
      res = floor(res * 10 + 0.5) / 10;
   }

   return res;
}

double UnitTk::megabyteToXbyte(int64_t megabyte, std::string *unit)
{
   return megabyteToXbyte(megabyte, unit, false);
}

double UnitTk::megabyteToXbyte(int64_t megabyte, std::string *unit, bool round)
{
   double res = megabyte;

   short count = 0;
   while (res > 1024 && count<4)
   {
      res = (double)res/1024;
      count++;
   }

   switch(count)
   {
      case 0:
         *unit = "MB";
         break;
      case 1:
         *unit = "GB";
         break;
      case 2:
         *unit = "TB";
         break;
      case 3:
         *unit = "PB";
         break;
      case 4:
         *unit = "EB";
         break;
   }

   if(round)
   {
      res = floor(res * 10 + 0.5) / 10;
   }

   return res;
}

/**
 * Convert number of bytes with a given unit (e.g. megabytes) to the number of bytes without a unit.
 */
int64_t UnitTk::xbyteToByte(double xbyte, std::string unit)
{
	double res = xbyte;

	if (unit == "KB")
	{
		res = kilobyteToByte(res);
   }
	else
	if (unit == "MB")
	{
		res = megabyteToByte(res);
   }
	else
	if (unit == "GB")
	{
		res = gigabyteToByte(res);
   }
   else
   if (unit == "TB")
   {
      res = terabyteToByte(res);
   }
   else
   if (unit == "PB")
   {
      res = petabyteToByte(res);
   }

	return (int64_t)res;
}

/**
 * Transforms an integer with a unit appended to its non-human equivalent,
 * e.g. "1K" will return 1024.
 */
int64_t UnitTk::strHumanToInt64(const char* s)
{
   int64_t oneKilo = 1024;
   int64_t oneMeg  = oneKilo * 1024;
   int64_t oneGig  = oneMeg * 1024;
   int64_t oneTera = oneGig * 1024;
   int64_t onePeta = oneTera * 1024;

   size_t sLen = strlen(s);

   if(sLen < 2)
      return StringTk::strToInt64(s);

   char unit = s[sLen-1];

   if( (unit == 'P') || (unit == 'p') )
   {
      std::string sNoUnit = s;
      sNoUnit.resize(sLen-1);

      return StringTk::strToInt64(sNoUnit) * onePeta;
   }
   else
   if( (unit == 'T') || (unit == 't') )
   {
      std::string sNoUnit = s;
      sNoUnit.resize(sLen-1);

      return StringTk::strToInt64(sNoUnit) * oneTera;
   }
   else
   if( (unit == 'G') || (unit == 'g') )
   {
      std::string sNoUnit = s;
      sNoUnit.resize(sLen-1);

      return StringTk::strToInt64(sNoUnit) * oneGig;
   }
   else
   if( (unit == 'M') || (unit == 'm') )
   {
      std::string sNoUnit = s;
      sNoUnit.resize(sLen-1);

      return StringTk::strToInt64(sNoUnit) * oneMeg;
   }
   if( (unit == 'K') || (unit == 'k') )
   {
      std::string sNoUnit = s;
      sNoUnit.resize(sLen-1);

      return StringTk::strToInt64(sNoUnit) * oneKilo;
   }
   else
      return StringTk::strToInt64(s);
}


/**
 * Prints the number with a unit appended, but only if it matches without a remainder
 * (e.g. 1025 will not be printed as 1k)
 */
std::string UnitTk::int64ToHumanStr(int64_t a)
{
   long long oneKilo = 1024;
   long long oneMeg  = oneKilo * 1024;
   long long oneGig  = oneMeg * 1024;
   long long oneTera = oneGig * 1024;
   long long onePeta = oneTera * 1024;
   long long oneExa = onePeta * 1024;

   char aStr[24];

   if( ( (a >= oneExa) || (-a >= oneExa) ) &&
       ( (a % oneExa) == 0) )
   {
      sprintf(aStr, "%qdE", a / oneExa);
   }
   else
   if( ( (a >= onePeta) || (-a >= onePeta) ) &&
       ( (a % onePeta) == 0) )
   {
      sprintf(aStr, "%qdP", a / onePeta);
   }
   else
   if( ( (a >= oneTera) || (-a >= oneTera) ) &&
       ( (a % oneTera) == 0) )
   {
      sprintf(aStr, "%qdT", a / oneTera);
   }
   else
   if( ( (a >= oneGig) || (-a >= oneGig) ) &&
       ( (a % oneGig) == 0) )
   {
      sprintf(aStr, "%qdG", a / oneGig);
   }
   else
   if( ( (a >= oneMeg) || (-a >= oneMeg) ) &&
       ( (a % oneMeg) == 0) )
   {
      sprintf(aStr, "%qdM", a / oneMeg);
   }
   else
   if( ( (a >= oneKilo) || (-a >= oneKilo) ) &&
       ( (a % oneKilo) == 0) )
   {
      sprintf(aStr, "%qdK", a / oneKilo);
   }
   else
      sprintf(aStr, "%qd", (long long)a);

   return aStr;
}

/**
 * checks if the given human string is valid an can processed by strHumanToInt64()
 *
 * @param humanString the human string to validate
 */
bool UnitTk::isValidHumanString(std::string humanString)
{
   if(StringTk::isNumeric(humanString))
      return true;

   size_t humanStringLen = humanString.length();
   char unit = humanString.at(humanStringLen-1);
   std::string value = humanString.substr(0, humanStringLen-1);

   if( StringTk::isNumeric(value) && ( (unit == 'E') || (unit == 'e') ||
      (unit == 'P') || (unit == 'p') || (unit == 'T') || (unit == 't') ||
      (unit == 'G') || (unit == 'g') || (unit == 'M') || (unit == 'm') ||
      (unit == 'K') || (unit == 'k') ) )
         return true;

   return false;
}

/**
 * Transforms an integer with a time unit appended to its non-human equivalent in seconds,
 * e.g. "1h" will return 3600.
 */
int64_t UnitTk::timeStrHumanToInt64(const char* s)
{
   int64_t oneMinute = 60;
   int64_t oneHour  = oneMinute * 60;
   int64_t oneDay  = oneHour * 24;

   size_t sLen = strlen(s);

   if(sLen < 2)
      return StringTk::strToInt64(s);

   char unit = s[sLen-1];

   if( (unit == 'D') || (unit == 'd') )
   {
      std::string sNoUnit = s;
      sNoUnit.resize(sLen-1);

      return StringTk::strToInt64(sNoUnit) * oneDay;
   }
   else
   if( (unit == 'H') || (unit == 'h') )
   {
      std::string sNoUnit = s;
      sNoUnit.resize(sLen-1);

      return StringTk::strToInt64(sNoUnit) * oneHour;
   }
   else
   if( (unit == 'M') || (unit == 'm') )
   {
      std::string sNoUnit = s;
      sNoUnit.resize(sLen-1);

      return StringTk::strToInt64(sNoUnit) * oneMinute;
   }
   else
      return StringTk::strToInt64(s);
}

/**
 * checks if the given human string is a valid time and can processed by timeStrHumanToInt64()
 *
 * @param humanString the human string to validate
 */
bool UnitTk::isValidHumanTimeString(std::string humanString)
{
   if(StringTk::isNumeric(humanString))
      return true;

   size_t humanStringLen = humanString.length();
   char unit = humanString.at(humanStringLen-1);
   std::string value = humanString.substr(0, humanStringLen-1);

   if( StringTk::isNumeric(value) && ( (unit == 'd') || (unit == 'D') || (unit == 'H') ||
      (unit == 'h') || (unit == 'M') || (unit == 'm') || (unit == 'S') || (unit == 's') ) )
         return true;

   return false;
}

uint64_t UnitTk::quotaBlockCountToByte(uint64_t quotaBlockCount, QuotaBlockDeviceFsType type)
{
   static uint64_t quotaBlockSizeXFS = 512;

   // the quota block size for XFS is 512 bytes, for ext4 the quota block size is 1 byte
   if(type == QuotaBlockDeviceFsType_XFS)
   {
      quotaBlockCount = quotaBlockCount * quotaBlockSizeXFS;
   }

   return quotaBlockCount;
}

/*
 * Prints the quota block count as a number with a unit appended, but only if it matches without a
 * remainder. A quota block has a size of 1024 B.
 */
std::string UnitTk::quotaBlockCountToHumanStr(uint64_t quotaBlockCount, QuotaBlockDeviceFsType type)
{
   std::string unit;

   quotaBlockCount = quotaBlockCountToByte(quotaBlockCount, type);

   double value = byteToXbyte(quotaBlockCount, &unit);

   return StringTk::doubleToStr(value) + " " + unit;
}

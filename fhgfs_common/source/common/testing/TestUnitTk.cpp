#include "TestUnitTk.h"

#include <common/toolkit/UnitTk.h>

TestUnitTk::TestUnitTk()
{
}

TestUnitTk::~TestUnitTk()
{
}

void TestUnitTk::setUp()
{
}

void TestUnitTk::tearDown()
{
}

void TestUnitTk::testGigabyteToByte()
{
   double gbValue = 1.0;
   int64_t byteValueExpected = 1073741824LL;
   int64_t byteValueCalc = UnitTk::gigabyteToByte(gbValue);
   CPPUNIT_ASSERT(byteValueExpected == byteValueCalc);

   gbValue = 10.0;
   byteValueExpected = 10737418240LL;
   byteValueCalc = UnitTk::gigabyteToByte(gbValue);
   CPPUNIT_ASSERT(byteValueExpected == byteValueCalc);

   gbValue = 10.598;
   byteValueExpected = 11379515850LL;

   byteValueCalc = UnitTk::gigabyteToByte(gbValue);
   CPPUNIT_ASSERT(byteValueExpected == byteValueCalc);
}

void TestUnitTk::testMegabyteToByte()
{
   double mbValue = 1.0;
   int64_t byteValueExpected = 1048576LL;
   int64_t byteValueCalc = UnitTk::megabyteToByte(mbValue);
   CPPUNIT_ASSERT(byteValueExpected == byteValueCalc);

   mbValue = 10.0;
   byteValueExpected = 10485760LL;
   byteValueCalc = UnitTk::megabyteToByte(mbValue);
   CPPUNIT_ASSERT(byteValueExpected == byteValueCalc);

   mbValue = 10.598;
   byteValueExpected = 11112808LL;
   byteValueCalc = UnitTk::megabyteToByte(mbValue);;
   CPPUNIT_ASSERT(byteValueExpected == byteValueCalc);
}

void TestUnitTk::testKilobyteToByte()
{
   double kbValue = 1.0;
   int64_t byteValueExpected = 1024LL;
   int64_t byteValueCalc = UnitTk::kilobyteToByte(kbValue);
   CPPUNIT_ASSERT(byteValueExpected == byteValueCalc);

   kbValue = 10.0;
   byteValueExpected = 10240LL;
   byteValueCalc = UnitTk::kilobyteToByte(kbValue);
   CPPUNIT_ASSERT(byteValueExpected == byteValueCalc);

   kbValue = 10.598;
   byteValueExpected = 10852LL;
   byteValueCalc = UnitTk::kilobyteToByte(kbValue);
   CPPUNIT_ASSERT(byteValueExpected == byteValueCalc);
}

void TestUnitTk::testByteToXbyte()
{
   std::string unit;
   int64_t value = 2048LL;
   double valueExpected = 2.0;
   double valueCalc = UnitTk::byteToXbyte(value, &unit);
   CPPUNIT_ASSERT(valueExpected == valueCalc);
   CPPUNIT_ASSERT(unit.compare("KB") == 0);

   value = 10240LL;
   valueExpected = 10.0;
   valueCalc = UnitTk::byteToXbyte(value, &unit);
   CPPUNIT_ASSERT(valueExpected == valueCalc);
   CPPUNIT_ASSERT(unit.compare("KB") == 0);

   value = 10843LL;
   valueExpected = 10.6;
   valueCalc = UnitTk::byteToXbyte(value, &unit, true);
   CPPUNIT_ASSERT(valueExpected == valueCalc);
   CPPUNIT_ASSERT(unit.compare("KB") == 0);

   value = 1073741824LL;
   valueExpected = 1024.0;
   valueCalc = UnitTk::byteToXbyte(value, &unit);
   CPPUNIT_ASSERT(valueExpected == valueCalc);
   CPPUNIT_ASSERT(unit.compare("MB") == 0);

   value = 10737418240LL;
   valueExpected = 10.0;
   valueCalc = UnitTk::byteToXbyte(value, &unit);
   CPPUNIT_ASSERT(valueExpected == valueCalc);
   CPPUNIT_ASSERT(unit.compare("GB") == 0);

   value = 11446087843LL;
   valueExpected = 10.7;
   valueCalc = UnitTk::byteToXbyte(value, &unit, true);
   CPPUNIT_ASSERT(valueExpected == valueCalc);
   CPPUNIT_ASSERT(unit.compare("GB") == 0);
}

void TestUnitTk::testXbyteToByte()
{
   std::string unit = "KB";
   double value = 1.0;
   int64_t byteValueExpected = 1024LL;
   int64_t byteValueCalc = UnitTk::xbyteToByte(value, unit);
   CPPUNIT_ASSERT(byteValueExpected == byteValueCalc);

   value = 10.0;
   byteValueExpected = 10240LL;
   byteValueCalc = UnitTk::xbyteToByte(value, unit);
   CPPUNIT_ASSERT(byteValueExpected == byteValueCalc);

   value = 10.598;
   byteValueExpected = 10852LL;
   byteValueCalc = UnitTk::xbyteToByte(value, unit);
   CPPUNIT_ASSERT(byteValueExpected == byteValueCalc);


   unit = "MB";
   value = 1.0;
   byteValueExpected = 1048576LL;
   byteValueCalc = UnitTk::xbyteToByte(value, unit);
   CPPUNIT_ASSERT(byteValueExpected == byteValueCalc);

   value = 10.0;
   byteValueExpected = 10485760LL;
   byteValueCalc = UnitTk::xbyteToByte(value, unit);
   CPPUNIT_ASSERT(byteValueExpected == byteValueCalc);

   value = 10.598;
   byteValueExpected = 11112808LL;
   byteValueCalc = UnitTk::xbyteToByte(value, unit);
   CPPUNIT_ASSERT(byteValueExpected == byteValueCalc);


   unit = "GB";
   value = 1.0;
   byteValueExpected = 1073741824LL;
   byteValueCalc = UnitTk::xbyteToByte(value, unit);
   CPPUNIT_ASSERT(byteValueExpected == byteValueCalc);

   value = 10.0;
   byteValueExpected = 10737418240LL;
   byteValueCalc = UnitTk::xbyteToByte(value, unit);
   CPPUNIT_ASSERT(byteValueExpected == byteValueCalc);

   value = 10.598;
   byteValueExpected = 11379515850LL;
   byteValueCalc = UnitTk::xbyteToByte(value, unit);
   CPPUNIT_ASSERT(byteValueExpected == byteValueCalc);
}

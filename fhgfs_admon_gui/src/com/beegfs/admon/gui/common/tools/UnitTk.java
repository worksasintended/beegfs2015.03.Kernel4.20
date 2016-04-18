package com.beegfs.admon.gui.common.tools;

import com.beegfs.admon.gui.common.ValueUnit;
import com.beegfs.admon.gui.common.enums.SizeUnitEnum;
import com.beegfs.admon.gui.common.enums.TimeUnitEnum;


public class UnitTk
{
   public static long timeSpanToMinutes(ValueUnit<TimeUnitEnum> timeSpan)
   {
      long retval = 0;
      
      if (null != timeSpan.getUnit())
      {
         switch (timeSpan.getUnit())
         {
            case MINUTES:
               retval = (long) timeSpan.getValue();
               break;
            case HOURS:
               retval = (long) (timeSpan.getValue() * 60);
               break;
            case DAYS:
               retval = (long) (timeSpan.getValue() * 60 * 24);
               break;
            default:
               break;
         }
      }

      return retval;
   }

   public static SizeUnitEnum strToSizeUnitEnum(String str)
   {
      SizeUnitEnum retval = SizeUnitEnum.NONE;

      if (SizeUnitEnum.BYTE.getUnit().equalsIgnoreCase(str))
      {
         retval = SizeUnitEnum.BYTE;
      }
      else if(SizeUnitEnum.KILOBYTE.getUnit().equalsIgnoreCase(str))
      {
         retval = SizeUnitEnum.KILOBYTE;
      }
      else if(SizeUnitEnum.MEGABYTE.getUnit().equalsIgnoreCase(str))
      {
         retval = SizeUnitEnum.MEGABYTE;
      }
      else if(SizeUnitEnum.GIGABYTE.getUnit().equalsIgnoreCase(str))
      {
         retval = SizeUnitEnum.GIGABYTE;
      }
      else if(SizeUnitEnum.TERABYTE.getUnit().equalsIgnoreCase(str))
      {
         retval = SizeUnitEnum.TERABYTE;
      }
      else if(SizeUnitEnum.PETABYTE.getUnit().equalsIgnoreCase(str))
      {
         retval = SizeUnitEnum.PETABYTE;
      }
      else if(SizeUnitEnum.EXABYTE.getUnit().equalsIgnoreCase(str))
      {
         retval = SizeUnitEnum.EXABYTE;
      }
      else if(SizeUnitEnum.ZETTABYTE.getUnit().equalsIgnoreCase(str))
      {
         retval = SizeUnitEnum.ZETTABYTE;
      }
      else if(SizeUnitEnum.YOTTABYTE.getUnit().equalsIgnoreCase(str))
      {
         retval = SizeUnitEnum.YOTTABYTE;
      }
      else if (SizeUnitEnum.BYTE.getDescription().equalsIgnoreCase(str))
      {
         retval = SizeUnitEnum.BYTE;
      }
      else if(SizeUnitEnum.KILOBYTE.getDescription().equalsIgnoreCase(str))
      {
         retval = SizeUnitEnum.KILOBYTE;
      }
      else if(SizeUnitEnum.MEGABYTE.getDescription().equalsIgnoreCase(str))
      {
         retval = SizeUnitEnum.MEGABYTE;
      }
      else if(SizeUnitEnum.GIGABYTE.getDescription().equalsIgnoreCase(str))
      {
         retval = SizeUnitEnum.GIGABYTE;
      }
      else if(SizeUnitEnum.TERABYTE.getDescription().equalsIgnoreCase(str))
      {
         retval = SizeUnitEnum.TERABYTE;
      }
      else if(SizeUnitEnum.PETABYTE.getDescription().equalsIgnoreCase(str))
      {
         retval = SizeUnitEnum.PETABYTE;
      }
      else if(SizeUnitEnum.EXABYTE.getDescription().equalsIgnoreCase(str))
      {
         retval = SizeUnitEnum.EXABYTE;
      }
      else if(SizeUnitEnum.ZETTABYTE.getDescription().equalsIgnoreCase(str))
      {
         retval = SizeUnitEnum.ZETTABYTE;
      }
      else if(SizeUnitEnum.YOTTABYTE.getDescription().equalsIgnoreCase(str))
      {
         retval = SizeUnitEnum.YOTTABYTE;
      }

      return retval;
   }

   public static TimeUnitEnum strToTimeUnitEnum(String str)
   {
      TimeUnitEnum retval = TimeUnitEnum.NONE;

      if (TimeUnitEnum.MILLISECONDS.getUnit().equalsIgnoreCase(str))
      {
         retval = TimeUnitEnum.MILLISECONDS;
      }
      else if(TimeUnitEnum.SECONDS.getUnit().equalsIgnoreCase(str))
      {
         retval = TimeUnitEnum.SECONDS;
      }
      else if(TimeUnitEnum.MINUTES.getUnit().equalsIgnoreCase(str))
      {
         retval = TimeUnitEnum.MINUTES;
      }
      else if(TimeUnitEnum.HOURS.getUnit().equalsIgnoreCase(str))
      {
         retval = TimeUnitEnum.HOURS;
      }
      else if(TimeUnitEnum.DAYS.getUnit().equalsIgnoreCase(str))
      {
         retval = TimeUnitEnum.DAYS;
      }
      else if (TimeUnitEnum.MILLISECONDS.getDescription().equalsIgnoreCase(str))
      {
         retval = TimeUnitEnum.MILLISECONDS;
      }
      else if(TimeUnitEnum.SECONDS.getDescription().equalsIgnoreCase(str))
      {
         retval = TimeUnitEnum.SECONDS;
      }
      else if(TimeUnitEnum.MINUTES.getDescription().equalsIgnoreCase(str))
      {
         retval = TimeUnitEnum.MINUTES;
      }
      else if(TimeUnitEnum.HOURS.getDescription().equalsIgnoreCase(str))
      {
         retval = TimeUnitEnum.HOURS;
      }
      else if(TimeUnitEnum.DAYS.getDescription().equalsIgnoreCase(str))
      {
         retval = TimeUnitEnum.DAYS;
      }

      return retval;
   }

   public static ValueUnit<TimeUnitEnum> strToValueUnitofTime(String str)
   {
      double value = 0;
      TimeUnitEnum unit = TimeUnitEnum.NONE;

      if (!str.isEmpty())
      {
         String[] splitted = str.split(" ");
         try
         {
            value = Double.parseDouble(splitted[0]);
            unit = strToTimeUnitEnum(splitted[1]);
         }
         catch (ArrayIndexOutOfBoundsException | NumberFormatException ex)
         {
            unit = TimeUnitEnum.NONE;
         }
      }
      return new ValueUnit<>(value, unit);
   }

   public static ValueUnit<SizeUnitEnum> strToValueUnitofSize(String str)
   {
      double value = 0;
      SizeUnitEnum unit = SizeUnitEnum.NONE;

      if (!str.isEmpty())
      {
         String[] splitted = str.split(" ");
         try
         {
            value = Double.parseDouble(splitted[0]);
            unit = strToSizeUnitEnum(splitted[1]);
         }
         catch (ArrayIndexOutOfBoundsException | NumberFormatException ex)
         {
            unit = SizeUnitEnum.NONE;
         }
      }
      return new ValueUnit<>(value, unit);
   }

   public static void adjustByteUnits(ValueUnit<SizeUnitEnum> valueOne,
      ValueUnit<SizeUnitEnum> valueTwo)
   {
      SizeUnitEnum unitOne = valueOne.getUnit();
      SizeUnitEnum unitTwo = valueTwo.getUnit();

      int comparison = (unitOne.compareTo(unitTwo));
      
      if (comparison == -1)
      { // which one is greater
         long bytesOne = xbyteTobyte(valueOne);
         long bytesTwo = xbyteTobyte(valueTwo);
         valueOne.update(byteToXbyte(bytesOne, unitOne).getValue(), unitOne);
         valueTwo.update(byteToXbyte(bytesTwo, unitOne).getValue(), unitOne);
      }
      else if (comparison == 1)
      { // which one is greater
         long bytesOne = xbyteTobyte(valueOne);
         long bytesTwo = xbyteTobyte(valueTwo);
         valueOne.update(byteToXbyte(bytesOne, unitOne).getValue(), unitTwo);
         valueTwo.update(byteToXbyte(bytesTwo, unitOne).getValue(), unitTwo);
      }
   }

   public static ValueUnit<SizeUnitEnum> byteToXbyte(long bytes)
   {
      long value = bytes;
      SizeUnitEnum unit;

      short count = 0;
      while ((value > 1024) && (count < 8))
      {
         value = (value / 1024);
         count++;
      }

      switch (count)
      {
         case 0:
            unit = SizeUnitEnum.BYTE;
            break;
         case 1:
            unit = SizeUnitEnum.KILOBYTE;
            break;
         case 2:
            unit = SizeUnitEnum.MEGABYTE;
            break;
         case 3:
            unit = SizeUnitEnum.GIGABYTE;
            break;
         case 4:
            unit = SizeUnitEnum.TERABYTE;
            break;
         case 5:
            unit = SizeUnitEnum.PETABYTE;
            break;
         case 6:
            unit = SizeUnitEnum.EXABYTE;
            break;
         case 7:
            unit = SizeUnitEnum.ZETTABYTE;
            break;
         case 8:
            unit = SizeUnitEnum.YOTTABYTE;
            break;
         default:
            unit = SizeUnitEnum.NONE;
      }

      return new ValueUnit<>(value, unit);
   }

   public static ValueUnit<SizeUnitEnum> byteToXbyte(long bytes, SizeUnitEnum forceUnit)
   {
      long value = bytes;
      SizeUnitEnum unit;

      int maxCount = 0;

      if (null != forceUnit)
      {
         switch (forceUnit)
         {
            case KILOBYTE:
               maxCount = 1;
               break;
            case MEGABYTE:
               maxCount = 2;
               break;
            case GIGABYTE:
               maxCount = 3;
               break;
            case TERABYTE:
               maxCount = 4;
               break;
            case PETABYTE:
               maxCount = 5;
               break;
            case EXABYTE:
               maxCount = 6;
               break;
            case ZETTABYTE:
               maxCount = 7;
               break;
            case YOTTABYTE:
               maxCount = 8;
               break;
            default:
               break;
         }
      }

      short count = 0;

      while (count < maxCount)
      {
         value = (value / 1024);
         count++;
      }

      switch (count)
      {
         case 0:
            unit = SizeUnitEnum.BYTE;
            break;
         case 1:
            unit = SizeUnitEnum.KILOBYTE;
            break;
         case 2:
            unit = SizeUnitEnum.MEGABYTE;
            break;
         case 3:
            unit = SizeUnitEnum.GIGABYTE;
            break;
         case 4:
            unit = SizeUnitEnum.TERABYTE;
            break;
         case 5:
            unit = SizeUnitEnum.PETABYTE;
            break;
         case 6:
            unit = SizeUnitEnum.EXABYTE;
            break;
         case 7:
            unit = SizeUnitEnum.ZETTABYTE;
            break;
         case 8:
            unit = SizeUnitEnum.YOTTABYTE;
            break;
         default:
            unit = SizeUnitEnum.NONE;
      }

      return new ValueUnit<>(value, unit);
   }

   public static long xbyteTobyte(ValueUnit<SizeUnitEnum> value)
   {
      int maxCount = 0;
      SizeUnitEnum unit = value.getUnit();

      if (null != unit)
      {
         switch (unit)
         {
            case KILOBYTE:
               maxCount = 1;
               break;
            case MEGABYTE:
               maxCount = 2;
               break;
            case GIGABYTE:
               maxCount = 3;
               break;
            case TERABYTE:
               maxCount = 4;
               break;
            case PETABYTE:
               maxCount = 5;
               break;
            case EXABYTE:
               maxCount = 6;
               break;
            case ZETTABYTE:
               maxCount = 7;
               break;
            case YOTTABYTE:
               maxCount = 8;
               break;
            default:
               maxCount = 0;
               break;
         }
      }

      long out = (long) value.getValue();
      short count = 0;

      while (count < maxCount)
      {
         out *= 1024;
         count++;
      }

      return out;
   }

   public static ValueUnit<SizeUnitEnum> xbyteToXbyte(ValueUnit<SizeUnitEnum> value)
   {
      long bytes = UnitTk.xbyteTobyte(value);
      return UnitTk.byteToXbyte(bytes);
   }

   public static ValueUnit<SizeUnitEnum> subtract(ValueUnit<SizeUnitEnum> x,
      ValueUnit<SizeUnitEnum> y)
   {
      UnitTk.adjustByteUnits(x, y);
      return new ValueUnit<>(x.getValue() - y.getValue(), x.getUnit());
   }

   private UnitTk()
   {
   }
}

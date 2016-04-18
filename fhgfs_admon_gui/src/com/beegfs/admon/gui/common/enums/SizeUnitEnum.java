package com.beegfs.admon.gui.common.enums;



public enum SizeUnitEnum implements UnitEnum
{
   NONE("",""),
   BYTE("Byte", "byte"),
   KILOBYTE("KB", "kilobyte"),
   MEGABYTE("MB", "megabyte"),
   GIGABYTE("GB", "gigabyte"),
   TERABYTE("TB", "terabyte"),
   PETABYTE("PB", "petabyte"),
   EXABYTE("EB", "exabyte"),
   ZETTABYTE("ZB", "zettabyte"),
   YOTTABYTE("YB", "yottabyte");

   private final String unit;
   private final String description;

   SizeUnitEnum(String unit, String description)
   {
      this.unit = unit;
      this.description = description;
   }

   @Override
   public String getUnit()
   {
      return this.unit;
   }

   @Override
   public String getDescription()
   {
      return this.description;
   }

   @Override
   public int getID()
   {
      return this.ordinal();
   }
}

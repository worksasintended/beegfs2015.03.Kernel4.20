package com.beegfs.admon.gui.common.enums;



public enum StatsTypeEnum
{
   STATS_NONE("None type", "none"),
   STATS_CLIENT_METADATA("Client stats metadata", "ClientStatsMeta"),
   STATS_CLIENT_STORAGE("Client stats storage", "ClientStatsStorage"),
   STATS_USER_METADATA("User stats metadata", "UserStatsMetadata"),
   STATS_USER_STORAGE("User stats storage", "UserStatsStorage");

   private final String description;
   private final String type;

   StatsTypeEnum(String description, String type)
   {
      this.description = description;
      this.type = type;
   }

   public String description()
   {
      return description;
   }

   public String type()
   {
      return type;
   }
}

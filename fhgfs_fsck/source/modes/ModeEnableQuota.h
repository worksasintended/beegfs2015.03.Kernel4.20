#ifndef MODEENABLEQUOTA_H
#define MODEENABLEQUOTA_H

#include <app/config/Config.h>
#include <database/FsckDB.h>
#include <modes/Mode.h>

#define MODEENABLEQUOTA_OUTPUT_INTERVAL_MS 5000

class ModeEnableQuota : public Mode
{
   public:
      ModeEnableQuota();
      virtual ~ModeEnableQuota();

      static void printHelp();

      virtual int execute();

   private:
      FsckDB* database;
      std::map<FsckErrorCode, FsckRepairAction> repairActions;

      LogContext log;

      int initDatabase();
      void printHeaderInformation();

      void fixPermissions();
};

#endif /* MODEENABLEQUOTA_H */

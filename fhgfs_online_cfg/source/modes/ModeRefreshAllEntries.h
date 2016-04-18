#ifndef MODEREFRESHALLENTRIES_H_
#define MODEREFRESHALLENTRIES_H_

#include <common/Common.h>
#include <common/net/message/nodes/RefresherControlMsg.h>
#include <common/net/message/nodes/RefresherControlRespMsg.h>
#include <common/nodes/NodeStore.h>
#include "Mode.h"

/**
 * This mode orders the given metadata server to refresh all its owned entries.
 */
class ModeRefreshAllEntries : public Mode
{
   public:
      ModeRefreshAllEntries()
      {
      }

      virtual int execute();

      static void printHelp();


   private:
      RefresherControlType convertCommandStrToControlType(std::string commandStr);
      void printCommand(RefresherControlType controlCommand);
      void printStatusResult(bool isRunning, uint64_t numDirsRefreshed, uint64_t numFilesRefreshed);

      bool sendCmdAndPrintResult(uint16_t nodeID, RefresherControlType controlCommand);
};

#endif /* MODEREFRESHALLENTRIES_H_ */

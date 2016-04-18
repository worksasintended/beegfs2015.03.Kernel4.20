#ifndef GETNODEINFOMSGEX_H_
#define GETNODEINFOMSGEX_H_

#include <common/net/message/admon/GetNodeInfoMsg.h>
#include <common/app/log/LogContext.h>
#include <app/App.h>


class GetNodeInfoMsgEx : public GetNodeInfoMsg
{
   public:
      GetNodeInfoMsgEx() : GetNodeInfoMsg()
      {

      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock, char* respBuf,
         size_t bufLen, HighResolutionStats* stats);

   private:
      void getMemInfo(int *memTotal, int *memFree);
      void getCPUInfo(std::string *cpuName, int *cpuSpeed, int *cpuCount);

};

#endif /*GETNODEINFOMSGEX_H_*/

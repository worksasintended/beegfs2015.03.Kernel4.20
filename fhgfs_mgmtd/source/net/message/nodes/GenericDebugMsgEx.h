#ifndef GENERICDEBUGMSGEX_H_
#define GENERICDEBUGMSGEX_H_

#include <common/net/message/nodes/GenericDebugMsg.h>


class GenericDebugMsgEx : public GenericDebugMsg
{
   public:
      GenericDebugMsgEx() : GenericDebugMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);


   private:
      std::string processCommand();

      std::string processOpListOpenFiles(std::istringstream& commandStream);
      std::string processOpVersion(std::istringstream& commandStream);
      std::string processOpQuotaExceeded(std::istringstream& commandStream);
      std::string processOpUsedQuota(std::istringstream& commandStream);
      std::string processOpListPools(std::istringstream& commandStream);
};

#endif /* GENERICDEBUGMSGEX_H_ */

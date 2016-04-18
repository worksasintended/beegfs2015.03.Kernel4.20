#ifndef STORAGEBENCHCONTROLMSGEX_H_
#define STORAGEBENCHCONTROLMSGEX_H_

#include <common/net/message/nodes/StorageBenchControlMsg.h>
#include <common/Common.h>

class StorageBenchControlMsgEx: public StorageBenchControlMsg
{
   public:
      StorageBenchControlMsgEx() : StorageBenchControlMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);

   protected:

   private:
};

#endif /* STORAGEBENCHCONTROLMSGEX_H_ */

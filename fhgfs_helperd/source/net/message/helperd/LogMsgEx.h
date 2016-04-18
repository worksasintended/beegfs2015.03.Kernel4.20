#ifndef LOGMSGEX_H_
#define LOGMSGEX_H_

#include <common/net/message/helperd/LogMsg.h>

class LogMsgEx : public LogMsg
{
   public:
      LogMsgEx() : LogMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
   
   protected:
   
   private:

};

#endif /*LOGMSGEX_H_*/

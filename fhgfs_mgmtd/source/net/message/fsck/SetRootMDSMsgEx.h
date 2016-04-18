#ifndef SETROOTMDSMSGEX_H
#define SETROOTMDSMSGEX_H

#include <common/net/message/fsck/SetRootMDSMsg.h>

class SetRootMDSMsgEx : public SetRootMDSMsg
{
   public:
      SetRootMDSMsgEx() : SetRootMDSMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);

     
   protected:
      
   private:

};

#endif /*SETROOTMDSMSGEX_H*/

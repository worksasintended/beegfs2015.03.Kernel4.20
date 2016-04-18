#ifndef DUMMYMSGEX_H
#define DUMMYMSGEX_H

#include <common/net/message/control/DummyMsg.h>

/*
 * this is intended for testing purposes only
 * just sends another DummyMsg back to the sender
 */

class DummyMsgEx : public DummyMsg
{
   public:
      DummyMsgEx() : DummyMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);

     
   protected:
      
   private:
      void processIncomingRoot();
};

#endif /*DUMMYMSGEX_H*/

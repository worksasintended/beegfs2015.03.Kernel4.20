#ifndef REMOVEXATTRMSGEX_H_
#define REMOVEXATTRMSGEX_H_

#include <common/net/message/storage/attribs/RemoveXAttrMsg.h>

class RemoveXAttrMsgEx : public RemoveXAttrMsg
{
   public:
      RemoveXAttrMsgEx() : RemoveXAttrMsg() {}

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock, char* respBuf,
            size_t bufLen, HighResolutionStats* stats);

   protected:

   private:
};

#endif /*REMOVEXATTRMSGEX_H_*/


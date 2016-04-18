#ifndef SETXATTRMSGEX_H_
#define SETXATTRMSGEX_H_

#include <common/net/message/storage/attribs/SetXAttrMsg.h>

class SetXAttrMsgEx : public SetXAttrMsg
{
   public:
      SetXAttrMsgEx() : SetXAttrMsg() {}

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock, char* respBuf,
         size_t bufLen, HighResolutionStats* stats);

   protected:

   private:
};

#endif /*SETXATTRMSGEX_H_*/

#ifndef GETXATTRMSGEX_H_
#define GETXATTRMSGEX_H_

#include <common/net/message/storage/attribs/GetXAttrMsg.h>

class GetXAttrMsgEx : public GetXAttrMsg
{
   public:
      GetXAttrMsgEx() : GetXAttrMsg() {}

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock, char* respBuf,
         size_t bufLen, HighResolutionStats* stats);

   protected:

   private:
};

#endif /*GETXATTRMSGEX_H_*/

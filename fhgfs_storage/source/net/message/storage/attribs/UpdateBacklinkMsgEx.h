#ifndef UPDATEBACKLINKMSGEX_H
#define UPDATEBACKLINKMSGEX_H

#include <common/net/message/storage/attribs/UpdateBacklinkMsg.h>

class UpdateBacklinkMsgEx : public UpdateBacklinkMsg
{
   public:
      UpdateBacklinkMsgEx() : UpdateBacklinkMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
};


#endif /*UPDATEBACKLINKMSGEX_H*/

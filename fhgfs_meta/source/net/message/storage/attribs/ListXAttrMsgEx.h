#ifndef LISTXATTRMSGEX_H_
#define LISTXATTRMSGEX_H_

#include <common/net/message/storage/attribs/ListXAttrMsg.h>

class ListXAttrMsgEx : public ListXAttrMsg
{
   public:
      ListXAttrMsgEx() : ListXAttrMsg() {}

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock, char* respBuf,
         size_t bufLen, HighResolutionStats* stats);

   protected:

   private:

};

#endif /*LISTXATTRMSGEX_H_*/

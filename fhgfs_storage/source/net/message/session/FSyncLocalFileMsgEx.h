#ifndef FSYNCLOCALFILEMSGEX_H_
#define FSYNCLOCALFILEMSGEX_H_

#include <common/net/message/session/FSyncLocalFileMsg.h>

class FSyncLocalFileMsgEx : public FSyncLocalFileMsg
{
   public:
      FSyncLocalFileMsgEx() : FSyncLocalFileMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
   
   protected:
   
   private:

};

#endif /*FSYNCLOCALFILEMSGEX_H_*/

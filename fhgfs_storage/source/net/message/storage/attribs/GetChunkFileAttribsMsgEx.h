#ifndef GETCHUNKFILEATTRIBSMSGEX_H_
#define GETCHUNKFILEATTRIBSMSGEX_H_

#include <common/net/message/storage/attribs/GetChunkFileAttribsMsg.h>

class GetChunkFileAttribsMsgEx : public GetChunkFileAttribsMsg
{
   public:
      GetChunkFileAttribsMsgEx() : GetChunkFileAttribsMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
   
   protected:
   
   private:
      int getTargetFD(struct sockaddr_in* fromAddr, Socket* sock, char* respBuf, size_t bufLen,
         uint16_t actualTargetID, bool* outResponseSent);
   
};

#endif /*GETCHUNKFILEATTRIBSMSGEX_H_*/

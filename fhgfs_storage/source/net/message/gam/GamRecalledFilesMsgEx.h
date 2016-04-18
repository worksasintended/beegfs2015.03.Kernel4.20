#ifndef GAMRECALLEDFILESMSGEX_H
#define GAMRECALLEDFILESMSGEX_H

#include <common/net/message/gam/GamRecalledFilesMsg.h>
#include <common/net/message/gam/GamRecalledFilesMetaMsg.h>

class GamRecalledFilesMsgEx : public GamRecalledFilesMsg
{
   public:
      GamRecalledFilesMsgEx() : GamRecalledFilesMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);

     
   protected:
      
   private:
};

#endif /*GAMRECALLEDFILESMSGEX_H*/

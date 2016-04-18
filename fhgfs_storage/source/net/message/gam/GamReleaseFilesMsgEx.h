#ifndef GAMRELEASEFILESMSGEX_H
#define GAMRELEASEFILESMSGEX_H

#include <common/net/message/gam/GamReleaseFilesMsg.h>
#include <common/net/message/gam/GamReleaseFilesRespMsg.h>
#include <common/net/message/gam/GamReleaseFilesMetaMsg.h>
#include <common/net/message/gam/GamReleaseFilesMetaRespMsg.h>


class GamReleaseFilesMsgEx : public GamReleaseFilesMsg
{
   public:
      GamReleaseFilesMsgEx() : GamReleaseFilesMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);

     
   protected:
      
   private:
};

#endif /*GAMRELEASEFILESMSGEX_H*/

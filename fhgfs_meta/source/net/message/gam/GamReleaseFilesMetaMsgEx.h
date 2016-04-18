#ifndef GAMRELEASEFILESMETAMSGEX_H
#define GAMRELEASEFILESMETAMSGEX_H

#include <common/net/message/gam/GamReleaseFilesMetaMsg.h>
#include <common/net/message/gam/GamReleaseFilesMetaRespMsg.h>

class GamReleaseFilesMetaMsgEx : public GamReleaseFilesMetaMsg
{
   public:
      GamReleaseFilesMetaMsgEx() : GamReleaseFilesMetaMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);

     
   protected:
      
   private:

};

#endif /*GAMRELEASEFILESMETAMSGEX_H*/

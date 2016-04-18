#ifndef GAMRECALLEDFILESMETAMSGEX_H
#define GAMRECALLEDFILESMETAMSGEX_H

#include <common/net/message/gam/GamRecalledFilesMetaMsg.h>

class GamRecalledFilesMetaMsgEx : public GamRecalledFilesMetaMsg
{
   public:
      GamRecalledFilesMetaMsgEx() : GamRecalledFilesMetaMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);

     
   protected:
      
   private:

};

#endif /*GAMRECALLEDFILESMETAMSGEX_H*/

#ifndef GAMGETCHUNKCOLLOCATIONIDMSGEX_H
#define GAMGETCHUNKCOLLOCATIONIDMSGEX_H

#include <common/net/message/gam/GamGetChunkCollocationIDMsg.h>
#include <common/net/message/gam/GamGetChunkCollocationIDRespMsg.h>
#include <common/net/message/gam/GamGetCollocationIDMetaMsg.h>
#include <common/net/message/gam/GamGetCollocationIDMetaRespMsg.h>
#include <common/storage/HsmFileMetaData.h>


class GamGetChunkCollocationIDMsgEx : public GamGetChunkCollocationIDMsg
{
   public:
      GamGetChunkCollocationIDMsgEx() : GamGetChunkCollocationIDMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);

     
   protected:
      
   private:
      bool requestCollocationIDs(uint16_t nodeID, EntryInfoList* entryInfoList,
         EntryInfoList* outEntryInfos, GamCollocationIDVector* outCollocationIDs);

};

#endif /*GAMGETCHUNKCOLLOCATIONIDMSGEX_H*/

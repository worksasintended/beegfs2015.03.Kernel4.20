#ifndef ADJUSTCHUNKPERMISSIONSMSGEX_H
#define ADJUSTCHUNKPERMISSIONSMSGEX_H

#include <common/net/message/fsck/AdjustChunkPermissionsMsg.h>
#include <common/net/message/fsck/AdjustChunkPermissionsRespMsg.h>

class AdjustChunkPermissionsMsgEx : public AdjustChunkPermissionsMsg
{
   public:
      AdjustChunkPermissionsMsgEx() : AdjustChunkPermissionsMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);

     
   protected:
      
   private:
      bool sendSetAttrMsg(std::string& entryID, unsigned userID, unsigned groupID,
         PathInfo* pathInfo, StripePattern* pattern);
};

#endif /*ADJUSTCHUNKPERMISSIONSMSGEX_H*/

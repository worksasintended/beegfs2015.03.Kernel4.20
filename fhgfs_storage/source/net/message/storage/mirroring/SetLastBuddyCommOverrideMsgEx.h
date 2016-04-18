#ifndef SETLASTBUDDYCOMMOVERRIDEMSGEX_H_
#define SETLASTBUDDYCOMMOVERRIDEMSGEX_H_

#include <common/net/message/storage/mirroring/SetLastBuddyCommOverrideMsg.h>
#include <common/storage/StorageErrors.h>

class SetLastBuddyCommOverrideMsgEx : public SetLastBuddyCommOverrideMsg
{
   public:
      SetLastBuddyCommOverrideMsgEx() : SetLastBuddyCommOverrideMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
};

#endif /*SETLASTBUDDYCOMMOVERRIDEMSGEX_H_*/

#ifndef UPDATEDIRPARENTMSGEX1_H_
#define UPDATEDIRPARENTMSGEX1_H_

#include <storage/MetaStore.h>
#include <common/storage/StorageErrors.h>
#include <common/net/message/storage/attribs/UpdateDirParentMsg.h>

class UpdateDirParentMsgEx : public UpdateDirParentMsg
{
   public:
      UpdateDirParentMsgEx() : UpdateDirParentMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);

   protected:

   private:
};


#endif /*UPDATEDIRPARENTMSGEX1_H_*/

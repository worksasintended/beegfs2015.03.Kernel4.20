#ifndef SETLOCALATTRMSGEX_H_
#define SETLOCALATTRMSGEX_H_

#include <common/storage/StorageErrors.h>
#include <common/net/message/storage/attribs/SetLocalAttrMsg.h>

class SetLocalAttrMsgEx : public SetLocalAttrMsg
{
   public:
      SetLocalAttrMsgEx() : SetLocalAttrMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
   
   protected:
   
   private:
      int getTargetFD(struct sockaddr_in* fromAddr, Socket* sock, char* respBuf, size_t bufLen,
         uint16_t actualTargetID, bool* outResponseSent);
      FhgfsOpsErr forwardToSecondary(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, uint16_t actualTargetID, bool* outChunkLocked);
      FhgfsOpsErr fhgfsErrFromSysErr(int errCode);


};


#endif /*SETLOCALATTRMSGEX_H_*/

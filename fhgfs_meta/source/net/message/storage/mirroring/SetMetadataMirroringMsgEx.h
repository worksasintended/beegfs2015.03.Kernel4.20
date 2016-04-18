#ifndef SETMETADATAMIRRORINGMSGEX_H_
#define SETMETADATAMIRRORINGMSGEX_H_

#include <common/storage/StorageErrors.h>
#include <common/net/message/storage/mirroring/SetMetadataMirroringMsg.h>


class SetMetadataMirroringMsgEx : public SetMetadataMirroringMsg
{
   public:
      SetMetadataMirroringMsgEx() : SetMetadataMirroringMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
   
   protected:
   
   private:
      FhgfsOpsErr setDirMirroring(EntryInfo* entryInfo);

};


#endif /*SETMETADATAMIRRORINGMSGEX_H_*/

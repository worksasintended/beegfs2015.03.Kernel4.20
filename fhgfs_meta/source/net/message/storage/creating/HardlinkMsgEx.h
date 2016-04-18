#ifndef HARDLINKEX_H_
#define HARDLINKEX_H_

#include <common/storage/StorageErrors.h>
#include <common/net/message/storage/creating/HardlinkMsg.h>
#include <storage/DirEntry.h>
#include <storage/MetaStore.h>

class HardlinkMsgEx : public HardlinkMsg
{
   public:
      HardlinkMsgEx() : HardlinkMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
   
   protected:
   
   private:
      FhgfsOpsErr createLink(EntryInfo* fromDirInfo, EntryInfo* fromFileInfo, std::string& fromName,
         EntryInfo* toDirInfo, std::string& toName);
      bool isInSameDir(EntryInfo* fromDirInfo, EntryInfo* toDirInfo);

      FhgfsOpsErr createMultiDirHardlink(EntryInfo* fromFileInfo, EntryInfo* toDirInfo,
         std::string toName);
      FhgfsOpsErr createLinkDentry(EntryInfo* fromFileInfo, EntryInfo* toDirInfo,
         std::string& toName);


};


#endif /*HARDLINKEX_H_*/

#ifndef LISTDIRFROMOFFSETMSGEX_H_
#define LISTDIRFROMOFFSETMSGEX_H_

#include <storage/MetaStore.h>
#include <common/storage/EntryInfo.h>
#include <common/storage/StorageErrors.h>
#include <common/net/message/storage/listing/ListDirFromOffsetMsg.h>


class ListDirFromOffsetMsgEx : public ListDirFromOffsetMsg
{
   public:
      ListDirFromOffsetMsgEx() : ListDirFromOffsetMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);
   
   protected:
   
   private:
      FhgfsOpsErr listDirIncremental(EntryInfo* entryInfo, StringList* outNames,
         UInt8List* outEntryTypes, StringList* outEntryIDs, Int64List* outServerOffsets,
         int64_t* outNewOffset);
};


#endif /*LISTDIRFROMOFFSETMSGEX_H_*/

#ifndef LISTCHUNKDIRINCREMENTALMSGEX_H_
#define LISTCHUNKDIRINCREMENTALMSGEX_H_

#include <common/net/message/storage/listing/ListChunkDirIncrementalMsg.h>

class ListChunkDirIncrementalMsgEx : public ListChunkDirIncrementalMsg
{
   public:
      ListChunkDirIncrementalMsgEx() : ListChunkDirIncrementalMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);

   private:
      FhgfsOpsErr readChunks(uint16_t targetID, bool isMirror, std::string& relativeDir,
         int64_t offset, unsigned maxOutEntries, bool onlyFiles, StringList& outNames,
         IntList& outEntryTypes, int64_t &outNewOffset);
};

#endif /*LISTCHUNKDIRINCREMENTALMSGEX_H_*/

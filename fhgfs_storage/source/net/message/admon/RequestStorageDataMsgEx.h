#ifndef REQUESTSTORAGEDATAMSGEX_H_
#define REQUESTSTORAGEDATAMSGEX_H_

#include <app/App.h>
#include <common/app/log/LogContext.h>
#include <common/components/worker/queue/MultiWorkQueue.h>
#include <common/net/message/admon/RequestStorageDataMsg.h>
#include <common/storage/StorageErrors.h>
#include <common/storage/StorageDefinitions.h>
#include <common/storage/StorageTargetInfo.h>
#include <common/toolkit/MessagingTk.h>
#include <common/net/message/admon/RequestStorageDataRespMsg.h>
#include <nodes/NodeStoreEx.h>
#include <program/Program.h>


class RequestStorageDataMsgEx : public RequestStorageDataMsg
{
   public:
      RequestStorageDataMsgEx() : RequestStorageDataMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,char* respBuf,
         size_t bufLen, HighResolutionStats *stats);


   private:
};

#endif /*REQUESTSTORAGEDATAMSGEX_H_*/

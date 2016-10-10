#ifndef META_CLIENTSYNCER_H
#define META_CLIENTSYNCER_H

#include <common/threading/PThread.h>
#include <common/app/log/LogContext.h>
#include <common/components/ComponentInitException.h>
#include <common/Common.h>


class ClientSyncer : public PThread
{
      public:
      ClientSyncer() throw (ComponentInitException);
      void forceDownload();
      void syncClients(NodeList* clientsList, bool allowRemoteComm);
      void downloadAndSyncClients();

   private:
      LogContext log;

      bool downloadForced;
      Mutex downloadForcedMutex;

      virtual void run();
      void syncLoop();
      bool getAndResetDownloadForced();
};

#endif /* META_CLIENTSYNCER_H */

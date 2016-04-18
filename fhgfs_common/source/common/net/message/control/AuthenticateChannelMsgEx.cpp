#include <common/app/config/ICommonConfig.h>
#include <common/app/log/LogContext.h>
#include <common/app/AbstractApp.h>
#include "AuthenticateChannelMsgEx.h"

bool AuthenticateChannelMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   const char* logContext = "AuthenticateChannel incoming";

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG(logContext, Log_DEBUG,
      std::string("Received a AuthenticateChannelMsg from: ") + peer);

   AbstractApp* app = PThread::getCurrentThreadApp();
   ICommonConfig* cfg = app->getCommonConfig();
   uint64_t authHash = cfg->getConnAuthHash();

   if(authHash && (authHash != getAuthHash() ) )
   { // authentication failed

      #ifdef BEEGFS_DEBUG
         // print more info in debug mode
         LOG_DEBUG(logContext, Log_WARNING,
            "Peer sent wrong authentication: " + peer + " "
               "My authHash: " + StringTk::uint64ToHexStr(getAuthHash() ) + " "
               "Peer authHash: " + StringTk::uint64ToHexStr(getAuthHash() ) );
      #else
         LogContext(logContext).log(Log_WARNING,
            "Peer sent wrong authentication: " + peer);
      #endif // BEEGFS_DEBUG
   }
   else
   {
      LOG_DEBUG(logContext, Log_SPAM,
         "Authentication successful. "
            "My authHash: " + StringTk::uint64ToHexStr(getAuthHash() ) + " "
            "Peer authHash: " + StringTk::uint64ToHexStr(getAuthHash() ) );

      sock->setIsAuthenticated();
   }

   // note: this message does not require a response

   return true;
}



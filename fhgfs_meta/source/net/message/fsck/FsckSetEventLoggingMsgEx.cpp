#include <components/ModificationEventFlusher.h>
#include <program/Program.h>
#include "FsckSetEventLoggingMsgEx.h"

bool FsckSetEventLoggingMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG("FsckSetEventLoggingMsg incoming", Log_DEBUG,
      std::string("Received a FsckSetEventLoggingMsg from: ") + peer);

   LogContext log("FsckSetEventLoggingMsg incoming");

   bool result;
   bool missedEvents;
   bool loggingEnabled;

   App* app = Program::getApp();

   bool enableLogging = this->getEnableLogging();

   if (enableLogging)
   {
      unsigned portUDP = this->getPortUDP();
      NicAddressList nicList;
      this->parseNicList(&nicList);

      loggingEnabled = app->getModificationEventFlusher()->enableLogging(portUDP, nicList,
            getForceRestart());

      if (app->getModificationEventFlusher()->getNumLoggingWorkers() == app->getWorkers()->size() )
         result = true;
      else
         result = false;

      missedEvents = true; // (value ignored when logging is enabled)
   }
   else
   { // disable logging
      result = app->getModificationEventFlusher()->disableLogging();
      loggingEnabled = false; // (value ignored when logging is disabled)
      missedEvents = app->getModificationEventFlusher()->getFsckMissedEvent();
   }

   FsckSetEventLoggingRespMsg respMsg(result, loggingEnabled, missedEvents);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0, (struct sockaddr*) fromAddr,
      sizeof(struct sockaddr_in) );

   return true;
}

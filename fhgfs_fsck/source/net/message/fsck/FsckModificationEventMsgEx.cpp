#include <common/net/msghelpers/MsgHelperAck.h>
#include <program/Program.h>

#include "FsckModificationEventMsgEx.h"

bool FsckModificationEventMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats) throw(SocketException)
{
   LogContext log("FsckModificationEventMsg incoming");
   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG_CONTEXT(log, 4, std::string("Received a FsckModificationEventMsg from: ") + peer);

   App* app = Program::getApp();
   ModificationEventHandler *eventHandler = app->getModificationEventHandler();

   UInt8List eventTypeList;
   StringList entryIDList;

   this->parseModificationEventTypeList(&eventTypeList);
   this->parseEntryIDList(&entryIDList);

   bool addRes = eventHandler->add(eventTypeList, entryIDList);

   if (unlikely(!addRes))
   {
      log.logErr("Unable to add modification event to database");
      return false;
   }

   bool ackRes = MsgHelperAck::respondToAckRequest(this, fromAddr, sock, respBuf, bufLen,
      app->getDatagramListener());

   if (!ackRes)
      log.logErr("Unable to send ack to metadata server");

   return true;
}

#include <common/app/log/LogContext.h>
#include <common/app/AbstractApp.h>
#include <common/net/message/control/AckMsg.h>
#include <common/net/message/session/locking/LockGrantedMsg.h>
#include <common/net/message/NetMessage.h>
#include <program/Program.h>
#include "LockEntryNotificationWork.h"


Mutex LockEntryNotificationWork::ackCounterMutex;
unsigned LockEntryNotificationWork::ackCounter = 0;


void LockEntryNotificationWork::process(char* bufIn, unsigned bufInLen, char* bufOut,
   unsigned bufOutLen)
{
   /* note: this code is very similar to LockRangeNotificationWork, so if you change something here,
      you probably want to change it there, too. */

   const char* logContext = "LockEntryNotificationWork::process";
   App* app = Program::getApp();
   Logger* logger = app->getLogger();

   Config* cfg = app->getConfig();
   AcknowledgmentStore* ackStore = app->getAckStore();
   DatagramListener* dgramLis = app->getDatagramListener();
   MetaStore* metaStore = app->getMetaStore();
   NodeStoreClientsEx* clients = app->getClientNodes();
   uint16_t localNodeID = app->getLocalNode()->getNumID();

   // max total time is ackWaitMS * numRetries, defaults to 333ms * 15 => 5s
   int ackWaitSleepMS = cfg->getTuneLockGrantWaitMS();
   int numRetriesLeft = cfg->getTuneLockGrantNumRetries();

   WaitAckMap waitAcks;
   WaitAckMap receivedAcks;
   WaitAckNotification notifier;

   bool allAcksReceived = false;

   // note: we use uint for tv_sec (not uint64) because 32 bits are enough here
   // gives string like this: "time-counter-elck-"
   std::string ackIDPrefix =
      StringTk::uintToHexStr(TimeAbs().getTimeval()->tv_sec) + "-" +
      StringTk::uintToHexStr(incAckCounter() ) + "-" "elck" "-";


   if(unlikely(notifyList->empty() ) )
      return; // nothing to be done


   // create and register waitAcks

   /* note: waitAcks store pointers to notifyList items, so make sure to not remove anything from
      the list while we're still using the waitAcks pointers */

   for(LockEntryNotifyListIter iter = notifyList->begin(); iter != notifyList->end(); iter++)
   {
      std::string ackID = ackIDPrefix + iter->lockAckID; // (we assume lockAckID is globally unique)

      WaitAck waitAck(ackID, &(*iter) );

      waitAcks.insert(WaitAckMapVal(ackID, waitAck) );
   }

   ackStore->registerWaitAcks(&waitAcks, &receivedAcks, &notifier);


   // loop: send requests -> waitforcompletion -> resend

   while(numRetriesLeft && !app->getSelfTerminate() )
   {
      // create waitAcks copy

      SafeMutexLock waitAcksLock(&notifier.waitAcksMutex);

      WaitAckMap currentWaitAcks(waitAcks);

      waitAcksLock.unlock();

      // send messages

      for(WaitAckMapIter iter = currentWaitAcks.begin(); iter != currentWaitAcks.end(); iter++)
      {
         EntryLockDetails* lockDetails = (EntryLockDetails*)iter->second.privateData;

         LockGrantedMsg msg(lockDetails->lockAckID, iter->first, localNodeID);

         bool serializeRes = msg.serialize(bufOut, bufOutLen);
         if(unlikely(!serializeRes) )
         { // buffer too small - should never happen
            logger->log(Log_CRITICAL, logContext, "BUG(?): Buffer too small for message "
               "serialization: " + StringTk::intToStr(bufOutLen) + "/" +
               StringTk::intToStr(msg.getMsgLength() ) );
            continue;
         }

         Node* node = clients->referenceNode(lockDetails->clientID);
         if(unlikely(!node) )
         { // node not exists
            logger->log(Log_DEBUG, logContext, "Cannot grant lock to unknown client: " +
               lockDetails->clientID);
            continue;
         }

         dgramLis->sendBufToNode(node, bufOut, msg.getMsgLength() );

         clients->releaseNode(&node);
      }

      // wait for acks

      allAcksReceived = ackStore->waitForAckCompletion(&currentWaitAcks, &notifier, ackWaitSleepMS);
      if(allAcksReceived)
         break; // all acks received

      // some waitAcks left => prepare next loop

      numRetriesLeft--;
   }

   // waiting for acks is over

   ackStore->unregisterWaitAcks(&waitAcks);

   // check and handle results (waitAcks now contains all unreceived acks)

   if(waitAcks.empty() )
   {
      LOG_DEBUG(logContext, 4, "Stats: received all acks: " +
         StringTk::intToStr(receivedAcks.size() ) + "/" + StringTk::intToStr(notifyList->size() ) );

      return; // perfect, all acks received
   }

   // some acks were missing...

   logger->log(Log_DEBUG, logContext, "Some replies to lock grants missing. Received: " +
      StringTk::intToStr(receivedAcks.size() ) + "/" +
      StringTk::intToStr(receivedAcks.size() + waitAcks.size() ) );

   // the inode is supposed to be be referenced already
   FileInode* inode = metaStore->referenceLoadedFile(this->parentEntryID, this->entryID);
   if(unlikely(!inode) )
   { // locked inode cannot be referenced
      logger->log(Log_DEBUG, logContext, "FileID cannot be referenced (file unlinked?): " +
         this->entryID);
      return;
   }

   // unlock all locks for which we didn't receive an ack

   for(WaitAckMapIter iter = waitAcks.begin(); iter != waitAcks.end(); iter++)
   {
      EntryLockDetails* lockDetails = (EntryLockDetails*)iter->second.privateData;

      unlockWaiter(inode, lockDetails);

      LOG_DEBUG(logContext, Log_DEBUG, "Reply was missing from: " + lockDetails->clientID);
   }

   // cancel all remaining lock waiters if too many acks were missing
   // (this is very important to avoid long timeouts if multiple clients are gone/disconnected)

   if(waitAcks.size() > 1)
   { // cancel all waiters
      cancelAllWaiters(inode);
   }

   // cleanup

   metaStore->releaseFile(this->parentEntryID, inode);
}

/**
 * Remove lock of a waiter from which we didn't receive an ack.
 */
void LockEntryNotificationWork::unlockWaiter(FileInode* inode, EntryLockDetails* lockDetails)
{
   const char* logContext = "LockEntryNotificationWork::unlockWaiter";
   App* app = Program::getApp();
   Logger* logger = app->getLogger();


   lockDetails->setUnlock();


   if(lockType == LockEntryNotifyType_APPEND)
      inode->flockAppend(*lockDetails);
   else
   if(lockType == LockEntryNotifyType_FLOCK)
      inode->flockEntry(*lockDetails);
   else
      logger->logErr(logContext, "Invalid lockType given: " + StringTk::uintToStr(lockType) );
}

/**
 * Cancel all remaining lock waiters.
 *
 * Usually called because too many acks were not received and we want to avoid repeated long
 * timeout stalls.
 */
void LockEntryNotificationWork::cancelAllWaiters(FileInode* inode)
{
   const char* logContext = "LockEntryNotificationWork::unlockWaiter";
   App* app = Program::getApp();
   Logger* logger = app->getLogger();

   if(lockType == LockEntryNotifyType_APPEND)
      inode->flockAppendCancelAllWaiters();
   else
   if(lockType == LockEntryNotifyType_FLOCK)
      inode->flockEntryCancelAllWaiters();
   else
      logger->logErr(logContext, "Invalid lockType given: " + StringTk::uintToStr(lockType) );
}

unsigned LockEntryNotificationWork::incAckCounter()
{
   unsigned currentAckCounter;

   SafeMutexLock mutexLock(&ackCounterMutex);

   currentAckCounter = ackCounter++;

   mutexLock.unlock();

   return currentAckCounter;
}

Mutex* LockEntryNotificationWork::getDGramLisMutex(AbstractDatagramListener* dgramLis)
{
   return &dgramLis->sendMutex;
}

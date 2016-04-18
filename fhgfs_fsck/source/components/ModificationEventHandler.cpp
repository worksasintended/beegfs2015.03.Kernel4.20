#include "ModificationEventHandler.h"
#include <database/FsckDBException.h>

#include <program/Program.h>

ModificationEventHandler::ModificationEventHandler(): PThread("ModificationEventHandler")
{
   this->database = Program::getApp()->getDatabase();
}

ModificationEventHandler::~ModificationEventHandler()
{
}

void ModificationEventHandler::run()
{
   while ( !this->getSelfTerminate() )
   {
      // make sure to group at least MODHANDLER_MINSIZE_FLUSH flush elements (to not bother the DB
      // with every single event)
      while ( this->bufferList.size() < MODHANDLER_MINSIZE_FLUSH )
      {
         SafeMutexLock safeLock(&eventsAddedMutex);
         this->eventsAddedCond.timedwait(&eventsAddedMutex, 5000);
         safeLock.unlock();

         if (this->getSelfTerminate())
            break;
      }

      this->flush();
   }
}

bool ModificationEventHandler::add(UInt8List& eventTypeList, StringList& entryIDList)
{
   const char* logContext = "ModificationEventHandler (add)";

   if ( unlikely(eventTypeList.size() != entryIDList.size()) )
   {
      LogContext(logContext).logErr("Unable to add events. The lists do not have equal sizes.");
      return false;
   }


   while ( true )
   {
      SafeMutexLock bufferListSafeLock(&bufferListMutex);
      if (this->bufferList.size() < MODHANDLER_MAXSIZE_EVENTLIST)
      {
         bufferListSafeLock.unlock();
         break;
      }
      else
      {
         bufferListSafeLock.unlock();
         SafeMutexLock eventsFlushedSafeLock(&eventsFlushedMutex);
         this->eventsFlushedCond.timedwait(&eventsFlushedMutex, 5000);
         eventsFlushedSafeLock.unlock();
      }
   }

   UInt8ListIter eventTypeIter = eventTypeList.begin();
   StringListIter entryIDIter = entryIDList.begin();

   SafeMutexLock bufferListSafeLock(&bufferListMutex);

   for ( ; eventTypeIter != eventTypeList.end(); eventTypeIter++, entryIDIter++ )
   {
      FsckModificationEvent event((ModificationEventType) *eventTypeIter, *entryIDIter);
      this->bufferList.push_back(event);
   }

   bufferListSafeLock.unlock();

   this->eventsAddedCond.signal();

   return true;
}

void ModificationEventHandler::flush()
{
   SafeMutexLock flushSafeLock(&flushMutex);
   const char* logContext = "ModificationEventHandler (flush)";

   // create a copy of the buffer list and this to DB, so that the buffer will become free
   // immediately and the incoming messages do not have to wait for DB
   SafeMutexLock bufferListCopySafeLock(&bufferListMutex);

   if (this->bufferList.empty())
   {
      bufferListCopySafeLock.unlock();
      flushSafeLock.unlock();
      return;
   }

   bufferListCopy.splice(bufferListCopy.begin(), this->bufferList);

   bufferListCopySafeLock.unlock();

   FsckModificationEvent failedInsert;
   int errorCode;

   bool success = database->insertModificationEvents(bufferListCopy, failedInsert, errorCode);

   if ( !success )
   {
      LogContext(logContext).log(1,
         "Failed to insert modification event; eventType: "
            + StringTk::uintToStr(failedInsert.getEventType()) + "; entryID: "
            + failedInsert.getEntryID());
      LogContext(logContext).log(1, "SQLite Error was error code " + StringTk::intToStr(errorCode));
      throw FsckDBException(
         "Error while inserting modification event to database. "
         "Please see log for more information.");
   }

   bufferListCopy.clear();

   flushSafeLock.unlock();
}

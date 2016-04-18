#ifndef MODIFICATIONEVENTHANDLER_H
#define MODIFICATIONEVENTHANDLER_H

#include <common/fsck/FsckModificationEvent.h>
#include <common/threading/Condition.h>
#include <database/FsckDB.h>


#define MODHANDLER_MAXSIZE_EVENTLIST      50000
#define MODHANDLER_MINSIZE_FLUSH          200

class ModificationEventHandler: public PThread
{
   private:
      FsckDB* database;
      FsckModificationEventList bufferList;
      FsckModificationEventList bufferListCopy;

      Mutex bufferListMutex;
      Mutex bufferListCopyMutex;
      Mutex flushMutex;

      Mutex eventsAddedMutex;
      Condition eventsAddedCond;
      Mutex eventsFlushedMutex;
      Condition eventsFlushedCond;

   public:
      ModificationEventHandler();
      virtual ~ModificationEventHandler();

      virtual void run();

      bool add(UInt8List& eventTypeList, StringList& entryIDList);
      void flush();
};

#endif /* MODIFICATIONEVENTHANDLER_H */

#include "DBHandlePool.h"
#include <common/threading/SafeMutexLock.h>
#include <program/Program.h>

DBHandlePool::DBHandlePool(const std::string& databasePath)
{
   Config* cfg = Program::getApp()->getConfig();

   this->databasePath = databasePath;

   this->openedHandles = 0;
   this->availableHandles = 0;

   this->maxHandles = cfg->getDatabaseNumMaxConns();
}

DBHandlePool::~DBHandlePool()
{
   for (HandleListIter iter = handleList.begin(); iter != handleList.end(); iter++)
   {
      SAFE_DELETE(*iter);
   }

   handleList.clear();
}

DBHandle* DBHandlePool::acquireHandle(bool allowWaiting)
{
   LogContext log("DBHandlePool: acquireHandle");

   DBHandle* dbHandle = NULL;

   SafeMutexLock mutexLock(&mutex); // L O C K

   if(!availableHandles && (openedHandles == maxHandles) )
   { // wait for a handle to become available

      if(!allowWaiting)
      { // all handles in use and waiting not allowed => exit
         mutexLock.unlock(); // U N L O C K
         return NULL;
      }

      while(!availableHandles && (openedHandles == maxHandles) )
         changeCond.wait(&mutex);
   }


   if(likely(availableHandles) )
   {
      // opended handle available => grab it

      HandleListIter iter = handleList.begin();

      while(!(*iter)->isAvailable() )
         iter++;

      dbHandle = *iter;
      dbHandle->setAvailable(false);

      availableHandles--;

      mutexLock.unlock(); // U N L O C K

      return dbHandle;
   }


   // no handle available, but maxHandles not reached yet => open a new handle
   openedHandles++;

   dbHandle = new DBHandle(databasePath);

   handleList.push_back(dbHandle);

   dbHandle->setAvailable(false);

   mutexLock.unlock();

   return dbHandle;
}

void DBHandlePool::releaseHandle(DBHandle* handle)
{
   // mark the handle as available
   SafeMutexLock mutexLock(&mutex);

   #ifdef BEEGFS_DEBUG
      if (handle->isAvailable())
      {
         LogContext log("DBHandlePool::releaseHandle");
         log.logErr("Bug(?): Application released db handle which was already marked as available");
         log.logBacktrace();
      }
   #endif

   availableHandles++;

   #ifdef BEEGFS_DEBUG
      if (availableHandles > openedHandles)
      {
         LogContext log("DBHandlePool::releaseHandle");
         log.logErr("Bug(?): More db handles available than open?");
         log.logBacktrace();
      }
   #endif

   handle->setAvailable(true);

   changeCond.signal();

   mutexLock.unlock();
}

#ifndef DBHANDLEPOOL_H_
#define DBHANDLEPOOL_H_

#include <common/threading/Condition.h>
#include <common/threading/Mutex.h>
#include <database/DBHandle.h>

typedef std::list<DBHandle*> HandleList;
typedef HandleList::iterator HandleListIter;

class DBHandlePool
{
   public:
      DBHandlePool(const std::string& databasePath);
      virtual ~DBHandlePool();

      DBHandle* acquireHandle(bool allowWaiting = true);
      void releaseHandle(DBHandle* handle);

   private:
      std::string databasePath;

      unsigned availableHandles; // available opened handles
      unsigned openedHandles; // not equal to handleList.size!!
      unsigned maxHandles;

      HandleList handleList;

      Mutex mutex;
      Condition changeCond;
};

#endif /* DBHANDLEPOOL_H_ */

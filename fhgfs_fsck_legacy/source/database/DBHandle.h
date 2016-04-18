#ifndef DBHANDLE_H_
#define DBHANDLE_H_

#include <common/app/log/LogContext.h>

#include <sqlite3.h>

#define SQLITE_BUSY_TIMEOUT_MS 500

struct SqliteUnlockNotification
{
  int fired; /* True after unlock event has occurred; important to prevent races, where the unlock
              * event comes before we go into sleep
              */
  Condition cond;
  Mutex mutex;
};

class DBHandle
{
   public:
      DBHandle(std::string& databasePath);
      virtual ~DBHandle();

      int sqliteBlockingStep(sqlite3_stmt *stmt);

      int sqliteBlockingPrepare(const char *query, int queryLen, sqlite3_stmt **outStmt,
         const char **sqlTail = NULL);

      int sqliteBlockingExec(const char* queryStr, std::string* outSqlErrorStr = NULL,
         bool logErrors = true);

      int sqliteBlockingExec(std::string& queryStr, std::string* outSqlErrorStr = NULL,
         bool logErrors = true);

   private:
      LogContext log;

      std::string databasePath;
      sqlite3* handle;
      bool available;

      int waitForUnlockNotify();

   public:
      sqlite3* getHandle()
      {
         return handle;
      }

      bool isAvailable()
      {
         return available;
      }

      void setAvailable(bool value)
      {
         this->available = value;
      }
};

#endif /* DBHANDLE_H_ */

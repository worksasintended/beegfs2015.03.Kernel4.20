#ifndef DBHANDLE_H_
#define DBHANDLE_H_

#include <common/app/log/LogContext.h>

#include <sqlite3.h>

#define SQLITE_BUSY_TIMEOUT_MS 500

class DBHandle
{
   public:
      DBHandle(std::string dbFilename);
      virtual ~DBHandle();

   private:
      LogContext log;

      std::string dbFilename;
      sqlite3* handle;
      bool available;

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

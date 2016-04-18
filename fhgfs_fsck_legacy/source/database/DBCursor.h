#ifndef DBCURSOR_H_
#define DBCURSOR_H_

#include <database/DBHandlePool.h>
#include <toolkit/DatabaseTk.h>


/*
 * DBCursor is an abstraction layer for all all database tables. A query can be mapped to a cursor
 * of a special type. By using this cursor the result set can be read incrementally.
 *
 * DBCursor works with every type, which has a function
 * DatabaseTk::createObjectFromRow(sqlite3_stmt* stmt, T* outObject, unsigned colShift)
 * defined.
 *
 * Normal usage is like this:
 *
 * T* object = cursor.open();
 * while (object)
 * {
 *   // do something with object
 *   object = cursor.step();
 * }
 * cursor.close();
 *
 * Note: At the moment there is no locking here, becuase there is no need for locking. Cursors are
 * never passed around between threads.
 */
template <typename T>
class DBCursor
{
   private:
      std::string queryStr;
      DBHandlePool* dbHandlePool;
      DBHandle* dbHandle;
      sqlite3_stmt* stmt;
      unsigned resultColShift;

      T* currentRow;

      LogContext log;

   public:
      /*
       * @param dbHandlePool
       * @param queryStr
       * @param resultColShift some tables might have additional attributes in first columns; this
       * parameter tells the cursor to ignore the first x columns when creating objects from the
       * result set
       */
      DBCursor(DBHandlePool* dbHandlePool, std::string queryStr, unsigned resultColShift = 0)
      {
         this->dbHandlePool = dbHandlePool;
         this->dbHandle = NULL;
         this->queryStr = queryStr;
         this->stmt = NULL;
         this->resultColShift = resultColShift;

         this->currentRow = new T();

         log.setContext("DBCursor");
      }

      virtual ~DBCursor()
      {
         SAFE_DELETE(this->currentRow);
      }

      /*
       * aquire a database handle and execute the query
       *
       * @return the first dataset in the result set (may be NULL, when result set is empty)
       */
      T* open()
      {
         dbHandle = this->dbHandlePool->acquireHandle();

         // SQLITE_BUSY should basically be impossible when reading, but as we cannot guarantee it,
         // we have to handle it
         int sqlite_retVal = SQLITE_BUSY;
         while (sqlite_retVal == SQLITE_BUSY)
         {
            sqlite_retVal = dbHandle->sqliteBlockingPrepare(queryStr.c_str(), queryStr.length(),
               &stmt, NULL );
         }

         if ( sqlite_retVal != SQLITE_OK)
         {
            // TODO: better error handling here
            log.logErr(
               "Error while opening SQL cursor. SQLite error code: "
                  + StringTk::intToStr(sqlite_retVal));

            return NULL;
         }

         if( dbHandle->sqliteBlockingStep(stmt) == SQLITE_ROW )
         {
            DatabaseTk::createObjectFromRow(stmt, currentRow, resultColShift);
            return this->currentRow;
         }
         else
            return NULL;
      }

      /*
       * close the cursor and free up memory
       */
      void close()
      {
         if (dbHandle)
            dbHandlePool->releaseHandle(dbHandle);

         sqlite3_finalize( stmt );
      }

      /*
       * @return the raw sqlite3_stmt* object returned by stepping to the next row in the result set
       */
      sqlite3_stmt* stepRaw()
      {
         if (!stmt)
         {
            log.log(3, "SQLite Statement is NULL. DBCursor was problably not opened.");
         }

         if( dbHandle->sqliteBlockingStep( stmt ) == SQLITE_ROW )
            return stmt;
         else
            return NULL;
      }

      /*
       * @return a pointer to a object that represents the current result set held in sqlite; do
       * not use this directly in your code (e.g. push it into a list); this will change with each
       * call to step() and will be deleted when the cursor is deleted.
       */
      T* step()
      {
         if (!stmt)
         {
            log.log(3, "SQLite Statement is NULL. DBCursor was problably not opened.");
            return NULL;
         }

         if( dbHandle->sqliteBlockingStep( stmt ) == SQLITE_ROW )
         {
            DatabaseTk::createObjectFromRow(stmt, currentRow, resultColShift);
            return this->currentRow;
         }
         else
         {
            return NULL;
         }
      }

      /*
       * returns the db handle, which is currently used by this cursor. This is useful if we have
       * an open cursor and want to do a modification to the database. If there is one handle with
       * a read lock, sqlite will not allow a different handle to obtain a write lock, therefore we
       * need to reuse the existing one
       *
       * @return the db handle used by this cursor
       */
      DBHandle* getHandle() const
      {
         return this->dbHandle;
      }
};


#endif /* DBCURSOR_H_ */

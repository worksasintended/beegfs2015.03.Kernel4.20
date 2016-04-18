#ifndef DBFOREIGNKEY_H
#define DBFOREIGNKEY_H

#include <common/Common.h>

class DBForeignKey;

typedef std::list<DBForeignKey> DBForeignKeyList;
typedef DBForeignKeyList::iterator DBForeignKeyListIter;

class DBForeignKey
{
   public:
      DBForeignKey(std::string fieldName, std::string foreignTableName,
         std::string foreignFieldName) : fieldName(fieldName), foreignTableName(foreignTableName),
         foreignFieldName(foreignFieldName) { };

   private:
      std::string fieldName;
      std::string foreignTableName;
      std::string foreignFieldName;

   public:
      std::string getFieldName() const
      {
         return fieldName;
      }

      std::string getForeignFieldName() const
      {
         return foreignFieldName;
      }

      std::string getForeignTableName() const
      {
         return foreignTableName;
      }

      void setFieldName(std::string fieldName)
      {
         this->fieldName = fieldName;
      }

      void setForeignFieldName(std::string foreignFieldName)
      {
         this->foreignFieldName = foreignFieldName;
      }

      void setForeignTableName(std::string foreignTableName)
      {
         this->foreignTableName = foreignTableName;
      }
};

#endif /* DBFOREIGNKEY_H */

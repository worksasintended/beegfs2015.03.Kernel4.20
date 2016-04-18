#ifndef DBTABLE_H_
#define DBTABLE_H_

#include <database/DBField.h>
#include <database/DBForeignKey.h>
#include <database/DBIndex.h>

class DBTable
{
   public:
      DBTable() : tableName(""), autoIncID(false) { };
      DBTable(std::string tableName) : tableName(tableName), autoIncID(false) { };
      virtual ~DBTable() { }

   private:
      std::string tableName;
      DBFieldList fields;
      StringList primaryKey;
      DBForeignKeyList foreignKeys;
      DBIndexList indices;
      bool autoIncID;

   public:
      DBFieldList getFields() const
      {
         return fields;
      }

      DBForeignKeyList getForeignKeys() const
      {
         return foreignKeys;
      }

      DBIndexList getIndices() const
      {
         return indices;
      }

      StringList getPrimaryKey() const
      {
         return primaryKey;
      }

      std::string getTableName() const
      {
         return tableName;
      }

      bool getAutoIncID() const
      {
         return autoIncID;
      }

      void setAutoIncID(bool autoIncID)
      {
         this->autoIncID = autoIncID;
      }

      void setFields(DBFieldList fields)
      {
         this->fields = fields;
      }

      void setForeignKeys(DBForeignKeyList foreignKeys)
      {
         this->foreignKeys = foreignKeys;
      }

      void setIndices(DBIndexList indices)
      {
         this->indices = indices;
      }

      void setPrimaryKey(StringList primaryKey)
      {
         this->primaryKey = primaryKey;
      }

      void setTableName(std::string tableName)
      {
         this->tableName = tableName;
      }

      void addField(DBField field)
      {
         this->fields.push_back(field);
      }

      void addForeignKey(DBForeignKey foreignKey)
      {
         this->foreignKeys.push_back(foreignKey);
      }

      void addIndex(DBIndex index)
      {
         this->indices.push_back(index);
      }

      void addPrimaryKeyElement(std::string fieldName)
      {
         this->primaryKey.push_back(fieldName);
      }

};

#endif /* DBTABLE_H_ */

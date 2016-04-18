#ifndef DBTABLE_H_
#define DBTABLE_H_

#include <database/DBField.h>
#include <database/DBForeignKey.h>
#include <database/DBIndex.h>

class DBTable
{
   public:
      DBTable() : tableName(""), autoIncID(false) { };
      DBTable(std::string schemaName, std::string tableName) : schemaName(schemaName),
         tableName(tableName), autoIncID(false) { };
      DBTable(std::string tableName) : schemaName(tableName), tableName(tableName),
         autoIncID(false) { };
      virtual ~DBTable() { }

   private:
      std::string schemaName;
      std::string tableName;
      DBFieldList fields;
      StringList primaryKey;
      DBIndexList indices;
      bool autoIncID;

   public:
      DBFieldList getFields() const
      {
         return fields;
      }

      DBIndexList getIndices() const
      {
         return indices;
      }

      StringList getPrimaryKey() const
      {
         return primaryKey;
      }

      std::string getSchemaName() const
      {
         return schemaName;
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

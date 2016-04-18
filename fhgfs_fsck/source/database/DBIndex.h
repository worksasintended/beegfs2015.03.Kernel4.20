#ifndef DBINDEX_H
#define DBINDEX_H

#include <common/Common.h>

class DBIndex;

typedef std::list<DBIndex> DBIndexList;
typedef DBIndexList::iterator DBIndexListIter;

class DBIndex
{
   public:
      DBIndex(std::string indexName) : indexName(indexName), unique(false) { };

   private:
      std::string indexName;
      StringList fields;
      bool unique;

   public:
      std::string getIndexName() const
      {
         return indexName;
      }

      StringList getFields() const
      {
         return fields;
      }

      bool getUnique() const
      {
         return unique;
      }

      void setIndexName(std::string indexName)
      {
         this->indexName = indexName;
      }

      void setUnique(bool unique)
      {
         this->unique = unique;
      }

      void addField(std::string fieldName)
      {
         this->fields.push_back(fieldName);
      }

};

#endif /* DBINDEX_H */

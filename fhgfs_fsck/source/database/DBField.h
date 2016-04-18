#ifndef DBFIELD_H_
#define DBFIELD_H_

#include <common/Common.h>

enum DBDataType
{
   DBDataType_TEXT = 0,
   DBDataType_INT = 1,
   DBDataType_BOOL = 2
};

class DBField;

typedef std::list<DBField> DBFieldList;
typedef DBFieldList::iterator DBFieldListIter;

class DBField
{
   public:

   private:
      std::string fieldName;
      DBDataType fieldType;
      bool nullAllowed;
      bool unique;
      std::string defaultValue;

   public:
      DBField(std::string fieldName, DBDataType fieldType):
         fieldName(fieldName), fieldType(fieldType), nullAllowed(false), unique(false),
         defaultValue("") {};

      DBField(std::string fieldName, DBDataType fieldType, std::string defaultValue):
         fieldName(fieldName), fieldType(fieldType), nullAllowed(false), unique(false),
         defaultValue(defaultValue) { };

      std::string getDefaultValue() const
      {
         return this->defaultValue;
      }

      std::string getFieldName() const
      {
         return this->fieldName;
      }

      DBDataType getFieldType() const
      {
         return fieldType;
      }

      bool getNullAllowed() const
      {
         return nullAllowed;
      }

      bool getUnique() const
      {
         return unique;
      }

      void setDefaultValue(std::string defaultValue)
      {
         this->defaultValue = defaultValue;
      }

      void setFieldName(std::string fieldName)
      {
         this->fieldName = fieldName;
      }

      void setFieldType(DBDataType fieldType)
      {
         this->fieldType = fieldType;
      }

      void setNullAllowed(bool nullAllowed)
      {
         this->nullAllowed = nullAllowed;
      }

      void setUnique(bool unique)
      {
         this->unique = unique;
      }
};

#endif /* DBFIELD_H_ */

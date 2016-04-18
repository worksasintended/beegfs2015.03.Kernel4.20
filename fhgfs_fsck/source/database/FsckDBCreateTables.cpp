#include "FsckDB.h"

#include <database/FsckDBException.h>
#include <toolkit/FsckTkEx.h>

void FsckDB::createTables()
{
   createTableDirEntries();
   createTableFileInodes();
   createTableDirInodes();
   createTableChunks();
   createTableContDirs();
   createTableFsIDs();
   createTableUsedTargets();

   createTableDanglingDentry();
   createTableInodesWithWrongOwner();
   createTableDirEntriesWithWrongOwner();
   createTableDentriesWithBrokenByIDFile();
   createTableOrphanedDentryByIDFiles();
   createTableOrphanedDirInodes();
   createTableOrphanedFileInodes();
   createTableOrphanedChunks();
   createTableInodesWithoutContDir();
   createTableOrphanedContDirs();
   createTableWrongFileAttribs();
   createTableWrongDirAttribs();
   createTableMissingStorageTargets();
   createTableFilesWithMissingTargets();
   createTableMissingMirrorChunks();
   createTableMissingPrimaryChunks();
   createTableDifferingChunkAttribs();
   createTableChunksWithWrongPermissions();
   createTableChunksInWrongPath();

   createTableModificationEvents();
}

void FsckDB::createTable(DBTable& tableDefinition)
{
   // table MUST have a name
   if  ( tableDefinition.getTableName().empty() )
      throw FsckDBException("Cannot create table without a name");

   // table MUST have fields
   if  ( tableDefinition.getFields().empty() )
      throw FsckDBException("Cannot create table with empty field definitions");

   // table MUST have a primary key OR autoIncID set
   if  ( ( tableDefinition.getPrimaryKey().empty() ) && ( ! tableDefinition.getAutoIncID() ) )
      throw FsckDBException("Cannot create table without primary key");

   // assemble the sql field definitions
   std::string fieldDefinitions = "";

   DBFieldList fields = tableDefinition.getFields();
   for (DBFieldListIter fieldIter = fields.begin();
      fieldIter != fields.end(); fieldIter++)
   {
      DBField currentField = *fieldIter;
      std::string typeStr;
      switch (currentField.getFieldType())
      {
         case DBDataType_TEXT:
            typeStr = "TEXT";
            break;
         case DBDataType_INT:
            typeStr = "INTEGER";
            break;
         case DBDataType_BOOL:
            typeStr = "BOOL";
            break;
         default:
            typeStr = ""; //this will result in an error
            break;
      }

      std::string paramStr = "";
      if (! currentField.getNullAllowed())
         paramStr = " NOT NULL";
      if (currentField.getUnique())
         paramStr += " UNIQUE";
      if (! currentField.getDefaultValue().empty())
         paramStr += " DEFAULT " + currentField.getDefaultValue();

      // if autoIncID is set, and this is the first column, we set it primary key with
      // autoincrement enabled
      // Caution : the primary key list of the table model will be ignored in this case
      if ( ( tableDefinition.getAutoIncID()) && (fieldIter == fields.begin()))
         paramStr += " PRIMARY KEY AUTOINCREMENT";

      // fieldDefinitions ends with a ',', but that's ok, because we append primary key directly
      fieldDefinitions += currentField.getFieldName() + " " + typeStr + " " + paramStr + ",";
   }

   std::string primKeyStr = "";

   StringList primaryKey = tableDefinition.getPrimaryKey();
   // if autoIncID is set ignore the primary key list, but strip the last "," from the definition
   if (( tableDefinition.getAutoIncID()) || (primaryKey.empty()) )
      fieldDefinitions = fieldDefinitions.substr(0,fieldDefinitions.length()-1);
   else
   {
      // add the primary key
      StringListIter primKeyIter = primaryKey.begin();
      // first one exists, we know that from the check in the beginning
      primKeyStr = *primKeyIter;
      primKeyIter++;

      while (primKeyIter != primaryKey.end())
      {
         primKeyStr += ","+*primKeyIter;
         primKeyIter++;
      }

      primKeyStr = "PRIMARY KEY (" + primKeyStr + ")";
   }

   // check for any foreign keys
   std::string foreignKeysStr = "";
   DBForeignKeyList foreignKeys = tableDefinition.getForeignKeys();
   if (! foreignKeys.empty())
   {
      for (DBForeignKeyListIter iter = foreignKeys.begin(); iter != foreignKeys.end(); iter++)
      {
         DBForeignKey foreignKey = *iter;
         foreignKeysStr += ",FOREIGN KEY(" + foreignKey.getFieldName() + ") REFERENCES " +
            foreignKey.getForeignTableName() + "(" + foreignKey.getForeignFieldName() + ")";
      }
   }

   std::string query = "CREATE TABLE IF NOT EXISTS " + tableDefinition.getTableName() + "(" +
      fieldDefinitions + primKeyStr + foreignKeysStr +");";

   // check for any indices
   DBIndexList indices = tableDefinition.getIndices();
   if (! indices.empty())
   {
      for (DBIndexListIter iter = indices.begin(); iter != indices.end(); iter++)
      {
         DBIndex index = *iter;
         StringList fields = iter->getFields();

         std::string indexStr;
         if (iter->getUnique())
            indexStr = "CREATE UNIQUE INDEX IF NOT EXISTS ";
         else
            indexStr = "CREATE INDEX IF NOT EXISTS ";

         indexStr += iter->getIndexName() + " ON " + tableDefinition.getTableName() + "("
            + StringTk::implode(',', fields, true) + ");";

         query += indexStr;
      }
   }

   // create the table
   DBHandle *dbHandle = this->dbHandlePool->acquireHandle();
   char* sqlErrorMsg;

   int execResult = sqlite3_exec(dbHandle->getHandle(), query.c_str(), NULL, NULL, &sqlErrorMsg);

   this->dbHandlePool->releaseHandle(dbHandle);

   if (execResult != SQLITE_OK )
   {
      std::string sqlErrorMsgStr = sqlErrorMsg;
      // error string has to be freed with sqlite3_free (http://www.sqlite.org/c3ref/exec.html)
      sqlite3_free(sqlErrorMsg);
      throw FsckDBException(sqlErrorMsgStr.c_str());
   }
}

void FsckDB::createTableDirEntries()
{
   std::string tableName = TABLE_NAME_DIR_ENTRIES;
   DBTable tableDefinition(tableName);

   // NOTE : Keep this in sync with class FsckDirEntry
   tableDefinition.addField(DBField("id", DBDataType_TEXT));
   tableDefinition.addField(DBField("name", DBDataType_TEXT));
   tableDefinition.addField(DBField("parentDirID", DBDataType_TEXT));
   tableDefinition.addField(DBField("entryOwnerNodeID", DBDataType_INT));
   tableDefinition.addField(DBField("inodeOwnerNodeID", DBDataType_INT));
   tableDefinition.addField(DBField("entryType", DBDataType_INT));
   tableDefinition.addField(DBField("hasInlinedInode", DBDataType_BOOL));
   tableDefinition.addField(DBField("saveNodeID", DBDataType_INT));
   tableDefinition.addField(DBField("saveDevice", DBDataType_INT));
   // saveInode is uint64_t, but sqlite int type can't handle uint64; therefore we save this
   // as text
   tableDefinition.addField(DBField("saveInode", DBDataType_TEXT));

   /* add a field ignore errors which holds an integer, representing a bitmask, to determine a set
    * of error codes, for which this entry is ignored
    * This is useful if user chooses not to repair something
    * We also need this later to make fsck online-capable
    */
   tableDefinition.addField(DBField(IGNORE_ERRORS_FIELD, DBDataType_INT, "0"));

   DBIndex indexID("idx_" + tableName + "_id");
   indexID.addField("id");
   tableDefinition.addIndex(indexID);

   tableDefinition.addPrimaryKeyElement("parentDirID");
   tableDefinition.addPrimaryKeyElement("name");
   tableDefinition.addPrimaryKeyElement("saveNodeID");

   createTable(tableDefinition);
}

void FsckDB::createTableFileInodes()
{
   std::string tableName = TABLE_NAME_FILE_INODES;
   DBTable tableDefinition(tableName);

   // NOTE : Keep this in sync with class FsckFileInode
   tableDefinition.addField(DBField("internalID", DBDataType_INT));

   tableDefinition.addField(DBField("id", DBDataType_TEXT));
   tableDefinition.addField(DBField("parentDirID", DBDataType_TEXT));
   tableDefinition.addField(DBField("parentNodeID", DBDataType_INT));

   // pathInfo
   tableDefinition.addField(DBField("origParentUID", DBDataType_INT));
   tableDefinition.addField(DBField("origParentEntryID", DBDataType_TEXT));
   tableDefinition.addField(DBField("pathInfoFlags", DBDataType_INT));

   tableDefinition.addField(DBField("mode", DBDataType_INT));
   tableDefinition.addField(DBField("userID", DBDataType_INT));
   tableDefinition.addField(DBField("groupID", DBDataType_INT));

   tableDefinition.addField(DBField("fileSize", DBDataType_INT));
   // usedBlocks is uint64_t, but sqlite int type can't handle uint64; therefore we save this
   // as text
   tableDefinition.addField(DBField("usedBlocks", DBDataType_TEXT));
   tableDefinition.addField(DBField("creationTime", DBDataType_INT));
   tableDefinition.addField(DBField("modificationTime", DBDataType_INT));
   tableDefinition.addField(DBField("lastAccessTime", DBDataType_INT));
   tableDefinition.addField(DBField("numHardLinks", DBDataType_INT));

   tableDefinition.addField(DBField("stripePatternType", DBDataType_INT));
   tableDefinition.addField(DBField("stripeTargets", DBDataType_TEXT));
   tableDefinition.addField(DBField("chunkSize", DBDataType_INT));

   tableDefinition.addField(DBField("saveNodeID", DBDataType_INT));
   tableDefinition.addField(DBField("isInlined", DBDataType_BOOL));

   tableDefinition.addField(DBField("readable", DBDataType_BOOL));

   /* add a field ignore errors which holds an integer, representing a bitmask, to determine a set
    * of error codes, for which this entry is ignored
    * This is useful if user chooses not to repair something
    * We also need this later to make fsck online-capable
    */
   tableDefinition.addField(DBField(IGNORE_ERRORS_FIELD, DBDataType_INT, "0"));

 /*  tableDefinition.addPrimaryKeyElement("id");
   tableDefinition.addPrimaryKeyElement("saveNodeID"); */

   tableDefinition.setAutoIncID(true);

   DBIndex indexID("idx_" + tableName + "_id");
   indexID.addField("id");
   tableDefinition.addIndex(indexID);

   DBIndex indexSaveNodeID("idx_" + tableName + "_savenodeid");
   indexSaveNodeID.addField("saveNodeID");
   tableDefinition.addIndex(indexSaveNodeID);

   DBIndex indexIDSaveNodeID("idx_" + tableName + "_id_savenodeid");
   indexIDSaveNodeID.addField("id");
   indexIDSaveNodeID.addField("saveNodeID");
   tableDefinition.addIndex(indexIDSaveNodeID);

   /* consider inodes to be equal if this matches */
   DBIndex indexUnique("idx_" + tableName + "_id_unique"); /* consider inodes to be equal if this matches */
   indexUnique.setUnique(true);
   indexUnique.addField("id");
   indexUnique.addField("parentDirID");
   indexUnique.addField("parentNodeID");
   indexUnique.addField("saveNodeID");
   tableDefinition.addIndex(indexUnique);

   createTable(tableDefinition);
}

void FsckDB::createTableDirInodes()
{
   std::string tableName = TABLE_NAME_DIR_INODES;
   DBTable tableDefinition(tableName);

   // NOTE : Keep this in sync with class FsckDirInode
   tableDefinition.addField(DBField("id", DBDataType_TEXT));
   tableDefinition.addField(DBField("parentDirID", DBDataType_TEXT));
   tableDefinition.addField(DBField("parentNodeID", DBDataType_INT));
   tableDefinition.addField(DBField("ownerNodeID", DBDataType_INT));

   tableDefinition.addField(DBField("size", DBDataType_INT));
   tableDefinition.addField(DBField("numHardLinks", DBDataType_INT));

   tableDefinition.addField(DBField("stripePatternType", DBDataType_INT));
   tableDefinition.addField(DBField("stripeTargets", DBDataType_TEXT));

   tableDefinition.addField(DBField("saveNodeID", DBDataType_INT));

   tableDefinition.addField(DBField("readable", DBDataType_BOOL));

   /* add a field ignore errors which holds an integer, representing a bitmask, to determine a set
    * of error codes, for which this entry is ignored
    * This is useful if user chooses not to repair something
    * We also need this later to make fsck online-capable
    */
   tableDefinition.addField(DBField(IGNORE_ERRORS_FIELD, DBDataType_INT, "0"));

   DBIndex indexID("idx_" + tableName + "_id");
   indexID.addField("id");
   tableDefinition.addIndex(indexID);

   tableDefinition.addPrimaryKeyElement("id");
   tableDefinition.addPrimaryKeyElement("saveNodeID");

   createTable(tableDefinition);
}

void FsckDB::createTableChunks()
{
   std::string tableName = TABLE_NAME_CHUNKS;
   DBTable tableDefinition(tableName);

   // NOTE : Keep this in sync with class FsckChunk
   tableDefinition.addField(DBField("id", DBDataType_TEXT));
   tableDefinition.addField(DBField("targetID", DBDataType_INT));

   // the path, where the chunk was found; relative to storeStorageDirectory and (in case of
   // mirrored targets) also relative to xyz.mirror directory
   tableDefinition.addField(DBField("savedPath", DBDataType_TEXT));

   tableDefinition.addField(DBField("fileSize", DBDataType_INT));
   // usedBlocks is uint64_t, but sqlite int type can't handle uint64; therefore we save this
   // as text
   tableDefinition.addField(DBField("usedBlocks", DBDataType_TEXT));
   tableDefinition.addField(DBField("creationTime", DBDataType_INT));
   tableDefinition.addField(DBField("modificationTime", DBDataType_INT));
   tableDefinition.addField(DBField("lastAccessTime", DBDataType_INT));

   tableDefinition.addField(DBField("userID", DBDataType_INT));
   tableDefinition.addField(DBField("groupID", DBDataType_INT));

   tableDefinition.addField(DBField("buddyGroupID", DBDataType_INT));
   /* add a field ignore errors which holds an integer, representing a bitmask, to determine a set
    * of error codes, for which this entry is ignored
    * This is useful if user chooses not to repair something
    * We also need this later to make fsck online-capable
    */
   tableDefinition.addField(DBField(IGNORE_ERRORS_FIELD, DBDataType_INT, "0"));

   DBIndex indexID("idx_" + tableName + "_id");
   indexID.addField("id");
   tableDefinition.addIndex(indexID);

   tableDefinition.addPrimaryKeyElement("id");
   tableDefinition.addPrimaryKeyElement("targetID");
   tableDefinition.addPrimaryKeyElement("buddyGroupID");

   createTable(tableDefinition);
}

void FsckDB::createTableContDirs()
{
   std::string tableName = TABLE_NAME_CONT_DIRS;
   DBTable tableDefinition(tableName);

   // NOTE : Keep this in sync with class FsckContDir
   tableDefinition.addField(DBField("id", DBDataType_TEXT));
   tableDefinition.addField(DBField("saveNodeID", DBDataType_INT));

   /* add a field ignore errors which holds an integer, representing a bitmask, to determine a set
    * of error codes, for which this entry is ignored
    * This is useful if user chooses not to repair something
    * We also need this later to make fsck online-capable
    */
   tableDefinition.addField(DBField(IGNORE_ERRORS_FIELD, DBDataType_INT, "0"));

   tableDefinition.addPrimaryKeyElement("id");
   tableDefinition.addPrimaryKeyElement("saveNodeID");

   createTable(tableDefinition);
}

void FsckDB::createTableFsIDs()
{
   std::string tableName = TABLE_NAME_FSIDS;
   DBTable tableDefinition(tableName);

   // NOTE : Keep this in sync with class FsckContDir
   tableDefinition.addField(DBField("id", DBDataType_TEXT));
   tableDefinition.addField(DBField("parentDirID", DBDataType_TEXT));
   tableDefinition.addField(DBField("saveNodeID", DBDataType_INT));
   tableDefinition.addField(DBField("saveDevice", DBDataType_INT));
   // saveInode is uint64_t, but sqlite int type can't handle uint64; therefore we save this
   // as text
   tableDefinition.addField(DBField("saveInode", DBDataType_TEXT));

   /* add a field ignore errors which holds an integer, representing a bitmask, to determine a set
    * of error codes, for which this entry is ignored
    * This is useful if user chooses not to repair something
    * We also need this later to make fsck online-capable
    */
   tableDefinition.addField(DBField(IGNORE_ERRORS_FIELD, DBDataType_INT, "0"));

   DBIndex indexID("idx_" + tableName + "_id");
   indexID.addField("id");
   tableDefinition.addIndex(indexID);

   tableDefinition.addPrimaryKeyElement("id");
   tableDefinition.addPrimaryKeyElement("parentDirID");
   tableDefinition.addPrimaryKeyElement("saveNodeID");

   createTable(tableDefinition);
}

/*
 * This table just keeps track of the targets (and buddy groups), which are used in the stripe
 * patterns; its a fairly simple table(), but it makes life easier, when matching used and really
 * existing targets
 */
void FsckDB::createTableUsedTargets()
{
   std::string tableName = TABLE_NAME_USED_TARGETS;
   DBTable tableDefinition(tableName);

   tableDefinition.addField(DBField("id", DBDataType_INT));
   tableDefinition.addField(DBField("targetIDType", DBDataType_INT));

   /* add a field ignore errors which holds an integer, representing a bitmask, to determine a set
    * of error codes, for which this entry is ignored
    * This is useful if user chooses not to repair something
    * We also need this later to make fsck online-capable
    */
   tableDefinition.addField(DBField(IGNORE_ERRORS_FIELD, DBDataType_INT, "0"));

   tableDefinition.addPrimaryKeyElement("id");
   tableDefinition.addPrimaryKeyElement("targetIDType");

   createTable(tableDefinition);
}

void FsckDB::createTableDanglingDentry()
{
   DBTable tableDefinition(TABLE_NAME_ERROR(FsckErrorCode_DANGLINGDENTRY));

   tableDefinition.addField(DBField("name", DBDataType_TEXT));
   tableDefinition.addField(DBField("parentDirID", DBDataType_TEXT));
   tableDefinition.addField(DBField("saveNodeID", DBDataType_INT));

   std::string defaultValue = StringTk::intToStr((int)FsckRepairAction_UNDEFINED);
   tableDefinition.addField(DBField("repairAction", DBDataType_INT, defaultValue));

   tableDefinition.addPrimaryKeyElement("name");
   tableDefinition.addPrimaryKeyElement("parentDirID");
   tableDefinition.addPrimaryKeyElement("saveNodeID");

   std::string foreignTableName = TABLE_NAME_DIR_ENTRIES;
   DBForeignKey foreignKey("name,parentDirID,saveNodeID", foreignTableName,
      "name,parentDirID,saveNodeID");
   tableDefinition.addForeignKey(foreignKey);

   createTable(tableDefinition);
}

void FsckDB::createTableInodesWithWrongOwner()
{
   DBTable tableDefinition(TABLE_NAME_ERROR(FsckErrorCode_WRONGINODEOWNER));

   tableDefinition.addField(DBField("id", DBDataType_TEXT));
   tableDefinition.addField(DBField("saveNodeID", DBDataType_INT));

   std::string defaultValue = StringTk::intToStr((int)FsckRepairAction_UNDEFINED);
   tableDefinition.addField(DBField("repairAction", DBDataType_INT, defaultValue));

   tableDefinition.addPrimaryKeyElement("id");
   tableDefinition.addPrimaryKeyElement("saveNodeID");

   std::string foreignTableName = TABLE_NAME_DIR_INODES;
   DBForeignKey foreignKey("id,saveNodeID", foreignTableName, "id,saveNodeID");
   tableDefinition.addForeignKey(foreignKey);

   createTable(tableDefinition);
}

void FsckDB::createTableDirEntriesWithWrongOwner()
{
   DBTable tableDefinition(TABLE_NAME_ERROR(FsckErrorCode_WRONGOWNERINDENTRY));

   tableDefinition.addField(DBField("name", DBDataType_TEXT));
   tableDefinition.addField(DBField("parentDirID", DBDataType_TEXT));
   tableDefinition.addField(DBField("saveNodeID", DBDataType_INT));

   std::string defaultValue = StringTk::intToStr((int)FsckRepairAction_UNDEFINED);
   tableDefinition.addField(DBField("repairAction", DBDataType_INT, defaultValue));

   tableDefinition.addPrimaryKeyElement("name");
   tableDefinition.addPrimaryKeyElement("parentDirID");
   tableDefinition.addPrimaryKeyElement("saveNodeID");

   std::string foreignTableName = TABLE_NAME_DIR_ENTRIES;
   DBForeignKey foreignKey("name,parentDirID,saveNodeID", foreignTableName,
      "name,parentDirID,saveNodeID");
   tableDefinition.addForeignKey(foreignKey);

   createTable(tableDefinition);
}

void FsckDB::createTableDentriesWithBrokenByIDFile()
{
   DBTable tableDefinition(TABLE_NAME_ERROR(FsckErrorCode_BROKENFSID));

   tableDefinition.addField(DBField("name", DBDataType_TEXT));
   tableDefinition.addField(DBField("parentDirID", DBDataType_TEXT));
   tableDefinition.addField(DBField("saveNodeID", DBDataType_INT));

   std::string defaultValue = StringTk::intToStr((int)FsckRepairAction_UNDEFINED);
   tableDefinition.addField(DBField("repairAction", DBDataType_INT, defaultValue));

   tableDefinition.addPrimaryKeyElement("name");
   tableDefinition.addPrimaryKeyElement("parentDirID");
   tableDefinition.addPrimaryKeyElement("saveNodeID");

   std::string foreignTableName = TABLE_NAME_DIR_ENTRIES;
   DBForeignKey foreignKey("name,parentDirID,saveNodeID", foreignTableName,
      "name,parentDirID,saveNodeID");
   tableDefinition.addForeignKey(foreignKey);

   createTable(tableDefinition);
}

void FsckDB::createTableOrphanedDentryByIDFiles()
{
   DBTable tableDefinition(TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDFSID));

   tableDefinition.addField(DBField("id", DBDataType_TEXT));
   tableDefinition.addField(DBField("parentDirID", DBDataType_TEXT));
   tableDefinition.addField(DBField("saveNodeID", DBDataType_TEXT));

   std::string defaultValue = StringTk::intToStr((int)FsckRepairAction_UNDEFINED);
   tableDefinition.addField(DBField("repairAction", DBDataType_INT, defaultValue));

   tableDefinition.addPrimaryKeyElement("id");
   tableDefinition.addPrimaryKeyElement("parentDirID");
   tableDefinition.addPrimaryKeyElement("saveNodeID");

   std::string foreignTableName = TABLE_NAME_FSIDS;
   DBForeignKey foreignKey("id,parentDirID,saveNodeID", foreignTableName,
      "id,parentDirID,saveNodeID");
   tableDefinition.addForeignKey(foreignKey);

   createTable(tableDefinition);
}

void FsckDB::createTableOrphanedDirInodes()
{
   DBTable tableDefinition(TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDDIRINODE));

   tableDefinition.addField(DBField("id", DBDataType_TEXT));
   tableDefinition.addField(DBField("saveNodeID", DBDataType_INT));

   std::string defaultValue = StringTk::intToStr((int)FsckRepairAction_UNDEFINED);
   tableDefinition.addField(DBField("repairAction", DBDataType_INT, defaultValue));

   tableDefinition.addPrimaryKeyElement("id");
   tableDefinition.addPrimaryKeyElement("saveNodeID");

   std::string foreignTableName = TABLE_NAME_DIR_INODES;
   DBForeignKey foreignKey("id,saveNodeID", foreignTableName, "id,saveNodeID");
   tableDefinition.addForeignKey(foreignKey);

   createTable(tableDefinition);
}

void FsckDB::createTableOrphanedFileInodes()
{
   DBTable tableDefinition(TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDFILEINODE));

 /*  tableDefinition.addField(DBField("id", DBDataType_TEXT));
   tableDefinition.addField(DBField("saveNodeID", DBDataType_INT)); */

   tableDefinition.addField(DBField("internalID", DBDataType_INT));

   std::string defaultValue = StringTk::intToStr((int)FsckRepairAction_UNDEFINED);
   tableDefinition.addField(DBField("repairAction", DBDataType_INT, defaultValue));

   tableDefinition.addPrimaryKeyElement("internalID");

   /* tableDefinition.addPrimaryKeyElement("id");
   tableDefinition.addPrimaryKeyElement("saveNodeID"); */

   std::string foreignTableName = TABLE_NAME_FILE_INODES;
  // DBForeignKey foreignKey("id,saveNodeID", foreignTableName, "id,saveNodeID");
   DBForeignKey foreignKey("internalID", foreignTableName, "internalID");
   tableDefinition.addForeignKey(foreignKey);

   createTable(tableDefinition);
}

void FsckDB::createTableOrphanedChunks()
{
   DBTable tableDefinition(TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDCHUNK));

   tableDefinition.addField(DBField("id", DBDataType_TEXT));
   tableDefinition.addField(DBField("targetID", DBDataType_INT));

   // only primary chunks will go here, nevertheless we need the field to comply with primary key
   tableDefinition.addField(DBField("buddyGroupID", DBDataType_INT, "0"));

   std::string defaultValue = StringTk::intToStr((int)FsckRepairAction_UNDEFINED);
   tableDefinition.addField(DBField("repairAction", DBDataType_INT, defaultValue));

   tableDefinition.addPrimaryKeyElement("id");
   tableDefinition.addPrimaryKeyElement("targetID");
   tableDefinition.addPrimaryKeyElement("buddyGroupID");

   std::string foreignTableName = TABLE_NAME_CHUNKS;
   DBForeignKey foreignKey("id,targetID,buddyGroupID", foreignTableName, "id,targetID,buddyGroupID");
   tableDefinition.addForeignKey(foreignKey);

   createTable(tableDefinition);
}

void FsckDB::createTableInodesWithoutContDir()
{
   DBTable tableDefinition(TABLE_NAME_ERROR(FsckErrorCode_MISSINGCONTDIR));

   tableDefinition.addField(DBField("id", DBDataType_TEXT));
   tableDefinition.addField(DBField("saveNodeID", DBDataType_INT));

   std::string defaultValue = StringTk::intToStr((int)FsckRepairAction_UNDEFINED);
   tableDefinition.addField(DBField("repairAction", DBDataType_INT, defaultValue));

   tableDefinition.addPrimaryKeyElement("id");
   tableDefinition.addPrimaryKeyElement("saveNodeID");

   std::string foreignTableName = TABLE_NAME_DIR_INODES;
   DBForeignKey foreignKey("id,saveNodeID", foreignTableName, "id,saveNodeID");
   tableDefinition.addForeignKey(foreignKey);

   createTable(tableDefinition);
}

void FsckDB::createTableOrphanedContDirs()
{
   DBTable tableDefinition(TABLE_NAME_ERROR(FsckErrorCode_ORPHANEDCONTDIR));

   tableDefinition.addField(DBField("id", DBDataType_TEXT));
   tableDefinition.addField(DBField("saveNodeID", DBDataType_INT));

   std::string defaultValue = StringTk::intToStr((int)FsckRepairAction_UNDEFINED);
   tableDefinition.addField(DBField("repairAction", DBDataType_INT, defaultValue));

   tableDefinition.addPrimaryKeyElement("id");
   tableDefinition.addPrimaryKeyElement("saveNodeID");

   std::string foreignTableName = TABLE_NAME_CONT_DIRS;
   DBForeignKey foreignKey("id,saveNodeID", foreignTableName, "id,saveNodeID");
   tableDefinition.addForeignKey(foreignKey);

   createTable(tableDefinition);
}

void FsckDB::createTableWrongFileAttribs()
{
   DBTable tableDefinition(TABLE_NAME_ERROR(FsckErrorCode_WRONGFILEATTRIBS));

   tableDefinition.addField(DBField("internalID", DBDataType_INT));

/*   tableDefinition.addField(DBField("id", DBDataType_TEXT));
   tableDefinition.addField(DBField("saveNodeID", DBDataType_INT)); */

   std::string defaultValue = StringTk::intToStr((int)FsckRepairAction_UNDEFINED);
   tableDefinition.addField(DBField("repairAction", DBDataType_INT, defaultValue));

/* tableDefinition.addPrimaryKeyElement("id");

   tableDefinition.addPrimaryKeyElement("saveNodeID");

   std::string foreignTableName = TABLE_NAME_FILE_INODES;
   DBForeignKey foreignKey("id,saveNodeID", foreignTableName, "id,saveNodeID");
   tableDefinition.addForeignKey(foreignKey); */

   tableDefinition.addPrimaryKeyElement("internalID");

   std::string foreignTableName = TABLE_NAME_FILE_INODES;
   DBForeignKey foreignKey("internalID", foreignTableName, "internalID");
   tableDefinition.addForeignKey(foreignKey);

   createTable(tableDefinition);
}

void FsckDB::createTableWrongDirAttribs()
{
   DBTable tableDefinition(TABLE_NAME_ERROR(FsckErrorCode_WRONGDIRATTRIBS));

   tableDefinition.addField(DBField("id", DBDataType_TEXT));
   tableDefinition.addField(DBField("saveNodeID", DBDataType_INT));

   std::string defaultValue = StringTk::intToStr((int)FsckRepairAction_UNDEFINED);
   tableDefinition.addField(DBField("repairAction", DBDataType_INT, defaultValue));

   tableDefinition.addPrimaryKeyElement("id");
   tableDefinition.addPrimaryKeyElement("saveNodeID");

   std::string foreignTableName = TABLE_NAME_DIR_INODES;
   DBForeignKey foreignKey("id,saveNodeID", foreignTableName, "id,saveNodeID");
   tableDefinition.addForeignKey(foreignKey);

   createTable(tableDefinition);
}

/*
 * crates a table, in which storage targets will be saved, which are found in stripe patterns, but
 * do not exist
 */
void FsckDB::createTableMissingStorageTargets()
{
   DBTable tableDefinition(TABLE_NAME_ERROR(FsckErrorCode_MISSINGTARGET));

   tableDefinition.addField(DBField("id", DBDataType_INT)); // id of the target
   tableDefinition.addField(DBField("targetIDType", DBDataType_INT)); // target or buddyMirrorGroup

   std::string defaultValue = StringTk::intToStr((int)FsckRepairAction_UNDEFINED);
   tableDefinition.addField(DBField("repairAction", DBDataType_INT, defaultValue));

   tableDefinition.addPrimaryKeyElement("id");
   tableDefinition.addPrimaryKeyElement("targetIDType");

   createTable(tableDefinition);
}

/*
 * crates a table, in which files will be saved, which have non-existent targets in their
 * stripe pattern (the dentries are saved in here, because they are needed to delete the whole file)
 */
void FsckDB::createTableFilesWithMissingTargets()
{
   DBTable tableDefinition(TABLE_NAME_ERROR(FsckErrorCode_FILEWITHMISSINGTARGET));

   tableDefinition.addField(DBField("name", DBDataType_TEXT));
   tableDefinition.addField(DBField("parentDirID", DBDataType_TEXT));
   tableDefinition.addField(DBField("saveNodeID", DBDataType_INT));

   std::string defaultValue = StringTk::intToStr((int)FsckRepairAction_UNDEFINED);
   tableDefinition.addField(DBField("repairAction", DBDataType_INT, defaultValue));

   tableDefinition.addPrimaryKeyElement("name");
   tableDefinition.addPrimaryKeyElement("parentDirID");
   tableDefinition.addPrimaryKeyElement("saveNodeID");

   std::string foreignTableName = TABLE_NAME_DIR_ENTRIES;
   DBForeignKey foreignKey("name,parentDirID,saveNodeID", foreignTableName,
      "name,parentDirID,saveNodeID");
   tableDefinition.addForeignKey(foreignKey);

   createTable(tableDefinition);
}

void FsckDB::createTableMissingMirrorChunks()
{
  /* DBTable tableDefinition(TABLE_NAME_ERROR(FsckErrorCode_MISSINGMIRRORCHUNK));

   tableDefinition.addField(DBField("id", DBDataType_TEXT));
   tableDefinition.addField(DBField("targetID", DBDataType_INT));

   // only primary chunks will go here, nevertheless we need the field to comply with primary key
   tableDefinition.addField(DBField("buddyGroupID", DBDataType_INT, "0"));

   std::string defaultValue = StringTk::intToStr((int)FsckRepairAction_UNDEFINED);
   tableDefinition.addField(DBField("repairAction", DBDataType_INT, defaultValue));

   tableDefinition.addPrimaryKeyElement("id");
   tableDefinition.addPrimaryKeyElement("targetID");
   tableDefinition.addPrimaryKeyElement("buddyGroupID");

   std::string foreignTableName = TABLE_NAME_CHUNKS;
   DBForeignKey foreignKey("id,targetID,buddyGroupID", foreignTableName, "id,targetID,buddyGroupID");
   tableDefinition.addForeignKey(foreignKey);

   createTable(tableDefinition); */
}

void FsckDB::createTableMissingPrimaryChunks()
{
/*  DBTable tableDefinition(TABLE_NAME_ERROR(FsckErrorCode_MISSINGPRIMARYCHUNK));

   tableDefinition.addField(DBField("id", DBDataType_TEXT));
   tableDefinition.addField(DBField("targetID", DBDataType_INT));

   // only primary chunks will go here, nevertheless we need the field to comply with primary key
   tableDefinition.addField(DBField("buddyGroupID", DBDataType_INT, "0"));

   std::string defaultValue = StringTk::intToStr((int)FsckRepairAction_UNDEFINED);
   tableDefinition.addField(DBField("repairAction", DBDataType_INT, defaultValue));

   tableDefinition.addPrimaryKeyElement("id");
   tableDefinition.addPrimaryKeyElement("targetID");
   tableDefinition.addPrimaryKeyElement("buddyGroupID");

   std::string foreignTableName = TABLE_NAME_CHUNKS;
   DBForeignKey foreignKey("id,targetID,buddyGroupID", foreignTableName, "id,targetID,buddyGroupID");
   tableDefinition.addForeignKey(foreignKey);

   createTable(tableDefinition); */
}

void FsckDB::createTableDifferingChunkAttribs()
{
 /*  DBTable tableDefinition(TABLE_NAME_ERROR(FsckErrorCode_DIFFERINGCHUNKATTRIBS));

   tableDefinition.addField(DBField("id", DBDataType_TEXT));
   tableDefinition.addField(DBField("targetID", DBDataType_INT));

   // only primary chunks will go here, nevertheless we need the field to comply with primary key
   tableDefinition.addField(DBField("mirrorOf", DBDataType_INT, "0"));

   std::string defaultValue = StringTk::intToStr((int)FsckRepairAction_UNDEFINED);
   tableDefinition.addField(DBField("repairAction", DBDataType_INT, defaultValue));

   tableDefinition.addPrimaryKeyElement("id");
   tableDefinition.addPrimaryKeyElement("targetID");
   tableDefinition.addPrimaryKeyElement("mirrorOf");

   std::string foreignTableName = TABLE_NAME_CHUNKS;
   DBForeignKey foreignKey("id,targetID,mirrorOf", foreignTableName, "id,targetID,mirrorOf");
   tableDefinition.addForeignKey(foreignKey);

   createTable(tableDefinition); */
}

/*
 * table for chunks with wrong owner/group
 */
void FsckDB::createTableChunksWithWrongPermissions()
{
   DBTable tableDefinition(TABLE_NAME_ERROR(FsckErrorCode_CHUNKWITHWRONGPERM));

   tableDefinition.addField(DBField("id", DBDataType_TEXT));
   tableDefinition.addField(DBField("targetID", DBDataType_INT));
   tableDefinition.addField(DBField("buddyGroupID", DBDataType_INT, "0"));

   std::string defaultValue = StringTk::intToStr((int)FsckRepairAction_UNDEFINED);
   tableDefinition.addField(DBField("repairAction", DBDataType_INT, defaultValue));

   tableDefinition.addPrimaryKeyElement("id");
   tableDefinition.addPrimaryKeyElement("targetID");
   tableDefinition.addPrimaryKeyElement("buddyGroupID");

   std::string foreignTableName = TABLE_NAME_CHUNKS;
   DBForeignKey foreignKey("id,targetID,buddyGroupID", foreignTableName, "id,targetID,buddyGroupID");
   tableDefinition.addForeignKey(foreignKey);

   createTable(tableDefinition);
}

void FsckDB::createTableChunksInWrongPath()
{
   DBTable tableDefinition(TABLE_NAME_ERROR(FsckErrorCode_CHUNKINWRONGPATH));

   tableDefinition.addField(DBField("id", DBDataType_TEXT));
   tableDefinition.addField(DBField("targetID", DBDataType_INT));
   tableDefinition.addField(DBField("buddyGroupID", DBDataType_INT, "0"));

   std::string defaultValue = StringTk::intToStr((int)FsckRepairAction_UNDEFINED);
   tableDefinition.addField(DBField("repairAction", DBDataType_INT, defaultValue));

   tableDefinition.addPrimaryKeyElement("id");
   tableDefinition.addPrimaryKeyElement("targetID");
   tableDefinition.addPrimaryKeyElement("buddyGroupID");

   std::string foreignTableName = TABLE_NAME_CHUNKS;
   DBForeignKey foreignKey("id,targetID,buddyGroupID", foreignTableName, "id,targetID,buddyGroupID");
   tableDefinition.addForeignKey(foreignKey);

   createTable(tableDefinition);
}


/*
 * creates a table to save modification events received from meta servers
 */
void FsckDB::createTableModificationEvents()
{
   std::string tableName = TABLE_NAME_MODIFICATION_EVENTS;
   DBTable tableDefinition(tableName);

   tableDefinition.addField(DBField("eventType", DBDataType_INT));
   tableDefinition.addField(DBField("entryID", DBDataType_TEXT));

   tableDefinition.addPrimaryKeyElement("eventType");
   tableDefinition.addPrimaryKeyElement("entryID");

   createTable(tableDefinition);
}

#include "MapTk.h"

#include <fstream>


#define MAPTK_SAVE_TMPEXTENSION  ".tmp" /* extension for temporary files */


/**
 * Splits a line into param and value (divided by the '='-char) and adds it to
 * the map.
 */
void MapTk::addLineToStringMap(std::string line, StringMap* outMap)
{
   std::string::size_type divPos = line.find_first_of("=", 0);

   if(divPos == std::string::npos)
   {
      stringMapRedefine(StringTk::trim(line), "", outMap);
      return;
   }

   std::string param = line.substr(0, divPos);
   std::string value = line.substr(divPos + 1);

   stringMapRedefine(StringTk::trim(param), StringTk::trim(value), outMap);
}

void MapTk::loadStringMapFromFile(const char* filename, StringMap* outMap)
   throw(InvalidConfigException)
{
   char line[STORAGETK_FILE_MAX_LINE_LENGTH];

   std::ifstream fis(filename);
   if(!fis.is_open() || fis.fail() )
   {
      throw InvalidConfigException(
         std::string("Failed to load map file: ") + filename);
   }

   while(!fis.eof() && !fis.fail() )
   {
      fis.getline(line, STORAGETK_FILE_MAX_LINE_LENGTH);
      std::string trimmedLine = StringTk::trim(line);
      if(trimmedLine.length() && (trimmedLine[0] != STORAGETK_FILE_COMMENT_CHAR) )
         addLineToStringMap(trimmedLine, outMap);
   }

   fis.close();
}

/**
 * Note: Saves to a tmp file first and then renames the tmp file.
 * Note: Has a special handling for keys starting with STORAGETK_FILE_COMMENT_CHAR.
 */
void MapTk::saveStringMapToFile(const char* filename, StringMap* map)
   throw(InvalidConfigException)
{
   std::string tmpFilename = std::string(filename) + MAPTK_SAVE_TMPEXTENSION;

   std::string line;

   std::ofstream file(tmpFilename.c_str(),
      std::ios_base::out | std::ios_base::in | std::ios_base::trunc);

   if(!file.is_open() || file.fail() )
      throw InvalidConfigException(
         std::string("Failed to open map file for writing: ") + tmpFilename);

   for(StringMapCIter iter = map->begin(); (iter != map->end() ) && !file.fail(); iter++)
   {
      if(!iter->first.empty() && (iter->first[0] != STORAGETK_FILE_COMMENT_CHAR) )
         file << iter->first << "=" << iter->second << std::endl;
      else
         file << iter->first << std::endl; // only "iter->first" for comment or empty lines
   }

   if(file.fail() )
   {
      file.close();

      throw InvalidConfigException(
         std::string("Failed to save data to map file: ") + tmpFilename);
   }

   file.close();

   // rename tmp file to actual name

   int renameRes = rename(tmpFilename.c_str(), filename);
   if(renameRes == -1)
      throw InvalidConfigException("Failed to rename tmp map file: " +
         tmpFilename + " -> " + std::string(filename) + "; "
         "SysErr: " + System::getErrString() );
}

void MapTk::copyUInt64VectorMap(std::map<uint64_t, UInt64Vector*> &inMap,
   std::map<uint64_t, UInt64Vector*> &outMap)
{
   std::map<uint64_t, UInt64Vector*>::iterator mapIter = inMap.begin();

   for ( ; mapIter != inMap.end(); mapIter++)
   {
      UInt64Vector* vector = new UInt64Vector();

      for (UInt64VectorIter vectorIter = mapIter->second->begin();
         vectorIter != mapIter->second->end(); vectorIter++)
      {
         uint64_t value = *vectorIter;
         vector->push_back(value);
      }

      outMap.insert(std::pair<uint64_t, UInt64Vector*>(mapIter->first ,vector));
   }
}

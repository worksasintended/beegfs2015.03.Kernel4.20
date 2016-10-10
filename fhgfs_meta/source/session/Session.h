#ifndef SESSION_H_
#define SESSION_H_

#include <common/Common.h>
#include "SessionFileStore.h"


class Session
{
   public:
      Session(std::string sessionID) : sessionID(sessionID) {}

      /*
       * For deserialization only
       */
      Session() {};

      void mergeSessionFiles(Session* session);

      unsigned serialize(char* buf);
      bool deserialize(const char* buf, size_t bufLen, unsigned* outLen);
      unsigned serialLen();

      friend bool sessionMetaEquals(Session* first, Session* second, bool disableInodeCheck);


   private:
      std::string sessionID;
      
      SessionFileStore files;
      
   public:
      // getters & setters
      std::string getSessionID()
      {
         return sessionID;
      }
      
      SessionFileStore* getFiles()
      {
         return &files;
      }
      
      bool relinkInodes(MetaStore& store)
      {
         return files.relinkInodes(store);
      }

};


#endif /*SESSION_H_*/

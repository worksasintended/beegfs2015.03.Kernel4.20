#ifndef SESSION_H_
#define SESSION_H_

#include <common/Common.h>
#include "SessionLocalFileStore.h"


class Session
{
   public:
      Session(std::string sessionID) : sessionID(sessionID) {}

      /*
       * For deserialization only
       */
      Session() {};

      void mergeSessionLocalFiles(Session* session);

      unsigned serializeForTarget(char* buf, uint16_t targetID);
      bool deserializeForTarget(const char* buf, size_t bufLen, unsigned* outLen,
         uint16_t targetID);
      unsigned serialLenForTarget(uint16_t targetID);

   private:
      std::string sessionID;

      SessionLocalFileStore localFiles;

   public:
      // getters & setters
      std::string getSessionID()
      {
         return sessionID;
      }

      SessionLocalFileStore* getLocalFiles()
      {
         return &localFiles;
      }
};


#endif /*SESSION_H_*/

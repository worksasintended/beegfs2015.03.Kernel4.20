#ifndef SESSIONSTORE_H_
#define SESSIONSTORE_H_

#include <common/nodes/Node.h>
#include <common/toolkit/ObjectReferencer.h>
#include <common/threading/Mutex.h>
#include <common/Common.h>
#include "Session.h"

typedef ObjectReferencer<Session*> SessionReferencer;
typedef std::map<std::string, SessionReferencer*> SessionMap;
typedef SessionMap::iterator SessionMapIter;
typedef SessionMap::value_type SessionMapVal;

typedef std::list<Session*> SessionList;
typedef SessionList::iterator SessionListIter;


class SessionStore
{
   public:
      SessionStore() {}

      Session* referenceSession(std::string sessionID, bool addIfNotExists=true);
      void releaseSession(Session* session);
      void syncSessions(NodeList* masterList, SessionList* outRemovedSessions,
         StringList* outUnremovableSesssions);

      size_t getAllSessionIDs(StringList* outSessionIDs);
      size_t getSize();

      unsigned serializeForTarget(char* buf, uint16_t targetID);
      bool deserializeForTarget(const char* buf, size_t bufLen, unsigned* outLen,
         uint16_t targetID);
      unsigned serialLenForTarget(uint16_t targetID);

      bool loadFromFile(std::string filePath, uint16_t targetID);
      bool saveToFile(std::string filePath, uint16_t targetID);

   private:
      SessionMap sessions;

      Mutex mutex;

      void addSession(Session* session); // actually not needed (maybe later one day...)
      bool removeSession(std::string sessionID); // actually not needed (maybe later one day...)
      Session* removeSessionUnlocked(std::string sessionID);
};

#endif /*SESSIONSTORE_H_*/

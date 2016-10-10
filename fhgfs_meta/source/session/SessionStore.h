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
      friend class TestSerialization; // for testing

   public:
      SessionStore() {}
      
      Session* referenceSession(std::string sessionID, bool addIfNotExists=true);
      void releaseSession(Session* session);
      void syncSessions(NodeList* masterList, bool doRemove, SessionList* outRemovedSessions,
         StringList* outUnremovableSesssions);
      
      size_t getAllSessionIDs(StringList* outSessionIDs);
      size_t getSize();

      unsigned serialize(char* buf);
      bool deserialize(const char* buf, size_t bufLen, unsigned* outLen);
      unsigned serialLen();

      bool loadFromFile(const std::string& filePath);
      bool saveToFile(const std::string& filePath);

      friend bool sessionStoreMetaEquals(SessionStore& first, SessionStore& second,
         bool disableInodeCheck);
      bool relinkInodes(MetaStore& store)
      {
         bool result = true;

         for (SessionMapIter it = sessions.begin(); it != sessions.end(); ++it)
            result &= it->second->getReferencedObject()->relinkInodes(store);

         return result;
      }


   private:
      SessionMap sessions;
      
      Mutex mutex;

      void addSession(Session* session); // actually not needed (maybe later one day...)
      bool removeSession(std::string sessionID); // actually not needed (maybe later one day...)
      Session* removeSessionUnlocked(std::string sessionID);

      bool deserializeLockStates(const char* buf, unsigned& bufPos, size_t bufLen);
};

#endif /*SESSIONSTORE_H_*/

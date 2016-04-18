#ifndef NODESTORESERVERSEX_H_
#define NODESTORESERVERSEX_H_

#include <common/nodes/NodeStoreServers.h>


class NodeStoreServersEx : public NodeStoreServers
{
   public:
      NodeStoreServersEx(NodeType storeType) throw(InvalidConfigException);

      virtual bool addOrUpdateNodeEx(Node** node, uint16_t* outNodeNumID=NULL);
      virtual bool deleteNode(uint16_t nodeID);
      
      virtual bool setRootNodeNumID(uint16_t nodeID, bool ignoreExistingRoot);
      
      uint16_t getLowestNodeID();

      bool loadFromFile();
      bool saveToFile();

      
   protected:
   
      
   private:
      Mutex storeMutex; // syncs access to the storePath file
      std::string storePath; // not thread-safe!
      bool storeDirty; // true if saved store file needs to be updated
      
      
      bool loadFromBuf(const char* buf, unsigned bufLen);
      unsigned saveToBuf(char* buf, unsigned bufLen);
      
      
   public:
      // getters & setters   
      void setStorePath(std::string storePath)
      {
         this->storePath = storePath;
      }
      
      bool isStoreDirty()
      {
         SafeMutexLock mutexLock(&storeMutex);
         bool retVal = this->storeDirty;
         mutexLock.unlock();
         
         return retVal;
      }

};

#endif /*NODESTORESERVERSEX_H_*/

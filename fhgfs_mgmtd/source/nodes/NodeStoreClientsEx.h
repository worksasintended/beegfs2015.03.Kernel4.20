#ifndef NODESTORECLIENTSEX_H_
#define NODESTORECLIENTSEX_H_

#include <common/nodes/NodeStoreClients.h>


class NodeStoreClientsEx : public NodeStoreClients
{
   public:
      NodeStoreClientsEx() throw(InvalidConfigException);

      virtual bool addOrUpdateNode(Node** node);
      virtual bool deleteNode(std::string nodeID);
      
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

#endif /*NODESTORECLIENTSEX_H_*/

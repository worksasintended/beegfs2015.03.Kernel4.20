#ifndef NODESTORESERVERSEX_H_
#define NODESTORESERVERSEX_H_

#include <common/nodes/NodeStoreServers.h>

class NodeStoreServersEx : public NodeStoreServers
{
   public:
      NodeStoreServersEx(NodeType storeType) throw(InvalidConfigException);
   
      virtual bool addOrUpdateNode(Node** node);

      
   protected:

      
   private:

      
   public:
      // getters & setters   

};

#endif /*NODESTORESERVERSEX_H_*/

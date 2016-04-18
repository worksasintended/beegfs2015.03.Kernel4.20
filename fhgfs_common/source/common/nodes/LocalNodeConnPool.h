#ifndef LOCALNODECONNPOOL_H_
#define LOCALNODECONNPOOL_H_

#include <common/components/worker/LocalConnWorker.h>
#include <common/net/sock/NetworkInterfaceCard.h>
#include <common/threading/Mutex.h>
#include <common/threading/Condition.h>
#include <common/nodes/NodeConnPool.h>
#include <common/Common.h>


typedef std::list<LocalConnWorker*> LocalConnWorkerList;
typedef LocalConnWorkerList::iterator LocalConnWorkerListIter;

/**
 * This is the conn pool which is used when a node sends network messages to itself, e.g. like a
 * single mds would do in case of an incoming mkdir msg.
 * It is based on unix sockets for inter-process communication instead of network sockets and
 * creates a handler thread for each established connection to process "incoming" msgs.
 */
class LocalNodeConnPool : public NodeConnPool
{
   public:
      LocalNodeConnPool(Node* parentNode, NicAddressList& nicList);
      virtual ~LocalNodeConnPool();
      
      Socket* acquireStreamSocketEx(bool allowWaiting);
      void releaseStreamSocket(Socket* sock);
      void invalidateStreamSocket(Socket* sock);
      
   private:
      NicAddressList nicList;
      LocalConnWorkerList connWorkerList;
      
      unsigned availableConns; // available established conns
      unsigned establishedConns; // not equal to connList.size!!
      unsigned maxConns;
      
      int numCreatedWorkers;
      
      Mutex mutex;
      Condition changeCond;
      
   public:
      // getters & setters
      
      NicAddressList getNicList()
      {
         // Note: Not synced (like in normal NodeConnPool) because the nicList will never
         //    change in the local connpool impl.
         
         return nicList;
      }


};

#endif /*LOCALNODECONNPOOL_H_*/

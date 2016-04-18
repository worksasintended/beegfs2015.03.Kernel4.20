#include <common/threading/SafeMutexLock.h>
#include <common/toolkit/Random.h>
#include <common/toolkit/serialization/Serialization.h>
#include <program/Program.h>
#include "NodeStoreServersEx.h"


#define NODESTOREEX_SERBUF_SIZE  (1024*1024)
#define NODESTOREEX_TMPFILE_EXT  ".tmp" /* temporary extension for saved files until we rename */


NodeStoreServersEx::NodeStoreServersEx(NodeType storeType) throw(InvalidConfigException) :
   NodeStoreServers(storeType, false)
{
   this->storeDirty = false;
}

bool NodeStoreServersEx::addOrUpdateNodeEx(Node** node, uint16_t* outNodeNumID)
{
   bool nodeAdded = NodeStore::addOrUpdateNodeEx(node, outNodeNumID);

   // Note: NodeStore might be dirty even if nodeAdded==false
   //    (e.g. because the ports might have changed)

   SafeMutexLock mutexLock(&storeMutex);
   storeDirty = true;
   mutexLock.unlock();

   return nodeAdded;
}

bool NodeStoreServersEx::deleteNode(uint16_t nodeID)
{
   bool delRes = NodeStore::deleteNode(nodeID);
   
   if(delRes)
   {
      SafeMutexLock mutexLock(&storeMutex);
      storeDirty = true;
      mutexLock.unlock();
   }
   
   return delRes;
}

bool NodeStoreServersEx::setRootNodeNumID(uint16_t nodeID, bool ignoreExistingRoot)
{
   bool setRes = NodeStore::setRootNodeNumID(nodeID, ignoreExistingRoot);
   
   if(setRes)
   {
      SafeMutexLock mutexLock(&storeMutex);
      storeDirty = true;
      mutexLock.unlock();
   }
   
   return setRes;
}

/**
 * @return 0 as invalid ID if store is empty
 */
uint16_t NodeStoreServersEx::getLowestNodeID()
{
   uint16_t lowestID = 0;
   
   SafeMutexLock mutexLock(&mutex);

   if(!activeNodes.empty() )
      lowestID = activeNodes.begin()->first;

   mutexLock.unlock();

   return lowestID;
}

/**
 * Note: setStorePath must be called before using this.
 */
bool NodeStoreServersEx::loadFromFile()
{
   /* note: we use a separate storeMutex here because we don't want to keep the standard mutex
      locked during disk access. */

   LogContext log("NodeStoreServersEx (load)");
   
   bool retVal = false;
   char* buf = NULL;
   int readRes;
   
   if(!this->storePath.length() )
      return false;

   SafeMutexLock mutexLock(&storeMutex);
   
   int fd = open(storePath.c_str(), O_RDONLY, 0);
   if(fd == -1)
   { // open failed
      LOG_DEBUG_CONTEXT(log, Log_DEBUG, "Unable to open nodes file: " + storePath + ". " +
         "SysErr: " + System::getErrString() );
         
      goto err_unlock;
   }
   
   buf = (char*)malloc(NODESTOREEX_SERBUF_SIZE);
   readRes = read(fd, buf, NODESTOREEX_SERBUF_SIZE);
   if(readRes <= 0)
   { // reading failed
      LOG_DEBUG_CONTEXT(log, Log_DEBUG, "Unable to read nodes file: " + storePath + ". " +
         "SysErr: " + System::getErrString() );
   }
   else
   { // parse contents
      retVal = loadFromBuf(buf, readRes);
   }
   
   
   free(buf);
   close(fd);

err_unlock:
   mutexLock.unlock();
   
   return retVal;
}


/**
 * Note: setStorePath must be called before using this.
 */
bool NodeStoreServersEx::saveToFile()
{
   /* note: we use a separate storeMutex here because we don't want to keep the standard mutex
      locked during disk access. */

   LogContext log("NodeStoreServersEx (save)");
   
   bool retVal = false;
   
   char* buf;
   unsigned bufLen;
   ssize_t writeRes;
   int renameRes;

   if(!this->storePath.length() )
      return false;
   
   std::string storePathTmp(storePath + NODESTOREEX_TMPFILE_EXT);

   SafeMutexLock mutexLock(&storeMutex); // L O C K

   // create/trunc file
   int openFlags = O_CREAT|O_TRUNC|O_WRONLY;

   int fd = open(storePathTmp.c_str(), openFlags, 0666);
   if(fd == -1)
   { // error
      log.logErr("Unable to create nodes file: " + storePathTmp + ". " +
         "SysErr: " + System::getErrString() );
         
      goto err_unlock;
   }

   // file created => store data
   buf = (char*)malloc(NODESTOREEX_SERBUF_SIZE);
   bufLen = saveToBuf(buf, NODESTOREEX_SERBUF_SIZE);
   writeRes = write(fd, buf, bufLen);
   free(buf);
   
   if(writeRes != (ssize_t)bufLen)
   {
      log.logErr("Unable to store nodes file: " + storePathTmp + ". " +
         "SysErr: " + System::getErrString() );

      goto err_closefile;
   }

   close(fd);
   
   renameRes = rename(storePathTmp.c_str(), storePath.c_str() );
   if(renameRes == -1)
   {
      log.logErr("Unable to rename nodes file: " + storePathTmp + ". " +
         "SysErr: " + System::getErrString() );

      goto err_unlink;
   }

   storeDirty = false;
   retVal = true;

   LOG_DEBUG_CONTEXT(log, Log_DEBUG, "Nodes file stored: " + storePath);

   mutexLock.unlock(); // U N L O C K

   return retVal;

   
   // error compensation
err_closefile:
   close(fd);

err_unlink:
   unlink(storePathTmp.c_str() );

err_unlock:
   mutexLock.unlock(); // U N L O C K
   
   return retVal;
}

/**
 * Note: Does not require locking (uses the locked addOrUpdate() method).
 */
bool NodeStoreServersEx::loadFromBuf(const char* buf, unsigned bufLen)
{
   LogContext log("NodeStoreServersEx (load)");

   bool retVal = false;
   unsigned bufPos = 0;

   // rootNodeID
   
   uint16_t rootID;
   unsigned rootIDBufLen;
   
   bool rootRes = Serialization::deserializeUShort(buf, bufLen, &rootID, &rootIDBufLen);
   if(!rootRes)
   {
      log.logErr("Unable to deserialize root ID from buffer");
      return retVal;
   }

   NodeStore::setRootNodeNumID(rootID, false); /* be careful not to call the virtual method
      of this object, because that would try to save the updated file (=> deadlock) */
   
   bufPos += rootIDBufLen;

   
   // nodeList
   
   unsigned nodeListElemNum;
   const char* nodeListStart;
   unsigned nodeListBufLen;
   NodeList nodeList;
   
   bool preprocessRes = Serialization::deserializeNodeListPreprocess(&buf[bufPos], bufLen-bufPos,
      &nodeListElemNum, &nodeListStart, &nodeListBufLen);
   if(preprocessRes)
   { // success
      Serialization::deserializeNodeList(nodeListElemNum, nodeListStart, &nodeList);
   }
   else
   { // preprocessing failed
      preprocessRes = Serialization::deserializeNodeListPreprocess2012(&buf[bufPos], bufLen-bufPos,
            &nodeListElemNum, &nodeListStart, &nodeListBufLen);

      if(!preprocessRes)
      { // preprocessing in 2012 format failed as well
         log.logErr("Unable to deserialize node from buffer (preprocessing error)");
         return retVal;
      }

      Serialization::deserializeNodeList2012(nodeListElemNum, nodeListStart, &nodeList);
   }

   // add all nodes
   for(NodeListIter iter=nodeList.begin(); iter != nodeList.end(); iter++)
   {
      Node* node = *iter;
      std::string nodeID = node->getID();
      
      // set local nic capabilities...

      Node* localNode = Program::getApp()->getLocalNode(); /* (note: not the one from this store) */
      NicAddressList localNicList(localNode->getNicList() );
      NicListCapabilities localNicCaps;

      NetworkInterfaceCard::supportedCapabilities(&localNicList, &localNicCaps);
      node->getConnPool()->setLocalNicCaps(&localNicCaps);

      // actually add the node to the store...

      /* note: using NodeStoreServersEx::addOrUpdate() here would deadlock (because it would try to
         acquire the storeMutex), so we explicitly call the parent class method. */
      
      bool nodeAdded = NodeStoreServers::addOrUpdateNodeEx(&node, NULL);
      
      LOG_DEBUG_CONTEXT(log, Log_DEBUG, "Added new node from buffer: " + nodeID);
      IGNORE_UNUSED_VARIABLE(nodeAdded);
      IGNORE_UNUSED_VARIABLE(&nodeID);
   }
   
   bufPos += nodeListBufLen;

   
   retVal = true;
   
   return retVal;
}

/**
 * Note: The NodeStore mutex may not be acquired when this is called (because this uses the locked
 * referenceAllNodes() method).
 */
unsigned NodeStoreServersEx::saveToBuf(char* buf, unsigned bufLen)
{
   size_t bufPos = 0;
   
   uint16_t rootNodeID = getRootNodeNumID();

   NodeList nodeList;
   referenceAllNodes(&nodeList);

   // rootNodeID
   bufPos += Serialization::serializeUShort(&buf[bufPos], rootNodeID);
   
   // nodeList
   bufPos += Serialization::serializeNodeList(&buf[bufPos], &nodeList);

   releaseAllNodes(&nodeList);

   return bufPos;
}

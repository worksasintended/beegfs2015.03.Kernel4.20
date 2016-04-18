#ifndef NODE_H_
#define NODE_H_

#include <common/threading/Mutex.h>
#include <common/threading/Condition.h>
#include <common/toolkit/StringTk.h>
#include <common/toolkit/Time.h>
#include <common/Common.h>
#include <toolkit/BitStore.h>
#include "NodeConnPool.h"

// forward declaration
struct App;


enum NodeType;
typedef enum NodeType NodeType;

struct Node;
typedef struct Node Node;


extern void Node_init(Node* this, struct App* app, const char* nodeID, uint16_t nodeNumID,
   unsigned short portUDP, unsigned short portTCP, NicAddressList* nicList);
extern Node* Node_construct(struct App* app, const char* nodeID, uint16_t nodeNumID,
   unsigned short portUDP, unsigned short portTCP, NicAddressList* nicList);
extern void Node_uninit(Node* this);
extern void Node_destruct(Node* this);

extern void Node_updateLastHeartbeatT(Node* this);
extern void Node_getLastHeartbeatT(Node* this, Time* outT);
extern fhgfs_bool Node_waitForNewHeartbeatT(Node* this, Time* oldT, int timeoutMS);
extern fhgfs_bool Node_updateInterfaces(Node* this, unsigned short portUDP, unsigned short portTCP,
   NicAddressList* nicList);

extern const char* Node_getNodeIDWithTypeStr(Node* this);

// static
extern const char* Node_nodeTypeToStr(NodeType nodeType);

// getters & setters
static inline char* Node_getID(Node* this);
static inline uint16_t Node_getNumID(Node* this);
static inline NicAddressList* Node_getNicList(Node* this);
static inline NodeConnPool* Node_getConnPool(Node* this);
static inline void Node_setIsActive(Node* this, fhgfs_bool isActive);
static inline fhgfs_bool Node_getIsActive(Node* this);
static inline unsigned short Node_getPortUDP(Node* this);
static inline unsigned short Node_getPortTCP(Node* this);
static inline NodeType Node_getNodeType(Node* this);
static inline void Node_setNodeType(Node* this, NodeType nodeType);
static inline const char* Node_getNodeTypeStr(Node* this);
static inline unsigned Node_getFhgfsVersion(Node* this);
static inline void Node_setFhgfsVersion(Node* this, unsigned fhgfsVersion);
static inline fhgfs_bool Node_hasFeature(Node* this, unsigned featureBitIndex);
static inline void Node_addFeature(Node* this, unsigned featureBitIndex);
static inline const BitStore* Node_getNodeFeatures(Node* this);
static inline void Node_setFeatureFlags(Node* this, BitStore* featureFlags);
static inline void Node_updateFeatureFlagsThreadSafe(Node* this, const BitStore* featureFlags);

// protected inliners
static inline void _Node_setConnPool(Node* this, NodeConnPool* connPool);



enum NodeType
   {NODETYPE_Invalid = 0, NODETYPE_Meta = 1, NODETYPE_Storage = 2, NODETYPE_Client = 3,
   NODETYPE_Mgmt = 4, NODETYPE_Hsm = 5, NODETYPE_Helperd = 6};


struct Node
{
   char* id; // string ID, generated locally on each node; not thread-safe (not meant to be changed)
   uint16_t numID; // numeric ID, assigned by mgmtd server store (unused for clients)

   NodeType nodeType; // set by NodeStore::addOrUpdate()
   char* nodeIDWithTypeStr; // for log messages (initially NULL, alloc'ed when needed)

   unsigned fhgfsVersion; // fhgfs version code of this node
   BitStore nodeFeatureFlags; /* supported features of this node (access not protected by mutex,
                                 so be careful with updates) */

   NodeConnPool* connPool;
   unsigned short portUDP;

   Time* lastHeartbeatT; // last heartbeat receive time

   fhgfs_bool isActive; // for internal use by the NodeStore only

   Mutex* mutex;
   Condition* changeCond; // for last heartbeat time only
};


char* Node_getID(Node* this)
{
   return this->id;
}

uint16_t Node_getNumID(Node* this)
{
   return this->numID;
}

NicAddressList* Node_getNicList(Node* this)
{
   return NodeConnPool_getNicList(this->connPool);
}

NodeConnPool* Node_getConnPool(Node* this)
{
   return this->connPool;
}

void _Node_setConnPool(Node* this, NodeConnPool* connPool)
{
   this->connPool = connPool;
}

/**
 * Note: For nodes that live inside a NodeStore, this method should only be called by the NodeStore.
 */
void Node_setIsActive(Node* this, fhgfs_bool isActive)
{
   Mutex_lock(this->mutex);

   this->isActive = isActive;
   
   Mutex_unlock(this->mutex);
}

fhgfs_bool Node_getIsActive(Node* this)
{
   fhgfs_bool isActive;
   
   Mutex_lock(this->mutex);
   
   isActive = this->isActive;
   
   Mutex_unlock(this->mutex);

   return isActive;
}

unsigned short Node_getPortUDP(Node* this)
{
   unsigned short retVal;
   
   Mutex_lock(this->mutex);
   
   retVal = this->portUDP;
   
   Mutex_unlock(this->mutex);

   return retVal;
}

unsigned short Node_getPortTCP(Node* this)
{
   return NodeConnPool_getStreamPort(this->connPool);
}

NodeType Node_getNodeType(Node* this)
{
   return this->nodeType;
}

/**
 * Note: Not thread-safe to avoid unnecessary overhead. We assume this is called only in situations
 * where the object is not used by multiple threads, such as NodeStore::addOrUpdate() or
 * Serialization_deserializeNodeList().
 */
void Node_setNodeType(Node* this, NodeType nodeType)
{
   this->nodeType = nodeType;

   SAFE_KFREE(this->nodeIDWithTypeStr); // will be recreated on demand by _getNodeIDWithTypeStr()
}

/**
 * Returns human-readable node type string.
 *
 * @return static string (not alloced => don't free it)
 */
const char* Node_getNodeTypeStr(Node* this)
{
   return Node_nodeTypeToStr(this->nodeType);
}

unsigned Node_getFhgfsVersion(Node* this)
{
   return this->fhgfsVersion;
}

/**
 * Note: Not thread-safe to avoid unnecessary overhead. We assume this is called only in situations
 * where the object is not used by multiple threads, such as NodeStore::addOrUpdate() or
 * Serialization_deserializeNodeList().
 */
void Node_setFhgfsVersion(Node* this, unsigned fhgfsVersion)
{
   this->fhgfsVersion = fhgfsVersion;
}

/**
 * Check if this node supports a certain feature.
 */
fhgfs_bool Node_hasFeature(Node* this, unsigned featureBitIndex)
{
   return BitStore_getBitNonAtomic(&this->nodeFeatureFlags, featureBitIndex);
}

/**
 * Add a feature flag to this node.
 */
void Node_addFeature(Node* this, unsigned featureBitIndex)
{
   BitStore_setBit(&this->nodeFeatureFlags, featureBitIndex, fhgfs_true);
}

/**
 * note: returns a reference to internal flags, so this is only valid while you hold a
 * reference to this node.
 */
const BitStore* Node_getNodeFeatures(Node* this)
{
   return &this->nodeFeatureFlags;
}


/**
 * Note: not thread-safe, so use this only when there are no other threads accessing this node
 * object.
 *
 * @param featureFlags will be copied.
 */
void Node_setFeatureFlags(Node* this, BitStore* featureFlags)
{
   BitStore_copy(&this->nodeFeatureFlags, featureFlags);
}

/**
 * Update internal flags from given featureFlags.
 *
 * Access to the internal feature flags store is not protected by a mutex for performance reasons,
 * so this thread-safe method cannot allow any reallocations of the internal buffers of the store.
 * Thus, we only copy feature bits that fit in the existing buffers here.
 */
void Node_updateFeatureFlagsThreadSafe(Node* this, const BitStore* featureFlags)
{
   BitStore_copyThreadSafe(&this->nodeFeatureFlags, featureFlags);
}


#endif /*NODE_H_*/

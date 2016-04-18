#include <common/nodes/NodeFeatureFlags.h>
#include <os/OsCompat.h>
#include "Node.h"

/**
 * @param portUDP value 0 if undefined
 * @param portTCP value 0 if undefined
 * @param nicList an internal copy will be created
 */
void Node_init(Node* this, struct App* app, const char* nodeID, uint16_t nodeNumID,
   unsigned short portUDP, unsigned short portTCP, NicAddressList* nicList)
{
   this->mutex = Mutex_construct();
   this->changeCond = Condition_construct();

   this->lastHeartbeatT = Time_construct();
   this->isActive = fhgfs_false;

   this->id = StringTk_strDup(nodeID);
   this->numID = nodeNumID;
   this->nodeType = NODETYPE_Invalid; // set by NodeStore::addOrUpdate()
   this->nodeIDWithTypeStr = NULL; // will by initialized on demand by getNodeIDWithTypeStr()

   this->fhgfsVersion = 0;
   BitStore_initWithSizeAndReset(&this->nodeFeatureFlags, NODE_FEATURES_MAX_INDEX);

   this->portUDP = portUDP;

   this->connPool = NodeConnPool_construct(app, this, portTCP, nicList);
}

/**
 * @param nicList an internal copy will be created
 */
Node* Node_construct(struct App* app, const char* nodeID, uint16_t nodeNumID,
   unsigned short portUDP, unsigned short portTCP, NicAddressList* nicList)
{
   Node* this = (Node*)os_kmalloc(sizeof(*this) );

   Node_init(this, app, nodeID, nodeNumID, portUDP, portTCP, nicList);

   return this;
}

void Node_uninit(Node* this)
{
   SAFE_DESTRUCT(this->lastHeartbeatT, Time_destruct);

   SAFE_DESTRUCT(this->connPool, NodeConnPool_destruct);

   SAFE_KFREE(this->nodeIDWithTypeStr);
   SAFE_KFREE(this->id);

   BitStore_uninit(&this->nodeFeatureFlags);

   Condition_destruct(this->changeCond);
   Mutex_destruct(this->mutex);
}

void Node_destruct(Node* this)
{
   Node_uninit(this);

   os_kfree(this);
}

void Node_updateLastHeartbeatT(Node* this)
{
   Mutex_lock(this->mutex);

   Time_setToNow(this->lastHeartbeatT);

   Condition_broadcast(this->changeCond);

   Mutex_unlock(this->mutex);
}

void Node_getLastHeartbeatT(Node* this, Time* outT)
{
   Mutex_lock(this->mutex);

   Time_setFromOther(outT, this->lastHeartbeatT);

   Mutex_unlock(this->mutex);
}

fhgfs_bool Node_waitForNewHeartbeatT(Node* this, Time* oldT, int timeoutMS)
{
   fhgfs_bool heartbeatChanged;
   int remainingTimeoutMS = timeoutMS;

   Time startT;

   Time_init(&startT);

   
   Mutex_lock(this->mutex);

   
   while( (remainingTimeoutMS > 0) && Time_equals(oldT, this->lastHeartbeatT) )
   {
      if(!Condition_timedwait(this->changeCond, this->mutex, remainingTimeoutMS) )
         break; // timeout

      remainingTimeoutMS = timeoutMS - Time_elapsedMS(&startT);
   }

   heartbeatChanged = Time_notequals(oldT, this->lastHeartbeatT);


   Mutex_unlock(this->mutex);

   Time_uninit(&startT);

   return heartbeatChanged;

}

/**
 * @param portUDP value 0 if undefined
 * @param portTCP value 0 if undefined
 * @return fhgfs_true if a port changed
 */
fhgfs_bool Node_updateInterfaces(Node* this, unsigned short portUDP, unsigned short portTCP,
   NicAddressList* nicList)
{
   fhgfs_bool portChanged = fhgfs_false;
   
   Mutex_lock(this->mutex);

   if(portUDP && (portUDP != this->portUDP) )
   {
      this->portUDP = portUDP;
      portChanged = fhgfs_true;
   }
   
   if(NodeConnPool_updateInterfaces(this->connPool, portTCP, nicList) )
      portChanged = fhgfs_true;

   Mutex_unlock(this->mutex);
   
   return portChanged;
}

/**
 * Returns human-readable node type.
 *
 * @return static string (not alloced => don't free it)
 */
const char* Node_nodeTypeToStr(NodeType nodeType)
{
   switch(nodeType)
   {
      case NODETYPE_Invalid:
      {
         return "<undefined/invalid>";
      } break;

      case NODETYPE_Meta:
      {
         return "beegfs-meta";
      } break;
      
      case NODETYPE_Storage:
      {
         return "beegfs-storage";
      } break;
      
      case NODETYPE_Client:
      {
         return "beegfs-client";
      } break;

      case NODETYPE_Mgmt:
      {
         return "beegfs-mgmtd";
      } break;

      case NODETYPE_Hsm:
      {
         return "hsm";
      } break;

      case NODETYPE_Helperd:
      {
         return "beegfs-helperd";
      } break;

      default:
      {
         return "<unknown>";
      } break;
   }
}

/**
 * Returns the node type dependent ID (numeric ID for servers and string ID for clients) and the
 * node type in a human-readable string.
 *
 * This is intendened as a convenient way to get a string with node ID and type for log messages.
 *
 * @return string is alloc'ed on demand and remains valid until Node_uninit() or _setNodeType() is
 * called; caller may not free the string.
 */
const char* Node_getNodeIDWithTypeStr(Node* this)
{
   const char* retVal;

   Mutex_lock(this->mutex); // L O C K

   /* we alloc the string here on demand. it gets freed either by _uninit() or when _setNodeType()
      is called */

   if(!this->nodeIDWithTypeStr)
   { // not initialized yet => alloc and init

      if(this->nodeType == NODETYPE_Client)
         this->nodeIDWithTypeStr = kasprintf(GFP_NOFS, "%s %s",
            Node_getNodeTypeStr(this), this->id);
      else
         this->nodeIDWithTypeStr = kasprintf(GFP_NOFS, "%s %s [ID: %hu]",
            Node_getNodeTypeStr(this), this->id, this->numID);
   }

   retVal = this->nodeIDWithTypeStr;

   Mutex_unlock(this->mutex); // U N L O C K

   return retVal;
}

#include <common/net/message/helperd/GetHostByNameMsg.h>
#include <common/net/message/helperd/GetHostByNameRespMsg.h>
#include <common/nodes/Node.h>
#include <common/toolkit/MessagingTk.h>
#include <toolkit/NoAllocBufferStore.h>
#include <toolkit/SignalTk.h>
#include "ExternalHelperd.h"


void ExternalHelperd_init(ExternalHelperd* this, App* app, Config* cfg)
{
   this->helperdReachable = fhgfs_true;

   this->app = app;

   this->mutex = Mutex_construct();

   ExternalHelperd_initHelperdNode(app, cfg, "helperd", &this->helperdNode);
}

ExternalHelperd* ExternalHelperd_construct(App* app, Config* cfg)
{
   ExternalHelperd* this = (ExternalHelperd*)os_kmalloc(sizeof(*this) );

   ExternalHelperd_init(this, app, cfg);

   return this;
}

void ExternalHelperd_uninit(ExternalHelperd* this)
{
   Node_destruct(this->helperdNode);

   Mutex_destruct(this->mutex);
}

void ExternalHelperd_destruct(ExternalHelperd* this)
{
   ExternalHelperd_uninit(this);

   os_kfree(this);
}

/**
 * Note: This is a public static method, because the (Helperd-)Logger also uses it.
 */
void ExternalHelperd_initHelperdNode(App* app, Config* cfg, const char* nodeID, Node** outNode)
{
   // create the helperdNode (and the corresponding temporary NicList)

   NicAddress nicAddress;
   NicAddressList helperdNicList;
   unsigned short helperdPortTCP = Config_getConnHelperdPortTCP(cfg);
   const char* helperdIP = Config_getLogHelperdIP(cfg);
   
   if(!StringTk_hasLength(helperdIP) )
      helperdIP = "127.0.0.1"; // localhost is default for undefined helperdIP

   os_memset(&nicAddress, 0, sizeof(NicAddress) );
   nicAddress.ipAddr = SocketTk_in_aton(helperdIP);
   nicAddress.nicType = NICADDRTYPE_STANDARD;

   NicAddressList_init(&helperdNicList);
   NicAddressList_append(&helperdNicList, &nicAddress);

   *outNode = Node_construct(app,
      nodeID, 0, 0, helperdPortTCP, &helperdNicList); // (node copies the NicList)
   
   Node_setNodeType(*outNode, NODETYPE_Helperd);
   NodeConnPool_setLogConnErrors(Node_getConnPool(*outNode), fhgfs_false); // disable error logging

   // clean-up
   NicAddressList_uninit(&helperdNicList);
}

/**
 * Drop established (available) connections.
 *
 * @return number of dropped connections
 */
unsigned ExternalHelperd_dropConns(ExternalHelperd* this)
{
   NodeConnPool* connPool = Node_getConnPool(this->helperdNode);

   unsigned numDroppedConns;

   numDroppedConns = NodeConnPool_disconnectAvailableStreams(connPool);

   return numDroppedConns;
}

/**
 * @return the result is either NULL (if communication failed) or will be kalloced
 * and needs to be kfree'd by the caller.
 */
char* ExternalHelperd_getHostByName(ExternalHelperd* this, const char* hostname)
{
   NoAllocBufferStore* bufStore = App_getMsgBufStore(this->app);

   sigset_t oldSignalSet;
   GetHostByNameMsg requestMsg;
   char* respBuf;
   NetMessage* respMsg;
   FhgfsOpsErr requestRes;
   GetHostByNameRespMsg* resolveResp;
   char* retVal = NULL;

   SignalTk_blockSignals(fhgfs_true, &oldSignalSet); // B L O C K _ S I G s

   // prepare request msg
   GetHostByNameMsg_initFromHostname(&requestMsg, hostname);

   // connect & communicate
   requestRes = MessagingTk_requestResponse(this->app, this->helperdNode,
      (NetMessage*)&requestMsg, NETMSGTYPE_GetHostByNameResp, &respBuf, &respMsg);

   if(unlikely(requestRes != FhgfsOpsErr_SUCCESS) )
   { // request/response failed
      __ExternalHelperd_logUnreachableHelperd(this);

      goto cleanup_request;
   }

   __ExternalHelperd_logReachableHelperd(this);

   // handle result
   resolveResp = (GetHostByNameRespMsg*)respMsg;
   retVal = StringTk_strDup(GetHostByNameRespMsg_getHostAddr(resolveResp) );

   // clean-up
   NetMessage_virtualDestruct(respMsg);
   NoAllocBufferStore_addBuf(bufStore, respBuf);


cleanup_request:
   GetHostByNameMsg_uninit( (NetMessage*)&requestMsg);
   SignalTk_restoreSignals(&oldSignalSet); // U N B L O C K _ S I G s

   return retVal;
}

void __ExternalHelperd_logReachableHelperd(ExternalHelperd* this)
{
   Mutex_lock(this->mutex); // L O C K

   if(this->helperdReachable)
   {
      // nothing to be done (we print the message only once)
   }
   else
   {
      // helperd was unreachable before => print notice
      printk_fhgfs(KERN_NOTICE, "Helper daemon is reachable now "
         "=> corresponding functionality re-enabled\n");

      this->helperdReachable = fhgfs_true;
   }

   Mutex_unlock(this->mutex); // U N L O C K
}

void __ExternalHelperd_logUnreachableHelperd(ExternalHelperd* this)
{
   Mutex_lock(this->mutex); // L O C K

   if(!this->helperdReachable)
   {
      // we already printed the error message in this case
   }
   else
   {
      // print error message
      printk_fhgfs(KERN_WARNING, "Helper daemon is unreachable "
         "=> corresponding functionality (e.g. hostname resolution) currently unavailable\n");

      this->helperdReachable = fhgfs_false;
   }

   Mutex_unlock(this->mutex); // U N L O C K
}


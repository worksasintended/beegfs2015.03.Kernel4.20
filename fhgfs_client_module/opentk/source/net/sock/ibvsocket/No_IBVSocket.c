#ifndef BEEGFS_OPENTK_IBVERBS

#include "IBVSocket.h"


void IBVSocket_init(IBVSocket* _this)
{
   printk_fhgfs(KERN_INFO, "%s:%d: You should never see this message\n", __func__, __LINE__);
   _this->sockValid = fhgfs_false;
}


IBVSocket* IBVSocket_construct(void)
{
   IBVSocket* _this = (IBVSocket*)kmalloc(sizeof(*_this), GFP_KERNEL);

   IBVSocket_init(_this);

   return _this;
}


void IBVSocket_uninit(IBVSocket* _this)
{
   // nothing to be done here
}


void IBVSocket_destruct(IBVSocket* _this)
{
   IBVSocket_uninit(_this);

   kfree(_this);
}


fhgfs_bool IBVSocket_rdmaDevicesExist(void)
{
   return fhgfs_false;
}


fhgfs_bool IBVSocket_connectByIP(IBVSocket* _this, struct in_addr* ipaddress, unsigned short port,
   IBVCommConfig* commCfg)
{
   printk_fhgfs(KERN_INFO, "%s:%d: You should never see this message\n", __func__, __LINE__);
   return fhgfs_false;
}


fhgfs_bool IBVSocket_bind(IBVSocket* _this, unsigned short port)
{
   printk_fhgfs(KERN_INFO, "%s:%d: You should never see this message\n", __func__, __LINE__);
   return fhgfs_false;
}


fhgfs_bool IBVSocket_bindToAddr(IBVSocket* _this, struct in_addr* ipAddr, unsigned short port)
{
   printk_fhgfs(KERN_INFO, "%s:%d: You should never see this message\n", __func__, __LINE__);
   return fhgfs_false;
}


fhgfs_bool IBVSocket_listen(IBVSocket* _this)
{
   printk_fhgfs(KERN_INFO, "%s:%d: You should never see this message\n", __func__, __LINE__);
   return fhgfs_false;
}


fhgfs_bool IBVSocket_shutdown(IBVSocket* _this)
{
   printk_fhgfs(KERN_INFO, "%s:%d: You should never see this message\n", __func__, __LINE__);
   return fhgfs_false;
}


ssize_t IBVSocket_recv(IBVSocket* _this, char* buf, size_t bufLen, int flags)
{
   printk_fhgfs(KERN_INFO, "%s:%d: You should never see this message\n", __func__, __LINE__);
   return -1;
}


ssize_t IBVSocket_recvT(IBVSocket* _this, char* buf, size_t bufLen, int flags, int timeoutMS)
{
   printk_fhgfs(KERN_INFO, "%s:%d: You should never see this message\n", __func__, __LINE__);
   return -1;
}


ssize_t IBVSocket_send(IBVSocket* _this, const char* buf, size_t bufLen, int flags)
{
   printk_fhgfs(KERN_INFO, "%s:%d: You should never see this message\n", __func__, __LINE__);
   return -1;
}


/**
 * @return 0 on success, -1 on error
 */
int IBVSocket_checkConnection(IBVSocket* _this)
{
   printk_fhgfs(KERN_INFO, "%s:%d: You should never see this message\n", __func__, __LINE__);
   return -1;
}


/**
 * @return <0 on error, 0 if recv would block, >0 if recv would not block
 */
int IBVSocket_nonblockingRecvCheck(IBVSocket* _this)
{
   printk_fhgfs(KERN_INFO, "%s:%d: You should never see this message\n", __func__, __LINE__);
   return -1;
}


/**
 * @return <0 on error, 0 if recv would block, >0 if recv would not block
 */
int IBVSocket_nonblockingSendCheck(IBVSocket* _this)
{
   printk_fhgfs(KERN_INFO, "%s:%d: You should never see this message\n", __func__, __LINE__);
   return -1;
}


unsigned long IBVSocket_poll(IBVSocket* _this, short events, fhgfs_bool finishPoll)
{
   printk_fhgfs(KERN_INFO, "%s:%d: You should never see this message\n", __func__, __LINE__);
   return ~0;
}


fhgfs_bool IBVSocket_getSockValid(IBVSocket* _this)
{
   printk_fhgfs(KERN_INFO, "%s:%d: You should never see this message\n", __func__, __LINE__);
   return _this->sockValid;
}


void IBVSocket_setTypeOfService(IBVSocket* _this, int typeOfService)
{
}


#endif // BEEGFS_OPENTK_IBVERBS


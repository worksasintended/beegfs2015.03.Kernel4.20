#include <app/log/Logger.h>
#include <app/App.h>
#include <common/components/worker/WorkQueue.h>
#include <filesystem/FhgfsOpsPages.h>

#include "RWPagesWork.h"

#define RWPagesWorkQueue_SUB_NAME BEEGFS_MODULE_NAME_STR "-rwPgWQ" // read-pages-work-queue

static struct workqueue_struct* rwPagesWorkQueue = NULL;


static void RWPagesWork_processQueue(RWPagesWork* this);
static fhgfs_bool RWPagesWork_queue(RWPagesWork *this);

static FhgfsOpsErr _RWPagesWork_initReferenceFile(struct inode* inode, Fhgfs_RWType rwType,
   FileHandleType* outHandleType, RemotingIOInfo* outIOInfo);

fhgfs_bool RWPagesWork_initworkQueue(void)
{
   rwPagesWorkQueue = create_workqueue(RWPagesWorkQueue_SUB_NAME);

   return !!rwPagesWorkQueue;
}

void RWPagesWork_destroyWorkQueue(void)
{
   if (rwPagesWorkQueue)
   {
      flush_workqueue(rwPagesWorkQueue);
      destroy_workqueue(rwPagesWorkQueue);
   }
}

void RWPagesWork_flushWorkQueue(void)
{
   if (rwPagesWorkQueue)
      flush_workqueue(rwPagesWorkQueue);
}


fhgfs_bool RWPagesWork_queue(RWPagesWork *this)
{
   return queue_work(rwPagesWorkQueue, &this->kernelWork);
}

fhgfs_bool RWPagesWork_init(RWPagesWork* this, App* app, struct inode* inode,
   FhgfsChunkPageVec *pageVec, Fhgfs_RWType rwType)
{
   FhgfsOpsErr referenceRes;

   this->app           = app;
   this->inode         = inode;
   this->pageVec       = pageVec;
   this->rwType        = rwType;

   referenceRes = _RWPagesWork_initReferenceFile(inode, rwType, &this->handleType, &this->ioInfo);

   if (unlikely(referenceRes != FhgfsOpsErr_SUCCESS) )
      return fhgfs_false;

   ihold(inode); // make sure the inode does not get evicted while we are reading/writing the file

   // printk(KERN_CRIT "%s:%d (%s)\n", __FILE__, __LINE__, __func__);

   #ifdef KERNEL_HAS_2_PARAM_INIT_WORK
      INIT_WORK(&this->kernelWork, RWPagesWork_process);
   #else
      INIT_WORK(&this->kernelWork, RWPagesWork_oldProcess, &this->kernelWork);
   #endif

   return fhgfs_true;
}

/**
 * Init helper function to reference a file.
 *
 * Note: The file is already supposed to be referenced by the FhgfsOpsPages_readpages or
 *       FhgfsOpsPages_writepages, so file referencing is not supposed to fail
 */
FhgfsOpsErr _RWPagesWork_initReferenceFile(struct inode* inode, Fhgfs_RWType rwType,
   FileHandleType* outHandleType, RemotingIOInfo* outIOInfo)
{
   FhgfsOpsErr referenceRes;

   FhgfsInode* fhgfsInode = BEEGFS_INODE(inode);
   int openFlags = (rwType == BEEGFS_RWTYPE_WRITE) ? OPENFILE_ACCESS_WRITE : OPENFILE_ACCESS_READ;

   referenceRes = FhgfsInode_referenceHandle(fhgfsInode, openFlags, fhgfs_true, NULL,
      outHandleType);

   if (unlikely(referenceRes != FhgfsOpsErr_SUCCESS) )
   {  // failure
      printk_fhgfs(KERN_INFO, "Bug: file not referenced");
      dump_stack();
   }
   else
   {  // success

      //get the right openFlags (might have changed to OPENFILE_ACCESS_READWRITE)
      openFlags = FhgfsInode_handleTypeToOpenFlags(*outHandleType);

      FhgfsInode_getRefIOInfo(fhgfsInode, *outHandleType, openFlags, outIOInfo);
   }

   return referenceRes;
}


/**
 * Process the work queue
 */
void RWPagesWork_process(struct work_struct* work)
{
   RWPagesWork* thisCast = (RWPagesWork*)work;

   // printk(KERN_INFO "%s:%d (%s)\n", __FILE__, __LINE__, __func__);

   RWPagesWork_processQueue(thisCast);
}

/**
 * Needed for old INIT_WORK() with 3 parameters (before 2.6.20)
 */
void RWPagesWork_oldProcess(void* data)
{
   struct work_struct* work = (struct work_struct*) data;

   return RWPagesWork_process(work);
}

/**
 * Build worker queues
 */
fhgfs_bool RWPagesWork_createQueue(App* app, FhgfsChunkPageVec* pageVec, struct inode* inode,
    Fhgfs_RWType rwType)
{
   Logger* log = App_getLogger(app);
   const char* logContext = __func__;

   fhgfs_bool retVal = fhgfs_true;

   // printk(KERN_CRIT "%s:%d (%s)\n", __FILE__, __LINE__, __func__);

   RWPagesWork* work;

   work = RWPagesWork_construct(app, inode, pageVec, rwType);
   if (likely(work) )
   {
      fhgfs_bool queueRes;

      queueRes = RWPagesWork_queue(work);
      if (!queueRes)
      {
         Logger_logErr(log, logContext, "RWPagesWork_construct failed.");

         if (rwType == BEEGFS_RWTYPE_READ)
            FhgfsChunkPageVec_iterateAllHandleReadErr(pageVec);
         else
            FhgfsChunkPageVec_iterateAllHandleWritePages(pageVec, -EIO);

         RWPagesWork_destruct(work);
      }
   }

   if (unlikely(!work))
   {  // Creating the work-queue failed

      Logger_logErr(log, logContext, "Failed to create work queue.");

      retVal = fhgfs_false;
   }

   return retVal;

}

/**
 * Process a request from the queue
 */
void RWPagesWork_processQueue(RWPagesWork* this)
{
   App* app = this->app;
   Logger* log = App_getLogger(app);

   FhgfsInode* fhgfsInode = BEEGFS_INODE(this->inode);

   ssize_t rwRes;

   if (this->rwType == BEEGFS_RWTYPE_WRITE)
      FhgfsInode_incWriteBackCounter(fhgfsInode);

   rwRes = FhgfsOpsRemoting_rwChunkPageVec(this->pageVec, &this->ioInfo,  this->rwType);

   if (this->rwType == BEEGFS_RWTYPE_WRITE)
   {
      spin_lock(&this->inode->i_lock);
      FhgfsInode_setLastWriteBackOrIsizeWriteTime(fhgfsInode);
      FhgfsInode_decWriteBackCounter(fhgfsInode);
      FhgfsInode_unsetNoIsizeDecrease(fhgfsInode);
      spin_unlock(&this->inode->i_lock);
   }

   if (unlikely(rwRes < 0) )
   {
      // retVal = FhgfsOpsErr_toSysErr(-rwRes);
      LOG_DEBUG_FORMATTED(log, 1, __func__, "error: %s", FhgfsOpsErr_toErrString(-rwRes) );
   }
   else
   {
      LOG_DEBUG_FORMATTED(log, 5, __func__, "rwRes: %d", rwRes );
      IGNORE_UNUSED_VARIABLE(log);
   }


   //printk_fhgfs_debug(KERN_INFO, "%s: pageList %p rwRes: %d expectedReadSize %d\n",
   //   __func__, pageList, rwRes, expectedReadSize);

   RWPagesWork_destruct(this);
}


